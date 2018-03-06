//
//  PLCrashCallbacksDelegate.h
//  CrashReporter
//
//  Created by Xavier Fortin on 2018-03-06.
//

#ifndef PLCrashCallbacksDelegate_h
#define PLCrashCallbacksDelegate_h

@protocol PLCrashCallbacksDelegate <NSObject>

@required

- (void) executeCrashCallbacks;

@end

#endif /* PLCrashCallbacksDelegate_h */
