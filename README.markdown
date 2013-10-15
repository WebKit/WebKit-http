# Haiku WebKit #

This repository contains the Haiku WebKit port source code.
For more information, please visit the [project's wiki and issue tracker](http://dev.haiku-os.org/)

## Quick build instructions ##

### Requirements ###

- You need a recent version of Haiku with the GCC 4 development tools
- The following dependencies: `CMake, GPerf, ICU, libxml, sqlite3, libxslt, Perl, Python`
- And a fast computer!

Dependencies can be installed (for a gcc2hybrid version) via:

    $ pkgman install cmake gperf sqlite_x86_devel libxml2_x86_devel libxslt_x86_devel icu_x86_devel icu_devel perl python

Or, if you build Haiku from source you can add the packages to your UserBuildConfig:

    AddHaikuImagePackages cmake gperf sqlite_x86_devel libxml2_x86_devel libxslt_devel icu_x86_devel icu_devel perl python ;

Packages for other flavors of Haiku may or may not be available. Use [haikuporter](http://haikuports.org) to build them if needed.

### Building WebKit ###

#### Configuring your build for the first time ####
On a gcc2hybrid Haiku:
    $ PKG_CONFIG_LIBDIR=/boot/system/develop/lib/x86/pkgconfig \
        CC=/boot/system/develop/tools/x86/bin/gcc \
        CXX=/boot/system/develop/tools/x86/bin/g++ \
        Tools/Scripts/build-webkit --haiku --no-webkit2

On other versions:
    $ Tools/Scripts/build-webkit --haiku --no-webkit2

#### Regular build, once configured ####
    $ cd WebKitBuild/Release
    $ make -j4

This will build a release version of WebKit libraries on a quad core cpu.

On a successful build, executables and libraries are generated in the WebKitBuild/Release directory.


### Advanced Build, other targets ###

The following make targets are available:

- libwtf.so - The Web Template Library
- libjavascriptcore.so -  The JavaScriptCore library
- jsc	 - The JavaScriptCore executable shell
- libwebcore.so - The WebCore library
- libwebkit.so - The WebKit library
- HaikuLauncher - A simple browsing test app
- DumpRenderTree - The tree parsing test tool

Example given, this will build the JavaScriptCore library in debug mode:

    $ make libjavascriptcore.so

In some rare cases the build system can be confused, to be sure that everything gets rebuilt from scratch,
you can remove the WebKitBuild/ directory and start over.

There are several cmake variable available to configure the build in various ways.
These can be given to build-webkit using the --cmakearg option, or changed later on
using "cmake -Dvar=value WebKitBuild/Release".

### Speeding up the build with distcc ###

You can set the compiler while calling the configure script:
    $ CC="distcc i586-pc-haiku-gcc" CXX="distcc i586-pc-haiku-g++" build-webkit ...

It is a good idea to set the NUMBER_OF_PROCESSORS environment variable as well
(this will be given to cmake through the -j option). If you don't set it, only
the local CPUs will be counted, leading to a sub-optimal distcc distribution.

distcc will look for a compiler named i586-pc-haiku-gcc. You'll need to adjust
the path on the local machines and the slaves to get that pointing to the gcc4
version. On the local machine, the path must be set for all make invocation.
CMake usually tries to resolve the compiler to an absolute path on the first
time it is called, but this doesn't work when the compiler is called through
distcc.

## Notes ##

cmake is smart enough to detect when a variable has changed and will rebuild everything.
You can keep several generated folders with different settings if you need to switch
between them very often (eg. debug and release builds). Just invoke the build-webkit
script with different settings and different output dirs. You can then run make from each
of these folders.

You can copy a WebPositive binary from your Haiku installation into the
WebKitBuild/Release folder. Launching it will then use the freshly built
libraries instead of the system ones.

This document was last updated October 11, 2013

Authors: Maxime Simon, Alexandre Deckner, Adrien Destugues
