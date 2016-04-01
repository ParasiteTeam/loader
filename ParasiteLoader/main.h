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

extern const CFStringRef kPSFiltersKey;
extern const CFStringRef kOPFiltersKey;
extern const CFStringRef kSIMBLFiltersKey;
extern const char *PSLibrariesPath;

// SIMBL
extern const CFStringRef kPSBundleIdentifierKey;
extern const CFStringRef kPSMinBundleVersionKey;
extern const CFStringRef kPSMaxBundleVersionKey;

// OPFilters
extern const CFStringRef kPSCoreFoundationVersionKey;
extern const CFStringRef kPSModeKey;
extern const CFStringRef kPSAnyValue;
extern const CFStringRef kPSExecutablesKey;
extern const CFStringRef kPSBundlesKey;
extern const CFStringRef kPSClassesKey;

#endif /* main_h */
