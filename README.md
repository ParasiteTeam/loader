# ParasiteLoader.dylib
After the [kext](https://github.com/ParasiteTeam/kext) inserts a LC_LOAD_WEAK_DYLIB command pointing to ParasiteLoader.dylib 
into the MachO header of the currently launching process, dyld will load this library and call `__ParasiteInit`. That function
makes sure it is safe to continue by checking a list of blacklisted processes and then goes on to loop through every bundle in the folder
`/Library/Parasite/Extensions` to see which bundles should be loaded into the process.

# Extension Format
Each extension is a `.bundle` located in `/Library/Parasite/Extensions`. In each bundle is an `Info.plist` file containing metadata
about the creator of the bundle, version, etc. This is where the filters go. Add a key into your `Info.plist` file called `PSFilters` 
of type `dictionary`.

Here are the types of filters that can go into the PSFilters dictionary. You may recognize these from MobileSubstrate filters. That's because
it's the same with a few extensions.


| Key | Type | Description |
|-----|:----:|-------------|
| CoreFoundationVersion | array | An array of one or two `real`s. The first one is the minimum CoreFoundation version of a system this bundle should load on and the second value, if it exists refers to the maximum verison. |
| Bundles | array | An array of bundle identifiers or SIMBL-style bundle entries that if are loaded into the current process will cause the bundle to be loaded, too. |
| Executables | array | List of executable names to load into. This is useful for loading into processes that do not have a bundle identifier |
| Classes | array | An array of zero or more strings of the names of classes that, if present, will cause the bundle to load. |
| Mode | String | `Any` is the only allowed option if this key is present. If `Mode` is not set to `Any` then all of the above filters must match for this bundle to be loaded. |

For more detailed information on the filter format, check out [CydiaSubstrate](http://www.cydiasubstrate.com/inject/darwin/), Parasite supports all of it.

A key difference is that the `Bundles` key not only allows strings of bundle id's but also dictionaries that follow the same format as SIMBL. Example:

| Key | Type | Description |
|-----|:----:|-------------|
| BundleIdentifier | string | identifier of bundle to load into |
| MinBundleVersion | real | seems straightforward enough |
| MaxBundleVersion | real | see above |

Parasite also supports SIMBL bundles themselves, though this feature is experimental and not guaranteeed to work. You should prefer to use extensions that were developed for Parasite over SIMBL alternatives if you can.
