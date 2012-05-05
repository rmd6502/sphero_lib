//
//  Sphero.h
//  Sphero
//
//  Created by Robert Diamond on 4/29/12.
//  Copyright (c) 2012. All rights reserved.
//

#ifndef Sphero_Sphero_h
#define Sphero_Sphero_h
#include <string>
#include <vector>
#include <pthread.h>
#include <map>

class SpheroResponse;
typedef void (*ResponseHandler)(const SpheroResponse &response);

class Command {
protected:
    Command(uint8_t did, uint8_t cid) : _did(did), _cid(cid), isAsync(false) { }
    virtual ~Command() {}
    virtual std::string packetData() = 0;
    static int seq;
    uint8_t _did, _cid;
    std::string commandHeader(uint16_t dlen = 0);
    std::string checksum(const std::string& data) const;
    void out16(std::ostream& out, uint16_t data);
    void out32(std::ostream& out, uint32_t data);
    int sequence_no;
public:
    std::string getPacket();
    bool isAsync;
    int sequence() const { return sequence_no; }
};

class MoveCommand : public Command {
public:
    MoveCommand(int heading, float velocity); 
private:
    std::string packetData();
    int heading; float velocity;
};

class LEDCommand : public Command {
public:
    LEDCommand(float red, float green, float blue, bool persist = false);
private:
    std::string packetData();
    float red; float green; float blue;
    bool persist;
};

class SetDataStreamingCommand : public Command {
private:
    uint16_t divisor; uint16_t framesPerPacket; uint32_t mask; uint8_t packetCount;
protected:
    std::string packetData();
public:
    SetDataStreamingCommand(uint16_t divisor, 
                            uint16_t framesPerPacket, 
                            uint32_t mask, 
                            uint8_t packetCount)
    : divisor(divisor), framesPerPacket(framesPerPacket), mask(mask), packetCount(packetCount),
    Command(0x02,0x11) {}
};

class SpheroResponse {
    enum ParseStates {
        SOP1, SOP2, MRSP, SEQ, DLEN, DATA, CHECKSUM, FINISHED
    };
private:
    ParseStates _currentState;
    uint8_t _mrsp, _seq, _sum;
    uint16_t _data_len;
    bool _isAsync;
    std::vector<uint8_t> _respData;
public:
    enum ParserReturns {
        CHECKSUM_FAIL = -1,NOT_FINISHED=0, PARSE_FINISHED=1
    };
    SpheroResponse() : _seq(0), _data_len(0), _currentState(SOP1), _isAsync(false) {}
    uint8_t mrsp() const { return _mrsp; }
    uint8_t seq() const { return _seq; }
    uint8_t chksum() const { return _sum; }
    bool isAsync() const { return _isAsync; }
    const std::vector<uint8_t> &respData() const { return _respData; }
    int parse(std::vector<uint8_t>& buf);
};

class Sphero {
private:
    ResponseHandler asyncHandler;
    int sphero_fh;
    std::map<int, ResponseHandler> responders;
    pthread_mutex_t responderLock;
    pthread_t reader_thread;
    static void *reader(void *passed_this);
    bool shouldStop;
    int parseBuf(std::vector<uint8_t> &buf);
    SpheroResponse *parser;
public:
    Sphero(const std::string &portPath);
    ~Sphero();
    void sendCommand(Command& cmd, ResponseHandler responseHandler = NULL);
    void setAsyncHandler(ResponseHandler responseHandler) { asyncHandler = responseHandler; }
};

#endif
