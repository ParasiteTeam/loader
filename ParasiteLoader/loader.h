//
//  loader.h
//  ParasiteLoader
//
//  Created by Alexander Zielenski on 3/30/16.
//  Copyright © 2016 ParasiteTeam. All rights reserved.
//

#ifndef loader_h
#define loader_h

#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>

void __ParasiteProcessExtensions(CFURLRef libraries, CFBundleRef bndl, CFStringRef executableName);

#endif /* loader_h */
