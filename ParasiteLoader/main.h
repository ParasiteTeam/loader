//
//  main.h
//  ParasiteLoader
//
//  Created by Alexander Zielenski on 3/30/16.
//  Copyright © 2016 ParasiteTeam. All rights reserved.
//

#ifndef main_h
#define main_h
#include <stdio.h>
#include <dlfcn.h>
#include <CoreFoundation/CoreFoundation.h>

extern const CFStringRef kOPFiltersKey;
extern const CFStringRef kSIMBLFiltersKey;
extern const char *OPLibrariesPath;

// SIMBL
extern const CFStringRef kOPBundleIdentifierKey;
extern const CFStringRef kOPMinBundleVersionKey;
extern const CFStringRef kOPMaxBundleVersionKey;

// OPFilters
extern const CFStringRef kOPCoreFoundationVersionKey;
extern const CFStringRef kOPModeKey;
extern const CFStringRef kOPAnyValue;
extern const CFStringRef kOPExecutablesKey;
extern const CFStringRef kOPBundlesKey;
extern const CFStringRef kOPClassesKey;
extern const CFStringRef kOPAllowsRootKey;

#endif /* main_h */
