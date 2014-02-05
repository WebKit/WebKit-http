# Haiku WebKit #

This repository contains the Haiku WebKit port source code.
For more information, please visit the [project's wiki and issue tracker](http://dev.haiku-os.org/)

## Quick build instructions ##

### Requirements ###

- You need a recent version of Haiku with the GCC 4 development tools
- The following dependencies: `CMake, GPerf, ICU, libxml, sqlite3, libxslt, Perl, Python, Ruby`
- And a fast computer!

Dependencies can be installed (for a gcc2hybrid version) via:

    $ pkgman install cmake gperf sqlite_x86_devel libxml2_x86_devel libxslt_x86_devel icu_x86_devel icu_devel perl python ruby

Or, if you build Haiku from source you can add the packages to your UserBuildConfig:

    AddHaikuImagePackages cmake gperf sqlite_x86_devel libxml2_x86_devel libxslt_devel icu_x86_devel icu_devel perl python ruby ;

Packages for other flavors of Haiku may or may not be available. Use [haikuporter](http://haikuports.org) to build them if needed.

### Building WebKit ###

#### Configuring your build for the first time ####
On a gcc2hybrid Haiku:
    $ PKG_CONFIG_LIBDIR=/boot/system/develop/lib/x86/pkgconfig \
        CC=gcc-x86 CXX=g++-x86 \
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
    $ CC="distcc gcc-x86" CXX="distcc g++-x86" build-webkit ...

It is a good idea to set the NUMBER\_OF\_PROCESSORS environment variable as well
(this will be given to cmake through the -j option). If you don't set it, only
the local CPUs will be counted, leading to a sub-optimal distcc distribution.

distcc will look for a compiler named gcc-x86 and g++-x86. You'll need to adjust
the path on the slaves to get that pointing to the gcc4 version (the gcc4 compiler
is already visible under this name on the local machine and haiku slaves).
CMake usually tries to resolve the compiler to an absolute path on the first
time it is called, but this doesn't work when the compiler is called through
distcc.

## Testing ##

### Testing the test framework ###
    $ ruby Tools/Scripts/test-webkitruby
    $ perl Tools/Scripts/test-webkitperl
    $ python Tools/Scripts/test-webkitpy

The ruby tests pass (all 2 of them!)
The perl test almost pass: Failed 2/25 test programs. 1/412 subtests failed.
The python testsuite prints a lot of "sem\_init: No space left on device" then crashes.
But before it does, there already are some test failures.

### JSC ###
    $ perl Tools/Scripts/run-javascriptcore-tests --no-build --haiku

Current results:
- 7500 tests are run (some are excluded because of missing features in our Ruby port)
- 4 failure related to parsing dates (ecmascript 3, Date, 15.9.5.6)

### WebKit ###
You will have to install the Ahem font for layout tests to work properly. This
is a font with known-size glyphs that render the same on all platforms. Most of
the characters look like black squares, this is expected and not a bug!
http://www.w3.org/Style/CSS/Test/Fonts/Ahem/

    $ cp LayoutTests/resources/Ahem.ttf /system/non-packaged/data/fonts/

It is also a good idea to enable automated debug reports for DumpRenderTree.
Create the file ~/config/settings/system/debug\_server/settings and add:

    exectuable_actions {
        DumpRenderTree log
    }

The crash reports will be moved from the Desktop to the test result directory
and renamed to the name of the test that triggered the crash. If you don't do
this, you have to manually click the "save report" button, and while the
testsuite waits on that, it may mark one or several tests as "timed out".

You can then run the testsuite:

    $ python Tools/Scripts/run-webkit-tests --platform=haiku --no-build \
        --no-http --no-retry-failures --clobber-old-results \
        --no-new-test-results

The options will prevent the script to try updating DumpRenderTree (it doesn't
know how to do that on Haiku, yet) and to run the HTTP tests (requires apache).
It doesn't retry failed tests, will remove previous results before starting,
and will not generate missing "expected" files in the LayoutTests directory.

A lot of tests are currently failing. The problems are either in the WebKit
code itself, or in the various parts of the test harness, none of which are
actually complete: DumpRenderTree, webkitpy, etc. Some of them are triggering
asserts in WebKit code.

### Others ###

There are more tests, but the build-\* scripts must be working before we can run them.

## Notes ##

cmake is smart enough to detect when a variable has changed and will rebuild everything.
You can keep several generated folders with different settings if you need to switch
between them very often (eg. debug and release builds). Just invoke the build-webkit
script with different settings and different output dirs. You can then run make from each
of these folders.

You can copy a WebPositive binary from your Haiku installation into the
WebKitBuild/Release folder. Launching it will then use the freshly built
libraries instead of the system ones.

This document was last updated November 26, 2013

Authors: Maxime Simon, Alexandre Deckner, Adrien Destugues
