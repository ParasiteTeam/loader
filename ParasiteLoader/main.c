//
//  loader.c
//  ParasiteLoader
//
//  Created by Alexander Zielenski on 3/30/16.
//  Copyright © 2016 ParasiteTeam. All rights reserved.
//

#include "main.h"
#include "loader.h"

const CFStringRef kPSFiltersKey     = CFSTR("PSFilters");
const CFStringRef kOPFiltersKey     = CFSTR("OPFilters");
const CFStringRef kSIMBLFiltersKey  = CFSTR("SIMBLTargetApplications");
const char *OPLibrariesPath         = "/Library/Parasite/Extensions";

const CFStringRef kPSBundleIdentifierKey = CFSTR("BundleIdentifier");
const CFStringRef kPSMinBundleVersionKey = CFSTR("MinBundleVersion");
const CFStringRef kPSMaxBundleVersionKey = CFSTR("MaxBundleVersion");

const CFStringRef kPSCoreFoundationVersionKey = CFSTR("CoreFoundationVersion");
const CFStringRef kPSModeKey                  = CFSTR("Mode");
const CFStringRef kPSAnyValue                 = CFSTR("Any");
const CFStringRef kPSExecutablesKey           = CFSTR("Executables");
const CFStringRef kPSBundlesKey               = CFSTR("Bundles");
const CFStringRef kPSClassesKey               = CFSTR("Classes");

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
    
    // Added Parasite 1.0.0b3
    BLACKLIST(opendirectoryd);
    BLACKLIST(configd);
    BLACKLIST(UserEventAgent);

    // Just in case...
    BLACKLIST(DumpPanic);
    
    // Blacklist developer tools
    BLACKLIST(clang)
    BLACKLIST(git);
    BLACKLIST(xcodebuild);
    BLACKLIST(gcc);
    BLACKLIST(lldb);
    BLACKLIST(codesign);
    BLACKLIST(svn);
    BLACKLIST(com.apple.dt.Xcode.sourcecontrol.Git);
    
    CFBundleRef mainBundle = CFBundleGetMainBundle();
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