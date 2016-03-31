//
//  loader.c
//  ParasiteLoader
//
//  Created by Alexander Zielenski on 3/30/16.
//  Copyright © 2016 ParasiteTeam. All rights reserved.
//

#include "main.h"
#include "loader.h"

const CFStringRef kOPFiltersKey     = CFSTR("OPFilters");
const CFStringRef kSIMBLFiltersKey  = CFSTR("SIMBLTargetApplications");
const char *OPLibrariesPath         = "/Library/Parasite/Extensions";

const CFStringRef kOPBundleIdentifierKey = CFSTR("BundleIdentifier");
const CFStringRef kOPMinBundleVersionKey = CFSTR("MinBundleVersion");
const CFStringRef kOPMaxBundleVersionKey = CFSTR("MaxBundleVersion");

const CFStringRef kOPCoreFoundationVersionKey = CFSTR("CoreFoundationVersion");
const CFStringRef kOPModeKey                  = CFSTR("Mode");
const CFStringRef kOPAnyValue                 = CFSTR("Any");
const CFStringRef kOPExecutablesKey           = CFSTR("Executables");
const CFStringRef kOPBundlesKey               = CFSTR("Bundles");
const CFStringRef kOPClassesKey               = CFSTR("Classes");

__attribute__((constructor))
static void __ParasiteInit(int argc, char **argv, char **envp) {
    if (dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_LAZY | RTLD_NOLOAD) == NULL)
        return;
    
    if (argc < 1 || argv == NULL)
        return;
    
    // The first argument is the spawned process
    // Get the process name by looking at the last path
    // component.
    char *executable = strrchr(argv[0], '/');
    executable = (executable == NULL) ? argv[0] : executable + 1;

#define BLACKLIST(PROCESS) if (strcmp(executable, #PROCESS) == 0) return;
    /*
     These are processes which are blacklisted because they break
     some system functionality
     */
    BLACKLIST(coreservicesd);
    BLACKLIST(amfid);
    
    BLACKLIST(ReportCrash);
    BLACKLIST(mDNSResponder);
    BLACKLIST(useractivityd);
    BLACKLIST(syslogd);
    BLACKLIST(ssh);

    // Blacklist developer tools
    BLACKLIST(git);
    BLACKLIST(xcodebuild);
    BLACKLIST(gcc);
    BLACKLIST(lldb);
    BLACKLIST(codesign);
    BLACKLIST(svn);
    BLACKLIST(com.apple.dt.Xcode.sourcecontrol.Git);
    
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    
#ifdef DEBUG
//    CFDictionaryRef info = CFBundleGetInfoDictionary(mainBundle);
//    CFStringRef identifier = (info == NULL) ? NULL : CFDictionaryGetValue(info, kCFBundleIdentifierKey);
//    OPLog(OPLogLevelNotice, "Installing %@ [%s] (%.2f)", identifier, executable, kCFCoreFoundationVersionNumber);
#endif
    
    CFStringRef executableName = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
                                                                 executable,
                                                                 kCFStringEncodingUTF8,
                                                                 kCFAllocatorNull);
    if (executableName == NULL) return;
    
    // Process extensions for all users
    CFURLRef libraries = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                                 (const UInt8 *)OPLibrariesPath,
                                                                 strlen(OPLibrariesPath),
                                                                 true);
    if (libraries == NULL) {
        CFRelease(executableName);
        return;
    }
    
    if (access(OPLibrariesPath, X_OK | R_OK) == -1) {
        OPLog(OPLogLevelNotice, "Unable to access root libraries directory\n");
        
    } else {
        __ParasiteProcessExtensions(libraries, mainBundle, executableName);
    }
    
    CFRelease(executableName);
    CFRelease(libraries);
    
}