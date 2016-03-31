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
#include <syslog.h>

#define OPLogLevelNotice LOG_NOTICE
#define OPLogLevelWarning LOG_WARNING
#define OPLogLevelKernel LOG_KERN
#define OPLogLevelError LOG_ERR

#ifdef DEBUG
#define OPLog(level, format, ...) do { \
        CFStringRef _formatted = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR(format), ## __VA_ARGS__); \
        size_t _size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(_formatted), kCFStringEncodingUTF8); \
        char _utf8[_size + sizeof('\0')]; \
        CFStringGetCString(_formatted, _utf8, sizeof(_utf8), kCFStringEncodingUTF8); \
        CFRelease(_formatted); \
        syslog(level, "[ParasiteLoader] " "%s", _utf8); \
    } while (false)
#else
    #define OPLog(...) (void)1
#endif

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
