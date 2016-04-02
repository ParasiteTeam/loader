//
//  loader.c
//  ParasiteLoader
//
//  Created by Alexander Zielenski on 3/30/16.
//  Copyright © 2016 ParasiteTeam. All rights reserved.
//

#include "main.h"
#include "loader.h"

static void *(*getClassObj)(void *) = NULL;
static void *(*getClass)(const char *) = NULL;
static void *(*registerName)(const char *) = NULL;
static bool (*respondsToSelector)(void *, void *) = NULL;
// Not worrying about casting this correctly
// Since I only ever use it on + (void)install;
// which is void (*)(id, SEL)
static void (*msgSend)(void *, void *);

void solve_symbols() {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        getClass = dlsym(RTLD_DEFAULT, "objc_getClass");
        getClassObj = dlsym(RTLD_DEFAULT, "object_getClass");
        registerName = dlsym(RTLD_DEFAULT, "sel_registerName");
        respondsToSelector = dlsym(RTLD_DEFAULT, "class_respondsToSelector");
        msgSend = dlsym(RTLD_DEFAULT, "objc_msgSend");
    });
}

bool commit_status(bool orig, bool new, bool match_all) {
    return (orig && new) || ((new || orig) && !match_all);
}

bool check_cf_version(CFDictionaryRef filters) {
    CFArrayRef versionFilter = CFDictionaryGetValue(filters, kPSCoreFoundationVersionKey);
    
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
    CFArrayRef executableFilter = CFDictionaryGetValue(filters, kPSExecutablesKey);
    
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

bool check_bundles(CFArrayRef bundlesFilter) {
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
                CFStringRef bundleName = CFDictionaryGetValue(filter, kPSBundleIdentifierKey);
                
                if (bundleName != NULL) {
                    if (CFEqual(bundleName, CFSTR("*"))) {
                        // * means we want to load into anything with a bundle
                        // this is to keep this functionality on par with the behavior of SIMBL
                        // which only loads into cocoa applications.
                        // If you really want to load into every process I suggest you rethink
                        // what you are doing.
                        return CFBundleGetMainBundle() != NULL;
                    }
                    
                    CFBundleRef bndl = CFBundleGetBundleWithIdentifier(bundleName);
                    if (bndl == NULL) continue;
                    
                    CFNumberRef minNum = CFDictionaryGetValue(filter, kPSMinBundleVersionKey);
                    CFNumberRef maxNum = CFDictionaryGetValue(filter, kPSMaxBundleVersionKey);
                    
                    unsigned int version = CFBundleGetVersionNumber(bndl);
                    
                    if (minNum != NULL) {
                        unsigned int min = 0;
                        // Some people might put the number as a string instead of a number
                        // help them with that
                        if (CFGetTypeID(minNum) == CFStringGetTypeID()) {
                            min = CFStringGetIntValue((CFStringRef)minNum);
                            
                        } else if (CFGetTypeID(minNum) == CFNumberGetTypeID()) {
                            CFNumberGetValue(minNum, kCFNumberIntType, &min);
                        }
                        
                        if (min > version) continue;
                    }
                    
                    if (maxNum != NULL) {
                        unsigned int max = 0;
                        if (CFGetTypeID(maxNum) == CFStringGetTypeID()) {
                            max = CFStringGetIntValue((CFStringRef)maxNum);
                            
                        } else if (CFGetTypeID(maxNum) == CFNumberGetTypeID()) {
                            CFNumberGetValue(maxNum, kCFNumberIntType, &max);
                        }
                        
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
    CFArrayRef classesFilter = CFDictionaryGetValue(filters, kPSClassesKey);
    
    if (classesFilter != NULL && CFGetTypeID(classesFilter) == CFArrayGetTypeID()) {
        for (CFIndex i = 0; i < CFArrayGetCount(classesFilter); i++) {
            CFStringRef class = CFArrayGetValueAtIndex(classesFilter, i);
            solve_symbols();
            
            if (getClass != NULL) {
                if (getClass(CFStringGetCStringPtr(class, kCFStringEncodingUTF8)) != NULL) {
                    return true;
                }
            }
        }
    }
    
    return classesFilter == NULL;
}

void load_simbl(CFBundleRef bundle) {
    // We need to somehow get a pointer to the principal class in this bundle and
    // call +install on it
    CFDictionaryRef info = CFBundleGetInfoDictionary(bundle);
    CFBundleLoadExecutable(bundle);
    
    solve_symbols();
    
    // Could not solve symbols?
    if (getClass == NULL) return;
    
    // Some classes have a +install method that the SIMBL agent calls upon loading.
    // We should atleast make an attempt to call it
    CFStringRef principalClassName = CFDictionaryGetValue(info, CFSTR("NSPrincipalClass"));
    
    // want the metaclass of the principal class since install is a class method
    void *principalClass = getClassObj(getClass(CFStringGetCStringPtr(principalClassName, kCFStringEncodingUTF8)));
    void *install = registerName("install");
    if (principalClass != NULL && respondsToSelector(principalClass, install)) {
        msgSend(principalClass, install);
    }
}

void __ParasiteProcessExtensions(CFURLRef libraries, CFBundleRef mainBundle, CFStringRef executableName) {
    if (libraries == NULL)
        return;
    
    CFBundleRef folder = CFBundleCreate(kCFAllocatorDefault, libraries);
    if (folder == NULL) return;
    
    CFArrayRef bundles = CFBundleCopyResourceURLsOfType(folder, CFSTR("bundle"), NULL);
    CFRelease(folder);
    if (bundles == NULL) return;
    
    CFMutableArrayRef shouldLoad = CFArrayCreateMutable(kCFAllocatorDefault, CFArrayGetCount(bundles), &kCFTypeArrayCallBacks);
    CFMutableArrayRef simblLoad = CFArrayCreateMutable(kCFAllocatorDefault, CFArrayGetCount(bundles), &kCFTypeArrayCallBacks);
    
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
        
        // By default try our filters, if it doesnt exist or is the wrong type
        // then we check for SIMBL filters
        CFDictionaryRef filters = CFDictionaryGetValue(info, kPSFiltersKey);
        
        // backwards support for OPFilters instead of PSFilters, they are
        // the same format, anyway
        if (filters == NULL || CFGetTypeID(filters) != CFDictionaryGetTypeID()) {
            filters = CFDictionaryGetValue(info, kOPFiltersKey);
        }
        
        if (filters == NULL || CFGetTypeID(filters) != CFDictionaryGetTypeID()) {
            if ((filters = CFDictionaryGetValue(info, kSIMBLFiltersKey)) != NULL &&
                CFGetTypeID(filters) == CFArrayGetTypeID()) {
                // We have a SIMBL plugin, try to load it
                // we can safely load it here because unlike Parasite of Opee, SIMBL
                // bundle identifiers are specifically the application running rather than
                // a library that might be loaded. This is lucky because it saves me about 10 minutes
                // reformatting this function.
                if (check_bundles((CFArrayRef)filters)) {
                    CFArrayAppendValue(simblLoad, bundle);
                }
            }
            
        } else {
            bool match_all = true;
            CFStringRef mode = CFDictionaryGetValue(filters, kPSModeKey);
            if (mode != NULL) {
                match_all = !CFEqual(mode, kPSAnyValue);
            }
            
            status = commit_status(status, check_cf_version(filters), match_all);
            status = commit_status(status, check_executable_name(filters, executableName), match_all);
            status = commit_status(status, check_bundles(CFDictionaryGetValue(filters, kPSBundlesKey)), match_all);
            status = commit_status(status, check_classes(filters), match_all);
            
            if (status) {
                CFURLRef executableURL = CFBundleCopyExecutableURL(bundle);
                if (executableURL != NULL) {
                    CFArrayAppendValue(shouldLoad, executableURL);
                    CFRelease(executableURL);
                }
            }
        }
    
        
        CFRelease(bundle);
    }
    
    for (CFIndex i = 0; i < CFArrayGetCount(shouldLoad); i++) {
        CFURLRef executableURL = CFArrayGetValueAtIndex(shouldLoad, i);
        const char executablePath[PATH_MAX];
        CFURLGetFileSystemRepresentation(executableURL, true, (UInt8*)&executablePath, PATH_MAX);
        
        // load the dylib
        // CFBundleLoad doesn't use the correct dlopen flags
        void *handle = dlopen(executablePath, RTLD_LAZY | RTLD_GLOBAL);
        if (handle == NULL) {
            OPLog(OPLogLevelError, "%s", dlerror());
        }
    }
    
    for (CFIndex i = 0; i < CFArrayGetCount(simblLoad); i++) {
        CFBundleRef bndl = (CFBundleRef)CFArrayGetValueAtIndex(simblLoad, i);
        if (CFGetTypeID(bndl) == CFBundleGetTypeID())
            load_simbl(bndl);
    }
    
    CFRelease(bundles);
    CFRelease(shouldLoad);
}