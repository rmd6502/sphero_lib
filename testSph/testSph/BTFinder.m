//
//  BTFinder.m
//  testSph
//
//  Created by Robert Diamond on 5/4/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "BTFinder.h"

@implementation BTFinder
@synthesize devices;

- (id)init {
    self = [super init];
    if (self != nil) {
        //[[[IOBluetoothDeviceInquiry inquiryWithDelegate:self] retain]start];
        devices = [IOBluetoothDevice pairedDevices];
    }
    return self;
}

- (NSString *)getSpheroDevice {
    return nil;
}

@end
