//
//  runtime.h
//  ParasiteLoader
//
//  Created by Alexander Zielenski on 4/7/16.
//  Copyright © 2016 ParasiteTeam. All rights reserved.
//

#ifndef runtime_h
#define runtime_h

#include <CoreFoundation/CoreFoundation.h>

typedef void(*PSLoaderCallback)(CFURLRef path, CFIndex idx, CFIndex total);

void PSNotify(CFURLRef path, CFIndex idx, CFIndex total);

#endif /* runtime_h */
