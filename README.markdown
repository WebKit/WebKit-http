# Haiku WebKit #

This repository contains the Haiku WebKit port source code.
For more information, please visit the [project's wiki and issue tracker](http://dev.haiku-os.org/)

## Quick build instructions ##

### Requirements ###

- You need a recent version of Haiku with the GCC 4 development tools
- The following dependencies: `CMake, GPerf, ICU, ICU-devel, Perl, Python`
- And a fast computer!

Dependencies can be installed via:

    $ installoptionalpackage CMake GPerf ICU ICU-devel Curl Perl Python

Or, if you build Haiku from source you can add the packages to your UserBuildConfig:

    AddOptionalHaikuImagePackages CMake GPerf ICU ICU-devel Curl Perl Python ;

Those packages can also be manually downloaded and installed from http://haiku-files.org/files/optional-packages/


### Building WebKit ###

#### Configuring your build for the first time ####
    $ setgcc gcc4
    $ Tools/Scripts/build-webkit --haiku

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
    $ CC="distcc gcc" CXX="distcc g++" build-webkit ...

It is a good idea to set the NUMBER_OF_PROCESSORS environment variable as well
(this will be given to cmake through the -j option). If you don't set it, only
the local CPUs will be counted, leading to a sub-optimal distcc distribution.

## Notes ##

If you switch between Release and Debug build versions, make sure to always
have matching libs in. For example, if you switch to release build, do something
which updates libwebcore.so, it will be placed in generated/release, along with
libwebkit.so and HaikuLauncher, since they depend on it. But it won't update
libjavascriptcore.so, since it didn't change. But you need the matching version.
As long as our build system doesn't take care of that, you need to make sure of
this manually.

cmake is smart enough to detect when a variable has changed and will rebuild everything.
You can keep several generated folders with different settings if you need to switch
between them very often (eg. debug and release builds).

You can copy a WebPositive binary from your Haiku installation into the
WebKitBuild/Release folder. Launching it will then use the freshly built
libraries instead of the system ones.

This document was last updated September 27, 2013

Authors: Maxime Simon, Alexandre Deckner, Adrien Destugues
