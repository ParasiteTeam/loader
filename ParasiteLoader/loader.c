//
//  loader.c
//  ParasiteLoader
//
//  Created by Alexander Zielenski on 3/30/16.
//  Copyright © 2016 ParasiteTeam. All rights reserved.
//

#include "main.h"
#include "loader.h"

bool commit_status(bool orig, bool new, bool match_all) {
    return (orig && new) || ((new || orig) && !match_all);
}

bool check_cf_version(CFDictionaryRef filters) {
    CFArrayRef versionFilter = CFDictionaryGetValue(filters, kOPCoreFoundationVersionKey);
    
    if (versionFilter != NULL && CFGetTypeID(versionFilter) == CFArrayGetTypeID()) {
        CFIndex count = CFArrayGetCount(versionFilter);
        if (count > 2) {
            return false;
        }
        
        CFNumberRef number;
        double value;
        
        number = CFArrayGetValueAtIndex(versionFilter, 0);
        
        // get min version
        if (CFGetTypeID(number) != CFNumberGetTypeID() || !CFNumberGetValue(number, kCFNumberDoubleType, &value)) {
            return false;
        }
        
        // make sure the CF version is above min
        if (value > kCFCoreFoundationVersionNumber)
            return false;
        
        if (count != 1) {
            number = CFArrayGetValueAtIndex(versionFilter, 1);
            // Get max version
            if (CFGetTypeID(number) != CFNumberGetTypeID() || !CFNumberGetValue(number, kCFNumberDoubleType, &value)) {
                return false;
            }
            
            // make sure the CF version is below max
            if (value <= kCFCoreFoundationVersionNumber)
                return false;
        }
    }
    
    return versionFilter == NULL;
}

bool check_executable_name(CFDictionaryRef filters, CFStringRef executableName) {
    CFArrayRef executableFilter = CFDictionaryGetValue(filters, kOPExecutablesKey);
    
    if (executableFilter != NULL && CFGetTypeID(executableFilter) == CFArrayGetTypeID()) {
        for (CFIndex i = 0; i < CFArrayGetCount(executableFilter); i++) {
            CFStringRef name = CFArrayGetValueAtIndex(executableFilter, i);
            if (CFEqual(executableName, name)) {
                return true;
            }
        }
    }
    
    return executableFilter == NULL;
}

bool check_bundles(CFDictionaryRef filters) {
    CFArrayRef bundlesFilter = CFDictionaryGetValue(filters, kOPBundlesKey);
    
    if (bundlesFilter != NULL && CFGetTypeID(bundlesFilter) == CFArrayGetTypeID()) {
        for (CFIndex i = 0; i < CFArrayGetCount(bundlesFilter); i++) {
            CFTypeRef entry = CFArrayGetValueAtIndex(bundlesFilter, i);
            
            if (CFGetTypeID(entry) == CFStringGetTypeID()) {
                CFStringRef bundleName = entry;
                CFBundleRef bndl = CFBundleGetBundleWithIdentifier(bundleName);
                if (bndl != NULL) {
                    return true;
                }
                
            } else if (CFGetTypeID(entry) == CFDictionaryGetTypeID()) {
                CFDictionaryRef filter = entry;
                CFStringRef bundleName = CFDictionaryGetValue(filter, kOPBundleIdentifierKey);
                
                if (bundleName != NULL) {
                    CFBundleRef bndl = CFBundleGetBundleWithIdentifier(bundleName);
                    if (bndl == NULL) continue;
                    
                    CFNumberRef minNum = CFDictionaryGetValue(filter, kOPMinBundleVersionKey);
                    CFNumberRef maxNum = CFDictionaryGetValue(filter, kOPMaxBundleVersionKey);
                    
                    unsigned int version = CFBundleGetVersionNumber(bndl);
                    
                    if (minNum != NULL) {
                        unsigned int min = 0;
                        CFNumberGetValue(minNum, kCFNumberIntType, &min);
                        if (min > version) continue;
                    }
                    
                    if (maxNum != NULL) {
                        unsigned int max = 0;
                        CFNumberGetValue(maxNum, kCFNumberIntType, &max);
                        if (max < version) continue;
                    }
                    
                    return true;
                }
            }
        }
    }
    
    return bundlesFilter == NULL;
}

bool check_classes(CFDictionaryRef filters) {
    CFArrayRef classesFilter = CFDictionaryGetValue(filters, kOPClassesKey);
    
    if (classesFilter != NULL && CFGetTypeID(classesFilter) == CFArrayGetTypeID()) {
        for (CFIndex i = 0; i < CFArrayGetCount(classesFilter); i++) {
            CFStringRef class = CFArrayGetValueAtIndex(classesFilter, i);
            static void *(*getClass)(const char *) = NULL;
            static dispatch_once_t onceToken;
            dispatch_once(&onceToken, ^{
                getClass = dlsym(RTLD_DEFAULT, "objc_getClass");
                
            });
            
            if (getClass != NULL) {
                if (getClass(CFStringGetCStringPtr(class, kCFStringEncodingUTF8)) != NULL) {
                    return true;
                }
            }
        }
    }
    
    return classesFilter == NULL;
}

void __ParasiteProcessExtensions(CFURLRef libraries, CFBundleRef mainBundle, CFStringRef executableName) {
    if (libraries == NULL)
        return;
    
    CFBundleRef folder = CFBundleCreate(kCFAllocatorDefault, libraries);
    if (folder == NULL) return;
    
    CFArrayRef bundles = CFBundleCopyResourceURLsOfType(folder, CFSTR("bundle"), NULL);
    CFRelease(folder);
    if (bundles == NULL) return;
    
    for (CFIndex i = 0; i < CFArrayGetCount(bundles); i++) {
        bool status = true;
        
        CFURLRef bundleURL = CFArrayGetValueAtIndex(bundles, i);
        CFBundleRef bundle = CFBundleCreate(kCFAllocatorDefault, bundleURL);
        if (bundle == NULL) {
            continue;
        }
        
        CFDictionaryRef info = CFBundleGetInfoDictionary(bundle);
        if (info == NULL || CFGetTypeID(info) != CFDictionaryGetTypeID()) {
            CFRelease(bundle);
            continue;
        }
        
        CFDictionaryRef filters = CFDictionaryGetValue(info, kOPFiltersKey);
        if (filters == NULL || CFGetTypeID(filters) != CFDictionaryGetTypeID()) {
            CFRelease(bundle);
            continue;
        }
        
        bool match_all = true;
        CFStringRef mode = CFDictionaryGetValue(filters, kOPModeKey);
        if (mode != NULL) {
            match_all = !CFEqual(mode, kOPAnyValue);
        }
        
        status = commit_status(status, check_cf_version(filters), match_all);
        status = commit_status(status, check_executable_name(filters, executableName), match_all);
        status = commit_status(status, check_bundles(filters), match_all);
        status = commit_status(status, check_classes(filters), match_all);
        
        if (status) {
            // CFBundleLoad doesn't use the correct dlopen flags
            CFURLRef executableURL = CFBundleCopyExecutableURL(bundle);

            if (executableURL != NULL) {
                const char executablePath[PATH_MAX];
                CFURLGetFileSystemRepresentation(executableURL, true, (UInt8*)&executablePath, PATH_MAX);
                CFRelease(executableURL);
                
                // load the dylib
                void *handle = dlopen(executablePath, RTLD_LAZY | RTLD_GLOBAL);
                if (handle == NULL) {
                     OPLog(OPLogLevelError, "%s", dlerror());
                }
            }
        }
        
        CFRelease(bundle);
    }
    
fin:
    CFRelease(bundles);
}