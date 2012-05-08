//
//  Sphero.cpp
//  Sphero
//
//  Created by Robert Diamond on 4/29/12.
//  Copyright (c) 2012 Robert Diamond. All rights reserved.
//

#include <ios>
#include <iomanip>
#include <string>
#include <sstream>
#include <numeric>
#include <vector>
#include <pthread.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include "Sphero.h"

int Command::seq = 0;

std::string Command::getPacket() {
    std::ostringstream out, out2;
    std::string data = packetData();
    
    out2 << (uint8_t)0xff;
    if (isAsync)  out2 << (uint8_t)0xfe; else out2 << (uint8_t)0xff;
    out << commandHeader((uint16_t)data.length())
        << data;
    std::string ret = out.str();
    out2 << ret << checksum(ret);
    ret = out2.str();
    
    return ret;
}

std::ostream& operator<<(std::ostream& out, Command& cmd) {
    std::string s = cmd.getPacket();
    for (std::string::const_iterator it = s.begin(); it != s.end(); ++it ) {
        out << std::hex << std::setfill('0') << std::setw(2) << (uint16_t)(*it & 0xff);
    }
    out << std::endl;
    return out;
}

std::string Command::checksum(const std::string& data) const {
    std::ostringstream out;
    uint8_t sum = (uint8_t)std::accumulate(data.begin(), data.end(), 0);
    out << (uint8_t)(~sum & 0xff);
    return out.str();
}

std::string Command::commandHeader(uint16_t dlen) {
    std::ostringstream out;
    out << _did << _cid << (uint8_t)sequence_no << (uint8_t)(dlen + 1);
    return out.str();
}

void Command::out16(std::ostream &out, uint16_t data) {
    out << (uint8_t)(data >> 8) << (uint8_t)(data & 0xff);
}
void Command::out32(std::ostream &out, uint32_t data) {
    out16(out, data >> 16);
    out16(out, data & 0xffff);
}

MoveCommand::MoveCommand(int heading, float velocity) 
: heading(heading), velocity(velocity), Command(0x02,0x30) {
    
}

std::string MoveCommand::packetData() {
    std::ostringstream out;
    out << (uint8_t)(velocity * 255);
    out16(out, (uint16_t)heading);
    if (velocity == 0) {
        out << (char)0x00;
    } else {
        out << (char)0x01;
    }
    return out.str();
}

LEDCommand::LEDCommand(float red, float green, float blue, bool persist)
: red(red), green(green), blue(blue), persist(persist), Command(0x02,0x20) {
    
}

std::string LEDCommand::packetData() {
    std::ostringstream out;
    out << (uint8_t)(red * 255) << (uint8_t)(green * 255) << (uint8_t)(blue * 255)
        << (uint8_t)persist;
    return out.str();
}

std::string SetDataStreamingCommand::packetData() {
    std::ostringstream out;
    out16(out, divisor);
    out16(out, framesPerPacket);
    out32(out, mask);
    out << packetCount;
    return out.str();
}

Sphero::Sphero(const std::string &portPath) 
: asyncHandler(NULL), shouldStop(false) {
    sphero_fh = open(portPath.c_str(), O_RDWR|O_NOCTTY|O_NDELAY|O_SYMLINK);
    if (sphero_fh < 0) {
        std::ostringstream out;
        out << "Could not open " << portPath << ": " << strerror(errno);
        throw out.str();
    }
    parser = new SpheroResponse;
    pthread_mutex_init(&responderLock, NULL);
    pthread_create(&reader_thread, NULL, Sphero::reader, this);
    termios t;
    tcgetattr(sphero_fh, &t);
    cfsetspeed(&t, B115200);
    t.c_cflag |= (CLOCAL | CREAD);
    t.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    t.c_oflag &= ~OPOST;
    t.c_cflag &= ~PARENB;
    t.c_cflag &= ~CSTOPB;
    t.c_cflag &= ~CSIZE;
    t.c_cflag |= CS8;
    tcsetattr(sphero_fh, TCSANOW, &t);
}

Sphero::~Sphero() {
    shouldStop = true;
    pthread_join(reader_thread, NULL);
    pthread_mutex_destroy(&responderLock);
    close(sphero_fh);
    delete parser;
}

int SpheroResponse::parse(std::vector<uint8_t> &buf) {    
    std::vector<uint8_t>::iterator bufPtr = buf.begin();
    bool breakout = false;
    while (!breakout && bufPtr != buf.end()) {
        uint8_t b = *bufPtr++;
        std::cout << "state " << _currentState 
            << " b " << std::hex << std::setfill('0') << std::setw(2) << (uint16_t)(b & 0xff) << std::endl;
        switch (_currentState) {
            case SOP1:
                if (b != 0xff) continue;
                _currentState = SOP2;
                break;
            case SOP2:
                if (b == 0xff) {
                    // we expect this to be in response to a command
                    _isAsync = false;
                } else if (b == 0xfe) {
                    _isAsync = true;
                } else {
                    _currentState = SOP1;
                    continue;
                }
                _currentState = MRSP;
                break;
            case MRSP:
                // for async packets this is ID
                _mrsp = b;
                _seq = 0;
                _currentState = _isAsync ? DLEN : SEQ;
                break;
            case SEQ:
                _seq = b;
                _currentState = DLEN;
                break;
            case DLEN:
                if (_isAsync) {
                    if (buf.end() - bufPtr < 2) {
                        breakout = true;
                        break;
                    }
                    _data_len = (uint16_t)(b << 8u) + *bufPtr++;
                } else {
                    _data_len = b;
                }
                // the last data byte is the checksum
                --_data_len;
                if (_data_len) {
                    _currentState = DATA;
                } else {
                    _currentState = CHECKSUM;
                }
                _respData.clear();
                break;
            case DATA:
                _respData.push_back(b);
                if (_respData.size() == _data_len) {
                    _currentState = CHECKSUM;
                }
                break;
            case CHECKSUM:
                _sum = b;
                _currentState = FINISHED;
                breakout = true;
                break;
            default:
                break;
        }
    }
    buf.erase(buf.begin(), bufPtr);
    if (_currentState == FINISHED) {
        _currentState = SOP1;
        uint8_t tsum = (uint8_t)(std::accumulate(_respData.begin(), _respData.end(), 0) 
                                 + _mrsp + _seq + _data_len + 1);
        if (_sum != (uint8_t)(~tsum)) {
            return CHECKSUM_FAIL;
        } else {
            return PARSE_FINISHED;
        }
    }
    return NOT_FINISHED;
}

void *Sphero::reader(void *passed_this) {
    Sphero *my_this = (Sphero *)passed_this;
    fd_set rdlist, erlist;
    FD_ZERO(&rdlist);
    FD_ZERO(&erlist);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    std::vector<uint8_t> inputbuf;
    while (!my_this->shouldStop) {
        FD_SET(my_this->sphero_fh, &rdlist);
        FD_SET(my_this->sphero_fh, &erlist);
        int ret = select(my_this->sphero_fh+1, &rdlist, NULL, &erlist, &tv);
        if (ret > 0) {
            if (FD_ISSET(my_this->sphero_fh, &erlist)) {
                break;
            }
            int bytes;
            ioctl(my_this->sphero_fh, FIONREAD, &bytes);
            uint8_t *buf = new uint8_t[bytes];
            read(my_this->sphero_fh, buf, bytes);
            inputbuf.insert(inputbuf.end(), buf, &buf[bytes]);
            delete [] buf;
            while (true) {
                int parse_result = my_this->parser->parse(inputbuf);
                if (parse_result == SpheroResponse::PARSE_FINISHED) {
                    if (my_this->parser->isAsync()) {
                        if (my_this->asyncHandler) {
                            my_this->asyncHandler(*my_this->parser);
                        }
                    }
                    std::map<int, ResponseHandler>::const_iterator hPair = my_this->responders.find(my_this->parser->seq());
                    if (hPair == my_this->responders.end()) {
                        if (my_this->asyncHandler) {
                            my_this->asyncHandler(*my_this->parser);
                        }
                    } else {
                        hPair->second(*my_this->parser);
                        std::map<int, ResponseHandler>::iterator hPair = my_this->responders.find(my_this->parser->seq());
                        my_this->responders.erase(hPair);
                    }
                } else {
                    break;
                }
            }
        }
    }
    return NULL;
}

void Sphero::sendCommand(Command& cmd, ResponseHandler responseHandler) {
    if (responseHandler) {
        pthread_mutex_lock(&responderLock);
        responders[cmd.sequence()] = responseHandler;
        pthread_mutex_unlock(&responderLock);
    }
    std::string s = cmd.getPacket();
    write(sphero_fh, s.data(), s.length());
}
