//
//  runtime.c
//  ParasiteLoader
//
//  Created by Alexander Zielenski on 4/7/16.
//  Copyright © 2016 ParasiteTeam. All rights reserved.
//

#include "runtime.h"
static CFMutableArrayRef callbacks;

void PSNotify(CFURLRef path, CFIndex idx, CFIndex total) {
    if (callbacks == NULL) return;
    
    CFIndex i = 0;
    CFIndex tot = CFArrayGetCount(callbacks);
    
    for (i = 0; i < tot; i++) {
        PSLoaderCallback cb = CFArrayGetValueAtIndex(callbacks, i);
        cb(path, idx, total);
    }
    
    if (idx == total - 1 && callbacks != NULL) {
        CFRelease(callbacks);
        callbacks = NULL;
    }
}

void _PSRegisterCallback(PSLoaderCallback callback) {
    if (callbacks == NULL) {
        callbacks = CFArrayCreateMutable(kCFAllocatorDefault,
                                         0,
                                         NULL);
    }
    
    CFArrayAppendValue(callbacks, callback);
}
