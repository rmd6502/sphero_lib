//
//  main.cpp
//  testSph
//
//  Created by Robert Diamond on 5/1/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <iostream>
#include "Sphero/Sphero.h"
//#import "BTFinder.h"

void ledResponse(const SpheroResponse &response); 
void motResponse(const SpheroResponse &response);
void asyncResponse(const SpheroResponse &response);

static volatile bool ledFinished = false;
static volatile bool moveFinished = false;
static volatile int sampleCount;

int main(int argc, const char * argv[])
{
    NSAutoreleasePool *arp = [[NSAutoreleasePool alloc] init];
    //BTFinder *btf = [[BTFinder alloc] init];
    LEDCommand l(.5, 0, .1);
    MoveCommand m(270, .5);
    
    std::cout << "Opening sphero" << std::endl;
    try {
        Sphero sph("/dev/cu.Robs-Sphero-RN-SPP");
        std::cout << "Sending LED command: " << l << std::endl;
        sph.sendCommand(l, ledResponse);
        std::cout << "Sending Move command: " << m << std::endl;
        sph.sendCommand(m, motResponse);
        while (!ledFinished || !moveFinished) {
            sleep(1);
        }
        m.setVelocity(0);
        sph.sendCommand(m, NULL);
        SetDataStreamingCommand s(20, 2, 0xe000, 100);
        sampleCount = 200;
        sph.setAsyncHandler(asyncResponse);
        sph.sendCommand(s, NULL);
        while (sampleCount) {
            sleep(1);
        }
        SleepCommand sl;
        sph.sendCommand(sl);
    } catch (std::string &e) {
        std::cerr << e << std::endl;
    }
    
    
    [arp drain];
    return 0;
}

void ledResponse(const SpheroResponse &response) {
    std::cout << "Got LED response" << std::endl;
    ledFinished = true;
}
void motResponse(const SpheroResponse &response) {
    std::cout << "Got motor response" << std::endl;
    moveFinished = true;
}
void asyncResponse(const SpheroResponse &response) {
    --sampleCount;
    const uint8_t *fv = (const uint8_t *)&response.respData()[0];
    for (int i=0; i < 6; ++i) {
        uint16_t val = (fv[i * 2] << 8) + fv[i * 2 + 1];
        std::cout << "param " << std::dec << i << " value " << (val/65535.f) << std::endl;
    }
}