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

static volatile bool ledFinished = false;
static volatile bool moveFinished = false;

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