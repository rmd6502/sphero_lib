//
//  BTFinder.h
//  testSph
//
//  Created by Robert Diamond on 5/4/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <IOBluetooth/IOBluetooth.h>

@interface BTFinder : NSObject<IOBluetoothDeviceInquiryDelegate>

@property (nonatomic, readonly) NSArray *devices;

- (NSString *)getSpheroDevice;

@end
