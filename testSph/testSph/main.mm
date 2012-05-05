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

static volatile bool commandFinished = false;

int main(int argc, const char * argv[])
{
    NSAutoreleasePool *arp = [[NSAutoreleasePool alloc] init];
    //BTFinder *btf = [[BTFinder alloc] init];
    LEDCommand l(.5, 0, .1);
    
    std::cout << "Opening sphero" << std::endl;
    try {
        Sphero sph("/dev/cu.Robs-Sphero-RN-SPP");
        std::cout << "Sending LED command" << std::endl;
        sph.sendCommand(l, ledResponse);
        while (!commandFinished) {
            sleep(1);
        }
    } catch (std::string &e) {
        std::cerr << e << std::endl;
    }
    
    
    [arp drain];
    return 0;
}

void ledResponse(const SpheroResponse &response) {
    std::cout << "Got response" << std::endl;
    commandFinished = true;
}