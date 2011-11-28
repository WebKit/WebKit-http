# Copyright (C) 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
# Copyright (C) 2009 Google Inc. All rights reserved.
# Copyright (C) 2011 Research In Motion Limited. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer. 
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution. 
# 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Module to share code to get to WebKit directories.

use strict;
use warnings;
use Config;
use FindBin;
use File::Basename;
use File::Path qw(mkpath rmtree);
use File::Spec;
use File::stat;
use POSIX;
use VCSUtils;

BEGIN {
   use Exporter   ();
   our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
   $VERSION     = 1.00;
   @ISA         = qw(Exporter);
   @EXPORT      = qw(
       &XcodeOptionString
       &XcodeOptionStringNoConfig
       &XcodeOptions
       &baseProductDir
       &blackberryTargetArchitecture
       &chdirWebKit
       &checkFrameworks
       &currentSVNRevision
       &debugSafari
       &passedConfiguration
       &productDir
       &runMacWebKitApp
       &safariPath
       &setConfiguration
       USE_OPEN_COMMAND
   );
   %EXPORT_TAGS = ( );
   @EXPORT_OK   = ();
}

use constant USE_OPEN_COMMAND => 1; # Used in runMacWebKitApp().

our @EXPORT_OK;

my $architecture;
my $numberOfCPUs;
my $baseProductDir;
my @baseProductDirOption;
my $configuration;
my $configurationForVisualStudio;
my $configurationProductDir;
my $sourceDir;
my $currentSVNRevision;
my $osXVersion;
my $generateDsym;
my $isQt;
my $qmakebin = "qmake"; # Allow override of the qmake binary from $PATH
my $isGtk;
my $isWinCE;
my $isWinCairo;
my $isWx;
my $isEfl;
my @wxArgs;
my $isBlackBerry;
my $isChromium;
my $isChromiumAndroid;
my $isChromiumMacMake;
my $forceChromiumUpdate;
my $isInspectorFrontend;
my $isWK2;

# Variables for Win32 support
my $vcBuildPath;
my $windowsSourceDir;
my $winVersion;
my $willUseVCExpressWhenBuilding = 0;

# Defined in VCSUtils.
sub exitStatus($);

sub determineSourceDir
{
    return if $sourceDir;
    $sourceDir = $FindBin::Bin;
    $sourceDir =~ s|/+$||; # Remove trailing '/' as we would die later

    # walks up path checking each directory to see if it is the main WebKit project dir, 
    # defined by containing Sources, WebCore, and WebKit
    until ((-d "$sourceDir/Source" && -d "$sourceDir/Source/WebCore" && -d "$sourceDir/Source/WebKit") || (-d "$sourceDir/Internal" && -d "$sourceDir/OpenSource"))
    {
        if ($sourceDir !~ s|/[^/]+$||) {
            die "Could not find top level webkit directory above source directory using FindBin.\n";
        }
    }

    $sourceDir = "$sourceDir/OpenSource" if -d "$sourceDir/OpenSource";
}

sub currentPerlPath()
{
    my $thisPerl = $^X;
    if ($^O ne 'VMS') {
        $thisPerl .= $Config{_exe} unless $thisPerl =~ m/$Config{_exe}$/i;
    }
    return $thisPerl;
}

sub setQmakeBinaryPath($)
{
    ($qmakebin) = @_;
}

# used for scripts which are stored in a non-standard location
sub setSourceDir($)
{
    ($sourceDir) = @_;
}

sub determineBaseProductDir
{
    return if defined $baseProductDir;
    determineSourceDir();

    $baseProductDir = $ENV{"WEBKITOUTPUTDIR"};

    if (!defined($baseProductDir) and isAppleMacWebKit()) {
        # Silently remove ~/Library/Preferences/xcodebuild.plist which can
        # cause build failure. The presence of
        # ~/Library/Preferences/xcodebuild.plist can prevent xcodebuild from
        # respecting global settings such as a custom build products directory
        # (<rdar://problem/5585899>).
        my $personalPlistFile = $ENV{HOME} . "/Library/Preferences/xcodebuild.plist";
        if (-e $personalPlistFile) {
            unlink($personalPlistFile) || die "Could not delete $personalPlistFile: $!";
        }

        my $xcodebuildVersionOutput = `xcodebuild -version`;
        my $xcodeVersion = ($xcodebuildVersionOutput =~ /Xcode ([0-9](\.[0-9]+)*)/) ? $1 : "3.0";
        my $xcodeDefaultsDomain = (eval "v$xcodeVersion" lt v4) ? "com.apple.Xcode" : "com.apple.dt.Xcode";
        my $xcodeDefaultsPrefix = (eval "v$xcodeVersion" lt v4) ? "PBX" : "IDE";

        open PRODUCT, "defaults read $xcodeDefaultsDomain ${xcodeDefaultsPrefix}ApplicationwideBuildSettings 2> " . File::Spec->devnull() . " |" or die;
        $baseProductDir = join '', <PRODUCT>;
        close PRODUCT;

        $baseProductDir = $1 if $baseProductDir =~ /SYMROOT\s*=\s*\"(.*?)\";/s;
        undef $baseProductDir unless $baseProductDir =~ /^\//;
    } elsif (isChromium()) {
        if (isLinux() || isChromiumAndroid() || isChromiumMacMake()) {
            $baseProductDir = "$sourceDir/out";
        } elsif (isDarwin()) {
            $baseProductDir = "$sourceDir/Source/WebKit/chromium/xcodebuild";
        } elsif (isWindows() || isCygwin()) {
            $baseProductDir = "$sourceDir/Source/WebKit/chromium/build";
        }
    }

    if (!defined($baseProductDir)) { # Port-spesific checks failed, use default
        $baseProductDir = "$sourceDir/WebKitBuild";
    }

    if (isBlackBerry()) {
        my %archInfo = blackberryTargetArchitecture();
        $baseProductDir = "$baseProductDir/" . $archInfo{"cpuDir"};
    }

    if (isGit() && isGitBranchBuild() && !isChromium()) {
        my $branch = gitBranch();
        $baseProductDir = "$baseProductDir/$branch";
    }

    if (isAppleMacWebKit()) {
        $baseProductDir =~ s|^\Q$(SRCROOT)/..\E$|$sourceDir|;
        $baseProductDir =~ s|^\Q$(SRCROOT)/../|$sourceDir/|;
        $baseProductDir =~ s|^~/|$ENV{HOME}/|;
        die "Can't handle Xcode product directory with a ~ in it.\n" if $baseProductDir =~ /~/;
        die "Can't handle Xcode product directory with a variable in it.\n" if $baseProductDir =~ /\$/;
        @baseProductDirOption = ("SYMROOT=$baseProductDir", "OBJROOT=$baseProductDir");
    }

    if (isCygwin()) {
        my $dosBuildPath = `cygpath --windows \"$baseProductDir\"`;
        chomp $dosBuildPath;
        $ENV{"WEBKITOUTPUTDIR"} = $dosBuildPath;
        my $unixBuildPath = `cygpath --unix \"$baseProductDir\"`;
        chomp $unixBuildPath;
        $baseProductDir = $unixBuildPath;
    }
}

sub setBaseProductDir($)
{
    ($baseProductDir) = @_;
}

sub determineConfiguration
{
    return if defined $configuration;
    determineBaseProductDir();
    if (open CONFIGURATION, "$baseProductDir/Configuration") {
        $configuration = <CONFIGURATION>;
        close CONFIGURATION;
    }
    if ($configuration) {
        chomp $configuration;
        # compatibility for people who have old Configuration files
        $configuration = "Release" if $configuration eq "Deployment";
        $configuration = "Debug" if $configuration eq "Development";
    } else {
        $configuration = "Release";
    }

    if ($configuration && isWinCairo()) {
        unless ($configuration =~ /_Cairo_CFLite$/) {
            $configuration .= "_Cairo_CFLite";
        }
    }
}

sub determineArchitecture
{
    return if defined $architecture;
    # make sure $architecture is defined in all cases
    $architecture = "";

    determineBaseProductDir();

    if (isGtk()) {
        determineConfigurationProductDir();
        my $host_triple = `grep -E '^host = ' $configurationProductDir/GNUmakefile`;
        if ($host_triple =~ m/^host = ([^-]+)-/) {
            # We have a configured build tree; use it.
            $architecture = $1;
        } else {
            # Fall back to output of `arch', if it is present.
            $architecture = `arch`;
            chomp $architecture;
        }
    } elsif (isAppleMacWebKit()) {
        if (open ARCHITECTURE, "$baseProductDir/Architecture") {
            $architecture = <ARCHITECTURE>;
            close ARCHITECTURE;
        }
        if ($architecture) {
            chomp $architecture;
        } else {
            if (isLeopard()) {
                $architecture = `arch`;
            } else {
                my $supports64Bit = `sysctl -n hw.optional.x86_64`;
                chomp $supports64Bit;
                $architecture = $supports64Bit ? 'x86_64' : `arch`;
            }
            chomp $architecture;
        }
    }
}

sub determineNumberOfCPUs
{
    return if defined $numberOfCPUs;
    if (isLinux()) {
        # First try the nproc utility, if it exists. If we get no
        # results fall back to just interpretting /proc directly.
        chomp($numberOfCPUs = `nproc 2> /dev/null`);
        if ($numberOfCPUs eq "") {
            $numberOfCPUs = (grep /processor/, `cat /proc/cpuinfo`);
        }
    } elsif (isWindows() || isCygwin()) {
        if (defined($ENV{NUMBER_OF_PROCESSORS})) {
            $numberOfCPUs = $ENV{NUMBER_OF_PROCESSORS};
        } else {
            # Assumes cygwin
            $numberOfCPUs = `ls /proc/registry/HKEY_LOCAL_MACHINE/HARDWARE/DESCRIPTION/System/CentralProcessor | wc -w`;
        }
    } elsif (isDarwin()) {
        chomp($numberOfCPUs = `sysctl -n hw.ncpu`);
    }
}

sub jscPath($)
{
    my ($productDir) = @_;
    my $jscName = "jsc";
    $jscName .= "_debug"  if configurationForVisualStudio() eq "Debug_All";
    $jscName .= ".exe" if (isWindows() || isCygwin());
    return "$productDir/$jscName" if -e "$productDir/$jscName";
    return "$productDir/JavaScriptCore.framework/Resources/$jscName";
}

sub argumentsForConfiguration()
{
    determineConfiguration();
    determineArchitecture();

    my @args = ();
    push(@args, '--debug') if $configuration eq "Debug";
    push(@args, '--release') if $configuration eq "Release";
    push(@args, '--32-bit') if $architecture ne "x86_64";
    push(@args, '--qt') if isQt();
    push(@args, '--gtk') if isGtk();
    push(@args, '--efl') if isEfl();
    push(@args, '--wincairo') if isWinCairo();
    push(@args, '--wince') if isWinCE();
    push(@args, '--wx') if isWx();
    push(@args, '--blackberry') if isBlackBerry();
    push(@args, '--chromium') if isChromium() && !isChromiumAndroid();
    push(@args, '--chromium-android') if isChromiumAndroid();
    push(@args, '--inspector-frontend') if isInspectorFrontend();
    return @args;
}

sub determineConfigurationForVisualStudio
{
    return if defined $configurationForVisualStudio;
    determineConfiguration();
    # FIXME: We should detect when Debug_All or Production has been chosen.
    $configurationForVisualStudio = $configuration;
}

sub usesPerConfigurationBuildDirectory
{
    # [Gtk][Efl] We don't have Release/Debug configurations in straight
    # autotool builds (non build-webkit). In this case and if
    # WEBKITOUTPUTDIR exist, use that as our configuration dir. This will
    # allows us to run run-webkit-tests without using build-webkit.
    return ($ENV{"WEBKITOUTPUTDIR"} && (isGtk() || isEfl())) || isAppleWinWebKit();
}

sub determineConfigurationProductDir
{
    return if defined $configurationProductDir;
    determineBaseProductDir();
    determineConfiguration();
    if (isAppleWinWebKit() && !isWx()) {
        $configurationProductDir = File::Spec->catdir($baseProductDir, configurationForVisualStudio(), "bin");
    } else {
        if (usesPerConfigurationBuildDirectory()) {
            $configurationProductDir = "$baseProductDir";
        } else {
            $configurationProductDir = "$baseProductDir/$configuration";
        }
    }
}

sub setConfigurationProductDir($)
{
    ($configurationProductDir) = @_;
}

sub determineCurrentSVNRevision
{
    return if defined $currentSVNRevision;
    determineSourceDir();
    $currentSVNRevision = svnRevisionForDirectory($sourceDir);
    return $currentSVNRevision;
}


sub chdirWebKit
{
    determineSourceDir();
    chdir $sourceDir or die;
}

sub baseProductDir
{
    determineBaseProductDir();
    return $baseProductDir;
}

sub sourceDir
{
    determineSourceDir();
    return $sourceDir;
}

sub productDir
{
    determineConfigurationProductDir();
    return $configurationProductDir;
}

sub jscProductDir
{
    my $productDir = productDir();
    $productDir .= "/bin" if isQt();
    $productDir .= "/Programs" if (isGtk() || isEfl());

    return $productDir;
}

sub configuration()
{
    determineConfiguration();
    return $configuration;
}

sub configurationForVisualStudio()
{
    determineConfigurationForVisualStudio();
    return $configurationForVisualStudio;
}

sub currentSVNRevision
{
    determineCurrentSVNRevision();
    return $currentSVNRevision;
}

sub generateDsym()
{
    determineGenerateDsym();
    return $generateDsym;
}

sub determineGenerateDsym()
{
    return if defined($generateDsym);
    $generateDsym = checkForArgumentAndRemoveFromARGV("--dsym");
}

sub argumentsForXcode()
{
    my @args = ();
    push @args, "DEBUG_INFORMATION_FORMAT=dwarf-with-dsym" if generateDsym();
    return @args;
}

sub XcodeOptions
{
    determineBaseProductDir();
    determineConfiguration();
    determineArchitecture();
    return (@baseProductDirOption, "-configuration", $configuration, "ARCHS=$architecture", argumentsForXcode());
}

sub XcodeOptionString
{
    return join " ", XcodeOptions();
}

sub XcodeOptionStringNoConfig
{
    return join " ", @baseProductDirOption;
}

sub XcodeCoverageSupportOptions()
{
    my @coverageSupportOptions = ();
    push @coverageSupportOptions, "GCC_GENERATE_TEST_COVERAGE_FILES=YES";
    push @coverageSupportOptions, "GCC_INSTRUMENT_PROGRAM_FLOW_ARCS=YES";
    push @coverageSupportOptions, "EXTRA_LINK= \$(EXTRA_LINK) -ftest-coverage -fprofile-arcs";
    push @coverageSupportOptions, "OTHER_CFLAGS= \$(OTHER_CFLAGS) -DCOVERAGE -MD";
    push @coverageSupportOptions, "OTHER_LDFLAGS=\$(OTHER_LDFLAGS) -ftest-coverage -fprofile-arcs -lgcov";
    return @coverageSupportOptions;
}

my $passedConfiguration;
my $searchedForPassedConfiguration;
sub determinePassedConfiguration
{
    return if $searchedForPassedConfiguration;
    $searchedForPassedConfiguration = 1;

    for my $i (0 .. $#ARGV) {
        my $opt = $ARGV[$i];
        if ($opt =~ /^--debug$/i || $opt =~ /^--devel/i) {
            splice(@ARGV, $i, 1);
            $passedConfiguration = "Debug";
            $passedConfiguration .= "_Cairo_CFLite" if (isWinCairo() && isCygwin());
            return;
        }
        if ($opt =~ /^--release$/i || $opt =~ /^--deploy/i) {
            splice(@ARGV, $i, 1);
            $passedConfiguration = "Release";
            $passedConfiguration .= "_Cairo_CFLite" if (isWinCairo() && isCygwin());
            return;
        }
        if ($opt =~ /^--profil(e|ing)$/i) {
            splice(@ARGV, $i, 1);
            $passedConfiguration = "Profiling";
            $passedConfiguration .= "_Cairo_CFLite" if (isWinCairo() && isCygwin());
            return;
        }
    }
    $passedConfiguration = undef;
}

sub passedConfiguration
{
    determinePassedConfiguration();
    return $passedConfiguration;
}

sub setConfiguration
{
    setArchitecture();

    if (my $config = shift @_) {
        $configuration = $config;
        return;
    }

    determinePassedConfiguration();
    $configuration = $passedConfiguration if $passedConfiguration;
}


my $passedArchitecture;
my $searchedForPassedArchitecture;
sub determinePassedArchitecture
{
    return if $searchedForPassedArchitecture;
    $searchedForPassedArchitecture = 1;

    for my $i (0 .. $#ARGV) {
        my $opt = $ARGV[$i];
        if ($opt =~ /^--32-bit$/i) {
            splice(@ARGV, $i, 1);
            if (isAppleMacWebKit() || isWx()) {
                $passedArchitecture = `arch`;
                chomp $passedArchitecture;
            }
            return;
        }
    }
    $passedArchitecture = undef;
}

sub passedArchitecture
{
    determinePassedArchitecture();
    return $passedArchitecture;
}

sub architecture()
{
    determineArchitecture();
    return $architecture;
}

sub numberOfCPUs()
{
    determineNumberOfCPUs();
    return $numberOfCPUs;
}

sub setArchitecture
{
    if (my $arch = shift @_) {
        $architecture = $arch;
        return;
    }

    determinePassedArchitecture();
    $architecture = $passedArchitecture if $passedArchitecture;
}


sub safariPathFromSafariBundle
{
    my ($safariBundle) = @_;

    return "$safariBundle/Contents/MacOS/Safari" if isAppleMacWebKit();
    return $safariBundle if isAppleWinWebKit();
}

sub installedSafariPath
{
    my $safariBundle;

    if (isAppleMacWebKit()) {
        $safariBundle = "/Applications/Safari.app";
    } elsif (isAppleWinWebKit()) {
        $safariBundle = readRegistryString("/HKLM/SOFTWARE/Apple Computer, Inc./Safari/InstallDir");
        $safariBundle =~ s/[\r\n]+$//;
        $safariBundle = `cygpath -u '$safariBundle'` if isCygwin();
        $safariBundle =~ s/[\r\n]+$//;
        $safariBundle .= "Safari.exe";
    }

    return safariPathFromSafariBundle($safariBundle);
}

# Locate Safari.
sub safariPath
{
    # Use WEBKIT_SAFARI environment variable if present.
    my $safariBundle = $ENV{WEBKIT_SAFARI};
    if (!$safariBundle) {
        determineConfigurationProductDir();
        # Use Safari.app in product directory if present (good for Safari development team).
        if (isAppleMacWebKit() && -d "$configurationProductDir/Safari.app") {
            $safariBundle = "$configurationProductDir/Safari.app";
        } elsif (isAppleWinWebKit()) {
            my $path = "$configurationProductDir/Safari.exe";
            my $debugPath = "$configurationProductDir/Safari_debug.exe";

            if (configurationForVisualStudio() eq "Debug_All" && -x $debugPath) {
                $safariBundle = $debugPath;
            } elsif (-x $path) {
                $safariBundle = $path;
            }
        }
        if (!$safariBundle) {
            return installedSafariPath();
        }
    }
    my $safariPath = safariPathFromSafariBundle($safariBundle);
    die "Can't find executable at $safariPath.\n" if isAppleMacWebKit() && !-x $safariPath;
    return $safariPath;
}

sub builtDylibPathForName
{
    my $libraryName = shift;
    determineConfigurationProductDir();
    if (isChromium()) {
        return "$configurationProductDir/$libraryName";
    }
    if (isBlackBerry()) {
        my $libraryExtension = $libraryName =~ /^WebKit$/i ? ".so" : ".a";
        return "$configurationProductDir/$libraryName/lib" . lc($libraryName) . $libraryExtension;
    }
    if (isQt()) {
        $libraryName = "QtWebKit";
        if (isDarwin() and -d "$configurationProductDir/lib/$libraryName.framework") {
            return "$configurationProductDir/lib/$libraryName.framework/$libraryName";
        } elsif (isDarwin() and -d "$configurationProductDir/lib") {
            return "$configurationProductDir/lib/lib$libraryName.dylib";
        } elsif (isWindows()) {
            if (configuration() eq "Debug") {
                # On Windows, there is a "d" suffix to the library name. See <http://trac.webkit.org/changeset/53924/>.
                $libraryName .= "d";
            }

            my $mkspec = `$qmakebin -query QMAKE_MKSPECS`;
            $mkspec =~ s/[\n|\r]$//g;
            my $qtMajorVersion = retrieveQMakespecVar("$mkspec/qconfig.pri", "QT_MAJOR_VERSION");
            if (not $qtMajorVersion) {
                $qtMajorVersion = "";
            }
            return "$configurationProductDir/lib/$libraryName$qtMajorVersion.dll";
        } else {
            return "$configurationProductDir/lib/lib$libraryName.so";
        }
    }
    if (isWx()) {
        return "$configurationProductDir/libwxwebkit.dylib";
    }
    if (isGtk()) {
        # WebKitGTK+ for GTK2, WebKitGTK+ for GTK3, and WebKit2 respectively.
        my @libraries = ("libwebkitgtk-1.0", "libwebkitgtk-3.0", "libwebkit2gtk-1.0");
        my $extension = isDarwin() ? ".dylib" : ".so";

        foreach $libraryName (@libraries) {
            my $libraryPath = "$configurationProductDir/.libs/" . $libraryName . $extension;
            return $libraryPath if -e $libraryPath;
        }
        return "NotFound";
    }
    if (isEfl()) {
        return "$configurationProductDir/$libraryName/../WebKit/libewebkit.so";
    }
    if (isWinCE()) {
        return "$configurationProductDir/$libraryName";
    }
    if (isAppleMacWebKit()) {
        return "$configurationProductDir/$libraryName.framework/Versions/A/$libraryName";
    }
    if (isAppleWinWebKit()) {
        if ($libraryName eq "JavaScriptCore") {
            return "$baseProductDir/lib/$libraryName.lib";
        } else {
            return "$baseProductDir/$libraryName.intermediate/$configuration/$libraryName.intermediate/$libraryName.lib";
        }
    }

    die "Unsupported platform, can't determine built library locations.\nTry `build-webkit --help` for more information.\n";
}

# Check to see that all the frameworks are built.
sub checkFrameworks # FIXME: This is a poor name since only the Mac calls built WebCore a Framework.
{
    return if isCygwin() || isWindows();
    my @frameworks = ("JavaScriptCore", "WebCore");
    push(@frameworks, "WebKit") if isAppleMacWebKit(); # FIXME: This seems wrong, all ports should have a WebKit these days.
    for my $framework (@frameworks) {
        my $path = builtDylibPathForName($framework);
        die "Can't find built framework at \"$path\".\n" unless -e $path;
    }
}

sub isInspectorFrontend()
{
    determineIsInspectorFrontend();
    return $isInspectorFrontend;
}

sub determineIsInspectorFrontend()
{
    return if defined($isInspectorFrontend);
    $isInspectorFrontend = checkForArgumentAndRemoveFromARGV("--inspector-frontend");
}

sub isQt()
{
    determineIsQt();
    return $isQt;
}

sub getQtVersion()
{
    my $qtVersion = `$qmakebin --version`;
    $qtVersion =~ s/^(.*)Qt version (\d\.\d)(.*)/$2/s ;
    return $qtVersion;
}

sub qtFeatureDefaults
{
    die "ERROR: qmake missing but required to build WebKit.\n" if not commandExists($qmakebin);

    my $qmakepath = File::Spec->catfile(sourceDir(), "Tools", "qmake");
    my $qmakecommand;
    if (isWindows()) {
        $qmakecommand = "(set QMAKEPATH=$qmakepath) && $qmakebin";
    } else {
        $qmakecommand = "QMAKEPATH=$qmakepath $qmakebin";
    }

    my $originalCwd = getcwd();

    my $file;
    my @buildArgs;

    if (@_) {
        @buildArgs = (@buildArgs, @{$_[0]});
        my $dir = File::Spec->catfile(productDir(), "Tools", "qmake");
        File::Path::mkpath($dir);
        chdir $dir or die "Failed to cd into " . $dir . "\n";
        $file = File::Spec->catfile($qmakepath, "configure.pro");
    } else {
        # Do a quick check of the features without running the config tests
        $file = File::Spec->catfile($qmakepath, "mkspecs", "features", "features.prf");
        push @buildArgs, "CONFIG+=compute_defaults";
    }

    my $defaults = `$qmakecommand @buildArgs $file 2>&1`;

    my %qtFeatureDefaults;
    while ($defaults =~ m/(\S+?)=(\S+?)/gi) {
        $qtFeatureDefaults{$1}=$2;
    }

    chdir $originalCwd;

    return %qtFeatureDefaults;
}

sub commandExists($)
{
    my $command = shift;
    my $devnull = File::Spec->devnull();
    return `$command --version 2> $devnull`;
}

sub checkForArgumentAndRemoveFromARGV
{
    my $argToCheck = shift;
    return checkForArgumentAndRemoveFromArrayRef($argToCheck, \@ARGV);
}

sub checkForArgumentAndRemoveFromArrayRef
{
    my ($argToCheck, $arrayRef) = @_;
    my @indicesToRemove;
    foreach my $index (0 .. $#$arrayRef) {
        my $opt = $$arrayRef[$index];
        if ($opt =~ /^$argToCheck$/i ) {
            push(@indicesToRemove, $index);
        }
    }
    foreach my $index (@indicesToRemove) {
        splice(@$arrayRef, $index, 1);
    }
    return $#indicesToRemove > -1;
}

sub isWK2()
{
    if (defined($isWK2)) {
        return $isWK2;
    }
    if (checkForArgumentAndRemoveFromARGV("-2")) {
        $isWK2 = 1;
    } else {
        $isWK2 = 0;
    }
    return $isWK2;
}

sub determineIsQt()
{
    return if defined($isQt);

    # Allow override in case QTDIR is not set.
    if (checkForArgumentAndRemoveFromARGV("--qt")) {
        $isQt = 1;
        return;
    }

    # The presence of QTDIR only means Qt if --gtk or --wx or --efl or --blackberry are not on the command-line
    if (isGtk() || isWx() || isEfl() || isBlackBerry()) {
        $isQt = 0;
        return;
    }
    
    $isQt = defined($ENV{'QTDIR'});
}

sub isBlackBerry()
{
    determineIsBlackBerry();
    return $isBlackBerry;
}

sub determineIsBlackBerry()
{
    return if defined($isBlackBerry);
    $isBlackBerry = checkForArgumentAndRemoveFromARGV("--blackberry");
}

sub blackberryTargetArchitecture()
{
    my $arch = $ENV{"BLACKBERRY_ARCH_TYPE"} ? $ENV{"BLACKBERRY_ARCH_TYPE"} : "arm";
    my $cpu = $ENV{"BLACKBERRY_ARCH_CPU"} ? $ENV{"BLACKBERRY_ARCH_CPU"} : "";
    my $cpuDir;
    my $buSuffix;
    if (($cpu eq "v7le") || ($cpu eq "a9")) {
        $cpuDir = $arch . "le-v7";
        $buSuffix = $arch . "v7";
    } else {
        $cpu = $arch;
        $cpuDir = $arch;
        $buSuffix = $arch;
    }
    return ("arch" => $arch,
            "cpu" => $cpu,
            "cpuDir" => $cpuDir,
            "buSuffix" => $buSuffix);
}

sub determineIsEfl()
{
    return if defined($isEfl);
    $isEfl = checkForArgumentAndRemoveFromARGV("--efl");
}

sub isEfl()
{
    determineIsEfl();
    return $isEfl;
}

sub isGtk()
{
    determineIsGtk();
    return $isGtk;
}

sub determineIsGtk()
{
    return if defined($isGtk);
    $isGtk = checkForArgumentAndRemoveFromARGV("--gtk");
}

sub isWinCE()
{
    determineIsWinCE();
    return $isWinCE;
}

sub determineIsWinCE()
{
    return if defined($isWinCE);
    $isWinCE = checkForArgumentAndRemoveFromARGV("--wince");
}

sub isWx()
{
    determineIsWx();
    return $isWx;
}

sub determineIsWx()
{
    return if defined($isWx);
    $isWx = checkForArgumentAndRemoveFromARGV("--wx");
}

sub getWxArgs()
{
    if (!@wxArgs) {
        @wxArgs = ("");
        my $rawWxArgs = "";
        foreach my $opt (@ARGV) {
            if ($opt =~ /^--wx-args/i ) {
                @ARGV = grep(!/^--wx-args/i, @ARGV);
                $rawWxArgs = $opt;
                $rawWxArgs =~ s/--wx-args=//i;
            }
        }
        @wxArgs = split(/,/, $rawWxArgs);
    }
    return @wxArgs;
}

# Determine if this is debian, ubuntu, linspire, or something similar.
sub isDebianBased()
{
    return -e "/etc/debian_version";
}

sub isFedoraBased()
{
    return -e "/etc/fedora-release";
}

sub isChromium()
{
    determineIsChromium();
    determineIsChromiumAndroid();
    return $isChromium || $isChromiumAndroid;
}

sub determineIsChromium()
{
    return if defined($isChromium);
    $isChromium = checkForArgumentAndRemoveFromARGV("--chromium");
    if ($isChromium) {
        $forceChromiumUpdate = checkForArgumentAndRemoveFromARGV("--force-update");
    }
}

sub isChromiumAndroid()
{
    determineIsChromiumAndroid();
    return $isChromiumAndroid;
}

sub determineIsChromiumAndroid()
{
    return if defined($isChromiumAndroid);
    $isChromiumAndroid = checkForArgumentAndRemoveFromARGV("--chromium-android");
}

sub isChromiumMacMake()
{
    determineIsChromiumMacMake();
    return $isChromiumMacMake;
}

sub determineIsChromiumMacMake()
{
    return if defined($isChromiumMacMake);

    my $hasUpToDateMakefile = 0;
    if (-e 'Makefile.chromium') {
        unless (-e 'Source/WebKit/chromium/WebKit.xcodeproj') {
            $hasUpToDateMakefile = 1;
        } else {
            $hasUpToDateMakefile = stat('Makefile.chromium')->mtime > stat('Source/WebKit/chromium/WebKit.xcodeproj')->mtime;
        }
    }
    $isChromiumMacMake = isDarwin() && $hasUpToDateMakefile;
}

sub forceChromiumUpdate()
{
    determineIsChromium();
    return $forceChromiumUpdate;
}

sub isWinCairo()
{
    determineIsWinCairo();
    return $isWinCairo;
}

sub determineIsWinCairo()
{
    return if defined($isWinCairo);
    $isWinCairo = checkForArgumentAndRemoveFromARGV("--wincairo");
}

sub isCygwin()
{
    return ($^O eq "cygwin") || 0;
}

sub isAnyWindows()
{
    return isWindows() || isCygwin() || isMsys();
}

sub determineWinVersion()
{
    return if $winVersion;

    if (!isAnyWindows()) {
        $winVersion = -1;
        return;
    }

    my $versionString = `cmd /c ver`;
    $versionString =~ /(\d)\.(\d)\.(\d+)/;

    $winVersion = {
        major => $1,
        minor => $2,
        build => $3,
    };
}

sub winVersion()
{
    determineWinVersion();
    return $winVersion;
}

sub isWindows7SP0()
{
    return isAnyWindows() && winVersion()->{major} == 6 && winVersion()->{minor} == 1 && winVersion()->{build} == 7600;
}

sub isWindowsVista()
{
    return isAnyWindows() && winVersion()->{major} == 6 && winVersion()->{minor} == 0;
}

sub isWindowsXP()
{
    return isAnyWindows() && winVersion()->{major} == 5 && winVersion()->{minor} == 1;
}

sub isDarwin()
{
    return ($^O eq "darwin") || 0;
}

sub isWindows()
{
    return ($^O eq "MSWin32") || 0;
}

sub isMsys()
{
    return ($^O eq "msys") || 0;
}

sub isLinux()
{
    return ($^O eq "linux") || 0;
}

sub isARM()
{
    return $Config{archname} =~ /^arm-/;
}

sub isAppleWebKit()
{
    return !(isQt() or isGtk() or isWx() or isChromium() or isEfl() or isWinCE() or isBlackBerry());
}

sub isAppleMacWebKit()
{
    return isAppleWebKit() && isDarwin();
}

sub isAppleWinWebKit()
{
    return isAppleWebKit() && (isCygwin() || isWindows());
}

sub isPerianInstalled()
{
    if (!isAppleWebKit()) {
        return 0;
    }

    if (-d "/Library/QuickTime/Perian.component") {
        return 1;
    }

    if (-d "$ENV{HOME}/Library/QuickTime/Perian.component") {
        return 1;
    }

    return 0;
}

sub determineOSXVersion()
{
    return if $osXVersion;

    if (!isDarwin()) {
        $osXVersion = -1;
        return;
    }

    my $version = `sw_vers -productVersion`;
    my @splitVersion = split(/\./, $version);
    @splitVersion >= 2 or die "Invalid version $version";
    $osXVersion = {
            "major" => $splitVersion[0],
            "minor" => $splitVersion[1],
            "subminor" => (defined($splitVersion[2]) ? $splitVersion[2] : 0),
    };
}

sub osXVersion()
{
    determineOSXVersion();
    return $osXVersion;
}

sub isLeopard()
{
    return isDarwin() && osXVersion()->{"minor"} == 5;
}

sub isSnowLeopard()
{
    return isDarwin() && osXVersion()->{"minor"} == 6;
}

sub isLion()
{
    return isDarwin() && osXVersion()->{"minor"} == 7;
}

sub isWindowsNT()
{
    return $ENV{'OS'} eq 'Windows_NT';
}

sub relativeScriptsDir()
{
    my $scriptDir = File::Spec->catpath("", File::Spec->abs2rel($FindBin::Bin, getcwd()), "");
    if ($scriptDir eq "") {
        $scriptDir = ".";
    }
    return $scriptDir;
}

sub launcherPath()
{
    my $relativeScriptsPath = relativeScriptsDir();
    if (isGtk() || isQt() || isWx() || isEfl() || isWinCE()) {
        return "$relativeScriptsPath/run-launcher";
    } elsif (isAppleWebKit()) {
        return "$relativeScriptsPath/run-safari";
    }
}

sub launcherName()
{
    if (isGtk()) {
        return "GtkLauncher";
    } elsif (isQt()) {
        return "QtTestBrowser";
    } elsif (isWx()) {
        return "wxBrowser";
    } elsif (isAppleWebKit()) {
        return "Safari";
    } elsif (isEfl()) {
        return "EWebLauncher";
    } elsif (isWinCE()) {
        return "WinCELauncher";
    }
}

sub checkRequiredSystemConfig
{
    if (isDarwin()) {
        chomp(my $productVersion = `sw_vers -productVersion`);
        if (eval "v$productVersion" lt v10.4) {
            print "*************************************************************\n";
            print "Mac OS X Version 10.4.0 or later is required to build WebKit.\n";
            print "You have " . $productVersion . ", thus the build will most likely fail.\n";
            print "*************************************************************\n";
        }
        my $xcodebuildVersionOutput = `xcodebuild -version`;
        my $devToolsCoreVersion = ($xcodebuildVersionOutput =~ /DevToolsCore-(\d+)/) ? $1 : undef;
        my $xcodeVersion = ($xcodebuildVersionOutput =~ /Xcode ([0-9](\.[0-9]+)*)/) ? $1 : undef;
        if (!$devToolsCoreVersion && !$xcodeVersion
            || $devToolsCoreVersion && $devToolsCoreVersion < 747
            || $xcodeVersion && eval "v$xcodeVersion" lt v2.3) {
            print "*************************************************************\n";
            print "Xcode Version 2.3 or later is required to build WebKit.\n";
            print "You have an earlier version of Xcode, thus the build will\n";
            print "most likely fail.  The latest Xcode is available from the web:\n";
            print "http://developer.apple.com/tools/xcode\n";
            print "*************************************************************\n";
        }
    } elsif (isGtk() or isQt() or isWx() or isEfl()) {
        my @cmds = qw(flex bison gperf);
        my @missing = ();
        foreach my $cmd (@cmds) {
            push @missing, $cmd if not commandExists($cmd);
        }

        if (@missing) {
            my $list = join ", ", @missing;
            die "ERROR: $list missing but required to build WebKit.\n";
        }
    }
    # Win32 and other platforms may want to check for minimum config
}

sub determineWindowsSourceDir()
{
    return if $windowsSourceDir;
    $windowsSourceDir = sourceDir();
    chomp($windowsSourceDir = `cygpath -w '$windowsSourceDir'`) if isCygwin();
}

sub windowsSourceDir()
{
    determineWindowsSourceDir();
    return $windowsSourceDir;
}

sub windowsLibrariesDir()
{
    return windowsSourceDir() . "\\WebKitLibraries\\win";
}

sub windowsOutputDir()
{
    return windowsSourceDir() . "\\WebKitBuild";
}

sub setupAppleWinEnv()
{
    return unless isAppleWinWebKit();

    if (isWindowsNT()) {
        my $restartNeeded = 0;
        my %variablesToSet = ();

        # Setting the environment variable 'CYGWIN' to 'tty' makes cygwin enable extra support (i.e., termios)
        # for UNIX-like ttys in the Windows console
        $variablesToSet{CYGWIN} = "tty" unless $ENV{CYGWIN};
        
        # Those environment variables must be set to be able to build inside Visual Studio.
        $variablesToSet{WEBKITLIBRARIESDIR} = windowsLibrariesDir() unless $ENV{WEBKITLIBRARIESDIR};
        $variablesToSet{WEBKITOUTPUTDIR} = windowsOutputDir() unless $ENV{WEBKITOUTPUTDIR};

        foreach my $variable (keys %variablesToSet) {
            print "Setting the Environment Variable '" . $variable . "' to '" . $variablesToSet{$variable} . "'\n\n";
            system qw(regtool -s set), '\\HKEY_CURRENT_USER\\Environment\\' . $variable, $variablesToSet{$variable};
            $restartNeeded ||= $variable eq "WEBKITLIBRARIESDIR" || $variable eq "WEBKITOUTPUTDIR";
        }

        if ($restartNeeded) {
            print "Please restart your computer before attempting to build inside Visual Studio.\n\n";
        }
    } else {
        if (!$ENV{'WEBKITLIBRARIESDIR'}) {
            print "Warning: You must set the 'WebKitLibrariesDir' environment variable\n";
            print "         to be able build WebKit from within Visual Studio.\n";
            print "         Make sure that 'WebKitLibrariesDir' points to the\n";
            print "         'WebKitLibraries/win' directory, not the 'WebKitLibraries/' directory.\n\n";
        }
        if (!$ENV{'WEBKITOUTPUTDIR'}) {
            print "Warning: You must set the 'WebKitOutputDir' environment variable\n";
            print "         to be able build WebKit from within Visual Studio.\n\n";
        }
    }
}

sub setupCygwinEnv()
{
    return if !isCygwin() && !isWindows();
    return if $vcBuildPath;

    my $vsInstallDir;
    my $programFilesPath = $ENV{'PROGRAMFILES(X86)'} || $ENV{'PROGRAMFILES'} || "C:\\Program Files";
    if ($ENV{'VSINSTALLDIR'}) {
        $vsInstallDir = $ENV{'VSINSTALLDIR'};
    } else {
        $vsInstallDir = File::Spec->catdir($programFilesPath, "Microsoft Visual Studio 8");
    }
    chomp($vsInstallDir = `cygpath "$vsInstallDir"`) if isCygwin();
    $vcBuildPath = File::Spec->catfile($vsInstallDir, qw(Common7 IDE devenv.com));
    if (-e $vcBuildPath) {
        # Visual Studio is installed; we can use pdevenv to build.
        # FIXME: Make pdevenv work with non-Cygwin Perl.
        $vcBuildPath = File::Spec->catfile(sourceDir(), qw(Tools Scripts pdevenv)) if isCygwin();
    } else {
        # Visual Studio not found, try VC++ Express
        $vcBuildPath = File::Spec->catfile($vsInstallDir, qw(Common7 IDE VCExpress.exe));
        if (! -e $vcBuildPath) {
            print "*************************************************************\n";
            print "Cannot find '$vcBuildPath'\n";
            print "Please execute the file 'vcvars32.bat' from\n";
            print "'$programFilesPath\\Microsoft Visual Studio 8\\VC\\bin\\'\n";
            print "to setup the necessary environment variables.\n";
            print "*************************************************************\n";
            die;
        }
        $willUseVCExpressWhenBuilding = 1;
    }

    my $qtSDKPath = File::Spec->catdir($programFilesPath, "QuickTime SDK");
    if (0 && ! -e $qtSDKPath) {
        print "*************************************************************\n";
        print "Cannot find '$qtSDKPath'\n";
        print "Please download the QuickTime SDK for Windows from\n";
        print "http://developer.apple.com/quicktime/download/\n";
        print "*************************************************************\n";
        die;
    }
    
    unless ($ENV{WEBKITLIBRARIESDIR}) {
        $ENV{'WEBKITLIBRARIESDIR'} = File::Spec->catdir($sourceDir, "WebKitLibraries", "win");
        chomp($ENV{WEBKITLIBRARIESDIR} = `cygpath -wa '$ENV{WEBKITLIBRARIESDIR}'`) if isCygwin();
    }

    print "Building results into: ", baseProductDir(), "\n";
    print "WEBKITOUTPUTDIR is set to: ", $ENV{"WEBKITOUTPUTDIR"}, "\n";
    print "WEBKITLIBRARIESDIR is set to: ", $ENV{"WEBKITLIBRARIESDIR"}, "\n";
}

sub dieIfWindowsPlatformSDKNotInstalled
{
    my $registry32Path = "/proc/registry/";
    my $registry64Path = "/proc/registry64/";
    my $windowsPlatformSDKRegistryEntry = "HKEY_LOCAL_MACHINE/SOFTWARE/Microsoft/MicrosoftSDK/InstalledSDKs/D2FF9F89-8AA2-4373-8A31-C838BF4DBBE1";

    # FIXME: It would be better to detect whether we are using 32- or 64-bit Windows
    # and only check the appropriate entry. But for now we just blindly check both.
    return if (-e $registry32Path . $windowsPlatformSDKRegistryEntry) || (-e $registry64Path . $windowsPlatformSDKRegistryEntry);

    print "*************************************************************\n";
    print "Cannot find registry entry '$windowsPlatformSDKRegistryEntry'.\n";
    print "Please download and install the Microsoft Windows Server 2003 R2\n";
    print "Platform SDK from <http://www.microsoft.com/downloads/details.aspx?\n";
    print "familyid=0baf2b35-c656-4969-ace8-e4c0c0716adb&displaylang=en>.\n\n";
    print "Then follow step 2 in the Windows section of the \"Installing Developer\n";
    print "Tools\" instructions at <http://www.webkit.org/building/tools.html>.\n";
    print "*************************************************************\n";
    die;
}

sub copyInspectorFrontendFiles
{
    my $productDir = productDir();
    my $sourceInspectorPath = sourceDir() . "/Source/WebCore/inspector/front-end/";
    my $inspectorResourcesDirPath = $ENV{"WEBKITINSPECTORRESOURCESDIR"};

    if (!defined($inspectorResourcesDirPath)) {
        $inspectorResourcesDirPath = "";
    }

    if (isAppleMacWebKit()) {
        $inspectorResourcesDirPath = $productDir . "/WebCore.framework/Resources/inspector";
    } elsif (isAppleWinWebKit()) {
        $inspectorResourcesDirPath = $productDir . "/WebKit.resources/inspector";
    } elsif (isQt() || isGtk()) {
        my $prefix = $ENV{"WebKitInstallationPrefix"};
        $inspectorResourcesDirPath = (defined($prefix) ? $prefix : "/usr/share") . "/webkit-1.0/webinspector";
    } elsif (isEfl()) {
        my $prefix = $ENV{"WebKitInstallationPrefix"};
        $inspectorResourcesDirPath = (defined($prefix) ? $prefix : "/usr/share") . "/ewebkit/webinspector";
    }

    if (! -d $inspectorResourcesDirPath) {
        print "*************************************************************\n";
        print "Cannot find '$inspectorResourcesDirPath'.\n" if (defined($inspectorResourcesDirPath));
        print "Make sure that you have built WebKit first.\n" if (! -d $productDir || defined($inspectorResourcesDirPath));
        print "Optionally, set the environment variable 'WebKitInspectorResourcesDir'\n";
        print "to point to the directory that contains the WebKit Inspector front-end\n";
        print "files for the built WebCore framework.\n";
        print "*************************************************************\n";
        die;
    }

    if (isAppleMacWebKit()) {
        my $sourceLocalizedStrings = sourceDir() . "/Source/WebCore/English.lproj/localizedStrings.js";
        my $destinationLocalizedStrings = $productDir . "/WebCore.framework/Resources/English.lproj/localizedStrings.js";
        system "ditto", $sourceLocalizedStrings, $destinationLocalizedStrings;
    }

    return system "rsync", "-aut", "--exclude=/.DS_Store", "--exclude=*.re2js", "--exclude=.svn/", !isQt() ? "--exclude=/WebKit.qrc" : "", $sourceInspectorPath, $inspectorResourcesDirPath;
}

sub buildXCodeProject($$@)
{
    my ($project, $clean, @extraOptions) = @_;

    if ($clean) {
        push(@extraOptions, "-alltargets");
        push(@extraOptions, "clean");
    }

    return system "xcodebuild", "-project", "$project.xcodeproj", @extraOptions;
}

sub usingVisualStudioExpress()
{
    setupCygwinEnv();
    return $willUseVCExpressWhenBuilding;
}

sub buildVisualStudioProject
{
    my ($project, $clean) = @_;
    setupCygwinEnv();

    my $config = configurationForVisualStudio();

    dieIfWindowsPlatformSDKNotInstalled() if $willUseVCExpressWhenBuilding;

    chomp($project = `cygpath -w "$project"`) if isCygwin();
    
    my $action = "/build";
    if ($clean) {
        $action = "/clean";
    }

    my @command = ($vcBuildPath, $project, $action, $config);

    print join(" ", @command), "\n";
    return system @command;
}

sub downloadWafIfNeeded
{
    # get / update waf if needed
    my $waf = "$sourceDir/Tools/waf/waf";
    my $wafURL = 'http://wxwebkit.kosoftworks.com/downloads/deps/waf';
    if (!-f $waf) {
        my $result = system "curl -o $waf $wafURL";
        chmod 0755, $waf;
    }
}

sub buildWafProject
{
    my ($project, $shouldClean, @options) = @_;
    
    # set the PYTHONPATH for waf
    my $pythonPath = $ENV{'PYTHONPATH'};
    if (!defined($pythonPath)) {
        $pythonPath = '';
    }
    my $sourceDir = sourceDir();
    my $newPythonPath = "$sourceDir/Tools/waf/build:$pythonPath";
    if (isCygwin()) {
        $newPythonPath = `cygpath --mixed --path $newPythonPath`;
    }
    $ENV{'PYTHONPATH'} = $newPythonPath;
    
    print "Building $project\n";

    my $wafCommand = "$sourceDir/Tools/waf/waf";
    if ($ENV{'WXWEBKIT_WAF'}) {
        $wafCommand = $ENV{'WXWEBKIT_WAF'};
    }
    if (isCygwin()) {
        $wafCommand = `cygpath --windows "$wafCommand"`;
        chomp($wafCommand);
    }
    if ($shouldClean) {
        return system $wafCommand, "uninstall", "clean", "distclean";
    }
    
    return system $wafCommand, 'configure', 'build', 'install', @options;
}

sub retrieveQMakespecVar
{
    my $mkspec = $_[0];
    my $varname = $_[1];

    my $varvalue = undef;
    #print "retrieveMakespecVar " . $mkspec . ", " . $varname . "\n";

    local *SPEC;
    open SPEC, "<$mkspec" or return $varvalue;
    while (<SPEC>) {
        if ($_ =~ /\s*include\((.+)\)/) {
            # open the included mkspec
            my $oldcwd = getcwd();
            (my $volume, my $directories, my $file) = File::Spec->splitpath($mkspec);
            my $newcwd = "$volume$directories";
            chdir $newcwd if $newcwd;
            $varvalue = retrieveQMakespecVar($1, $varname);
            chdir $oldcwd;
        } elsif ($_ =~ /$varname\s*=\s*([^\s]+)/) {
            $varvalue = $1;
            last;
        }
    }
    close SPEC;
    return $varvalue;
}

sub qtMakeCommand($)
{
    my ($qmakebin) = @_;
    chomp(my $mkspec = `$qmakebin -query QMAKE_MKSPECS`);
    $mkspec .= "/default";
    my $compiler = retrieveQMakespecVar("$mkspec/qmake.conf", "QMAKE_CC");

    #print "default spec: " . $mkspec . "\n";
    #print "compiler found: " . $compiler . "\n";

    if ($compiler && $compiler eq "cl") {
        return "nmake";
    }

    return "make";
}

sub autotoolsFlag($$)
{
    my ($flag, $feature) = @_;
    my $prefix = $flag ? "--enable" : "--disable";

    return $prefix . '-' . $feature;
}

sub runAutogenForAutotoolsProject($@)
{
    my ($dir, $prefix, $sourceDir, $saveArguments, $argumentsFile, @buildArgs) = @_;

    print "Calling autogen.sh in " . $dir . "\n\n";
    print "Installation prefix directory: $prefix\n" if(defined($prefix));

    if ($saveArguments) {
        # Write autogen.sh arguments to a file so that we can detect
        # when they change and automatically re-run it.
        open(AUTOTOOLS_ARGUMENTS, ">$argumentsFile");
        print AUTOTOOLS_ARGUMENTS join(" ", @buildArgs);
        close(AUTOTOOLS_ARGUMENTS);
    }

    # Make the path relative since it will appear in all -I compiler flags.
    # Long argument lists cause bizarre slowdowns in libtool.
    my $relSourceDir = File::Spec->abs2rel($sourceDir) || ".";
    if (system("$relSourceDir/autogen.sh", @buildArgs) ne 0) {
        die "Calling autogen.sh failed!\n";
    }
}

sub autogenArgumentsHaveChanged($@)
{
    my ($filename, @currentArguments) = @_;

    if (! -e $filename) {
        return 1;
    }

    open(AUTOTOOLS_ARGUMENTS, $filename);
    chomp(my $previousArguments = <AUTOTOOLS_ARGUMENTS>);
    close(AUTOTOOLS_ARGUMENTS);

    my $joinedCurrentArguments = join(" ", @currentArguments);
    if ($previousArguments ne $joinedCurrentArguments) {
        print "Previous autogen arguments were: $previousArguments\n\n";
        print "New autogen arguments are: $joinedCurrentArguments\n";
        return 1;
    }

    return 0;
}

sub buildAutotoolsProject($@)
{
    my ($project, $clean, $enableWebKit2, @buildParams) = @_;

    my $make = 'make';
    my $dir = productDir();
    my $config = passedConfiguration() || configuration();
    my $prefix;

    my @buildArgs = ();
    my $makeArgs = $ENV{"WebKitMakeArguments"} || "";
    for my $i (0 .. $#buildParams) {
        my $opt = $buildParams[$i];
        if ($opt =~ /^--makeargs=(.*)/i ) {
            $makeArgs = $makeArgs . " " . $1;
        } elsif ($opt =~ /^--prefix=(.*)/i ) {
            $prefix = $1;
        } else {
            push @buildArgs, $opt;
        }
    }

    # Automatically determine the number of CPUs for make only
    # if make arguments haven't already been specified.
    if ($makeArgs eq "") {
        $makeArgs = "-j" . numberOfCPUs();
    }

    # WebKit is the default target, so we don't need to specify anything.
    if ($project eq "JavaScriptCore") {
        $makeArgs .= " jsc";
    }

    # This is a temporary work-around to enable building WebKit2 on the bots,
    # but ensuring that it does not ship until the API is stable.
    if ($project eq "WebKit" and isGtk() and $enableWebKit2) {
        push @buildArgs, "--enable-webkit2";
    }

    $prefix = $ENV{"WebKitInstallationPrefix"} if !defined($prefix);
    push @buildArgs, "--prefix=" . $prefix if defined($prefix);

    # check if configuration is Debug
    if ($config =~ m/debug/i) {
        push @buildArgs, "--enable-debug";
    } else {
        push @buildArgs, "--disable-debug";
    }

    # Use rm to clean the build directory since distclean may miss files
    if ($clean && -d $dir) {
        system "rm", "-rf", "$dir";
    }

    if (! -d $dir) {
        File::Path::mkpath($dir) or die "Failed to create build directory " . $dir
    }
    chdir $dir or die "Failed to cd into " . $dir . "\n";

    if ($clean) {
        return 0;
    }

    # If GNUmakefile exists, don't run autogen.sh unless its arguments
    # have changed. The makefile should be smart enough to track autotools
    # dependencies and re-run autogen.sh when build files change.
    my $autogenArgumentsFile = "previous-autogen-arguments.txt";
    my $saveAutogenArguments = $project eq "WebKit";
    if (!(-e "GNUmakefile")) {
        runAutogenForAutotoolsProject($dir, $prefix, $sourceDir, $saveAutogenArguments, $autogenArgumentsFile, @buildArgs);
    }

    if ($saveAutogenArguments and autogenArgumentsHaveChanged($autogenArgumentsFile, @buildArgs)) {
        runAutogenForAutotoolsProject($dir, $prefix, $sourceDir, $saveAutogenArguments, $autogenArgumentsFile, @buildArgs);
    }

    if (system("$make $makeArgs") ne 0) {
        die "\nFailed to build WebKit using '$make'!\n";
    }

    chdir ".." or die;

    if (isGtk()) {
        my $relativeScriptsPath = relativeScriptsDir();
        if (system("$relativeScriptsPath/../gtk/generate-gtkdoc --skip-html")) {
            die "\n gtkdoc did not build without warnings\n";
        }
    }

    return 0;
}

sub generateBuildSystemFromCMakeProject
{
    my ($port, $prefixPath, @cmakeArgs, $additionalCMakeArgs) = @_;
    my $config = configuration();
    my $buildPath = File::Spec->catdir(baseProductDir(), $config);
    File::Path::mkpath($buildPath) unless -d $buildPath;
    my $originalWorkingDirectory = getcwd();
    chdir($buildPath) or die;

    my @args;
    push @args, "-DPORT=\"$port\"";
    push @args, "-DCMAKE_INSTALL_PREFIX=\"$prefixPath\"" if $prefixPath;
    if ($config =~ /release/i) {
        push @args, "-DCMAKE_BUILD_TYPE=Release";
    } elsif ($config =~ /debug/i) {
        push @args, "-DCMAKE_BUILD_TYPE=Debug";
    }
    push @args, @cmakeArgs if @cmakeArgs;
    push @args, $additionalCMakeArgs if $additionalCMakeArgs;

    push @args, '"' . sourceDir() . '"';

    # We call system("cmake @args") instead of system("cmake", @args) so that @args is
    # parsed for shell metacharacters.
    my $returnCode = system("cmake @args");

    chdir($originalWorkingDirectory);
    return $returnCode;
}

sub buildCMakeGeneratedProject($)
{
    my ($makeArgs) = @_;
    my $config = configuration();
    my $buildPath = File::Spec->catdir(baseProductDir(), $config);
    if (! -d $buildPath) {
        die "Must call generateBuildSystemFromCMakeProject() before building CMake project.";
    }
    my @args = ("--build", $buildPath, "--config", $config);
    push @args, ("--", $makeArgs) if $makeArgs;

    # We call system("cmake @args") instead of system("cmake", @args) so that @args is
    # parsed for shell metacharacters. In particular, $makeArgs may contain such metacharacters.
    return system("cmake @args");
}

sub cleanCMakeGeneratedProject()
{
    my $config = configuration();
    my $buildPath = File::Spec->catdir(baseProductDir(), $config);
    if (-d $buildPath) {
        return system("cmake", "--build", $buildPath, "--config", $config, "--target", "clean");
    }
    return 0;
}

sub buildCMakeProjectOrExit($$$$@)
{
    my ($clean, $port, $prefixPath, $makeArgs, @cmakeArgs) = @_;
    my $returnCode;

    exit(exitStatus(cleanCMakeGeneratedProject())) if $clean;

    $returnCode = exitStatus(generateBuildSystemFromCMakeProject($port, $prefixPath, @cmakeArgs));
    exit($returnCode) if $returnCode;
    $returnCode = exitStatus(buildCMakeGeneratedProject($makeArgs));
    exit($returnCode) if $returnCode;
}

sub promptUser
{
    my ($prompt, $default) = @_;
    my $defaultValue = $default ? "[$default]" : "";
    print "$prompt $defaultValue: ";
    chomp(my $input = <STDIN>);
    return $input ? $input : $default;
}

sub buildQMakeProject($@)
{
    my ($project, $clean, @buildParams) = @_;

    my @buildArgs = ();

    my $makeargs = "";
    my $installHeaders;
    my $installLibs;
    for my $i (0 .. $#buildParams) {
        my $opt = $buildParams[$i];
        if ($opt =~ /^--qmake=(.*)/i ) {
            $qmakebin = $1;
        } elsif ($opt =~ /^--qmakearg=(.*)/i ) {
            push @buildArgs, $1;
        } elsif ($opt =~ /^--makeargs=(.*)/i ) {
            $makeargs = $1;
        } elsif ($opt =~ /^--install-headers=(.*)/i ) {
            $installHeaders = $1;
        } elsif ($opt =~ /^--install-libs=(.*)/i ) {
            $installLibs = $1;
        } else {
            push @buildArgs, $opt;
        }
    }

    my $qmakepath = File::Spec->catfile(sourceDir(), "Tools", "qmake");
    my $qmakecommand;
    if (isWindows()) {
        $qmakecommand = "(set QMAKEPATH=$qmakepath) && $qmakebin";
    } else {
        $qmakecommand = "QMAKEPATH=$qmakepath $qmakebin";
    }

    my $make = qtMakeCommand($qmakebin);
    my $config = configuration();
    push @buildArgs, "INSTALL_HEADERS=" . $installHeaders if defined($installHeaders);
    push @buildArgs, "INSTALL_LIBS=" . $installLibs if defined($installLibs);

    my $passedConfig = passedConfiguration() || "";
    if ($passedConfig =~ m/debug/i) {
        push @buildArgs, "CONFIG-=release";
        push @buildArgs, "CONFIG+=debug";
    } elsif ($passedConfig =~ m/release/i) {
        push @buildArgs, "CONFIG+=release";
        push @buildArgs, "CONFIG-=debug";
    }
    push @buildArgs, "CONFIG-=debug_and_release" if ($passedConfig && isDarwin());

    my $originalCwd = getcwd();
    my $dir = File::Spec->canonpath(productDir());
    File::Path::mkpath($dir);
    chdir $dir or die "Failed to cd into " . $dir . "\n";

    my %defines = qtFeatureDefaults(\@buildArgs);

    my $needsCleanBuild = 0;

    # Ease transition to new build layout
    my $pathToOldDefinesFile = File::Spec->catfile($dir, "defaults.txt");
    if (-e $pathToOldDefinesFile) {
        print "Old build layout detected";
        $needsCleanBuild = 1;
    }

    my $pathToDefinesCache = File::Spec->catfile($dir, ".webkit.config");
    if ($needsCleanBuild || (-e $pathToDefinesCache && open(DEFAULTS, $pathToDefinesCache))) {
        if (!$needsCleanBuild) {
            while (<DEFAULTS>) {
                if ($_ =~ m/(\S+?)=(\S+?)/gi) {
                    if (! exists $defines{$1}) {
                        print "Feature $1 was removed";
                        $needsCleanBuild = 1;
                        last;
                    }

                    if ($defines{$1} != $2) {
                        print "Feature $1 has changed ($2 -> $defines{$1})";
                        $needsCleanBuild = 1;
                        last;
                    }
                }
            }
            close (DEFAULTS);
        }

        if ($needsCleanBuild) {
            print ", clean build needed!\n";
            # FIXME: This STDIN/STDOUT check does not work on the bots. Disable until it does.
            # if (! -t STDIN || ( &promptUser("Would you like to clean the build directory?", "yes") eq "yes")) {
                chdir $originalCwd;
                File::Path::rmtree($dir);
                File::Path::mkpath($dir);
                chdir $dir or die "Failed to cd into " . $dir . "\n";
            #}
        }
    }

    open(DEFAULTS, ">$pathToDefinesCache");
    print DEFAULTS "# These defines were set when building WebKit last time\n";
    foreach my $key (sort keys %defines) {
        print DEFAULTS "$key=$defines{$key}\n";
    }
    close(DEFAULTS);

    my $result = 0;

    my $makefile = File::Spec->catfile($dir, "Makefile");
    if (! -e $makefile) {
        if ($project) {
            push @buildArgs, "-after OVERRIDE_SUBDIRS=" . $project;
        }

        push @buildArgs, File::Spec->catfile(sourceDir(), "WebKit.pro");
        my $command = "$qmakecommand @buildArgs";
        print "Calling '$command' in " . $dir . "\n\n";
        print "Installation headers directory: $installHeaders\n" if(defined($installHeaders));
        print "Installation libraries directory: $installLibs\n" if(defined($installLibs));

        $result = system "$command";
        if ($result ne 0) {
           die "Failed to setup build environment using $qmakebin!\n";
        }
    } elsif ($project) {
        $dir = File::Spec->catfile($dir, "Source", $project);
        chdir $dir or die "Failed to cd into " . $dir . "\n";
        $make = "$make -f Makefile.$project";
    }

    my $command = "$make $makeargs";
    $command =~ s/\s+$//;

    if ($clean) {
        $command = "$command distclean";
    } else {
        $command = "$command incremental";
    }

    print "Calling '$command' in " . $dir . "\n\n";
    $result = system $command;

    chdir ".." or die;
    return $result;
}

sub buildQMakeQtProject($$@)
{
    my ($project, $clean, @buildArgs) = @_;

    return buildQMakeProject("", $clean, @buildArgs);
}

sub buildGtkProject
{
    my ($project, $clean, $enableWebKit2, @buildArgs) = @_;

    if ($project ne "WebKit" and $project ne "JavaScriptCore") {
        die "Unsupported project: $project. Supported projects: WebKit, JavaScriptCore\n";
    }

    return buildAutotoolsProject($project, $clean, $enableWebKit2, @buildArgs);
}

sub buildChromiumMakefile($$@)
{
    my ($target, $clean, @options) = @_;
    if ($clean) {
        return system qw(rm -rf out);
    }
    my $config = configuration();
    my $numCpus = numberOfCPUs();
    my $makeArgs;
    for (@options) {
        $makeArgs = $1 if /^--makeargs=(.*)/i;
    }
    $makeArgs = "-j$numCpus" if not $makeArgs;
    my $command = "";

    # Building the WebKit Chromium port for Android requires us to cross-
    # compile, which will be set up by Chromium's envsetup.sh. The script itself
    # will verify that the installed NDK is indeed available.
    if (isChromiumAndroid()) {
        $command .= "bash -c \"source " . sourceDir() . "/Source/WebKit/chromium/build/android/envsetup.sh && ";
        $ENV{ANDROID_NDK_ROOT} = sourceDir() . "/Source/WebKit/chromium/third_party/android-ndk-r7";
        $ENV{WEBKIT_ANDROID_BUILD} = 1;
    }

    $command .= "make -fMakefile.chromium $makeArgs BUILDTYPE=$config $target";
    $command .= "\"" if isChromiumAndroid();

    print "$command\n";
    return system $command;
}

sub buildChromiumVisualStudioProject($$)
{
    my ($projectPath, $clean) = @_;

    my $config = configuration();
    my $action = "/build";
    $action = "/clean" if $clean;

    # Find Visual Studio installation.
    my $vsInstallDir;
    my $programFilesPath = $ENV{'PROGRAMFILES'} || "C:\\Program Files";
    if ($ENV{'VSINSTALLDIR'}) {
        $vsInstallDir = $ENV{'VSINSTALLDIR'};
    } else {
        $vsInstallDir = "$programFilesPath/Microsoft Visual Studio 8";
    }
    $vsInstallDir = `cygpath "$vsInstallDir"` if isCygwin();
    chomp $vsInstallDir;
    $vcBuildPath = "$vsInstallDir/Common7/IDE/devenv.com";
    if (! -e $vcBuildPath) {
        # Visual Studio not found, try VC++ Express
        $vcBuildPath = "$vsInstallDir/Common7/IDE/VCExpress.exe";
        if (! -e $vcBuildPath) {
            print "*************************************************************\n";
            print "Cannot find '$vcBuildPath'\n";
            print "Please execute the file 'vcvars32.bat' from\n";
            print "'$programFilesPath\\Microsoft Visual Studio 8\\VC\\bin\\'\n";
            print "to setup the necessary environment variables.\n";
            print "*************************************************************\n";
            die;
        }
    }

    # Create command line and execute it.
    my @command = ($vcBuildPath, $projectPath, $action, $config);
    print "Building results into: ", baseProductDir(), "\n";
    print join(" ", @command), "\n";
    return system @command;
}

sub buildChromium($@)
{
    my ($clean, @options) = @_;

    # We might need to update DEPS or re-run GYP if things have changed.
    if (checkForArgumentAndRemoveFromArrayRef("--update-chromium", \@options)) {
        system("perl", "Tools/Scripts/update-webkit-chromium", "--force") == 0 or die $!;
    }

    my $result = 1;
    if (isDarwin() && !isChromiumAndroid() && !isChromiumMacMake()) {
        # Mac build - builds the root xcode project.
        $result = buildXCodeProject("Source/WebKit/chromium/WebKit", $clean, "-configuration", configuration(), @options);
    } elsif (isCygwin() || isWindows()) {
        # Windows build - builds the root visual studio solution.
        $result = buildChromiumVisualStudioProject("Source/WebKit/chromium/WebKit.sln", $clean);
    } elsif (isLinux() || isChromiumAndroid() || isChromiumMacMake) {
        # Linux build - build using make.
        $result = buildChromiumMakefile("all", $clean, @options);
    } else {
        print STDERR "This platform is not supported by chromium.\n";
    }
    return $result;
}

sub appleApplicationSupportPath
{
    open INSTALL_DIR, "</proc/registry/HKEY_LOCAL_MACHINE/SOFTWARE/Apple\ Inc./Apple\ Application\ Support/InstallDir";
    my $path = <INSTALL_DIR>;
    $path =~ s/[\r\n\x00].*//;
    close INSTALL_DIR;

    my $unixPath = `cygpath -u '$path'`;
    chomp $unixPath;
    return $unixPath;
}

sub setPathForRunningWebKitApp
{
    my ($env) = @_;

    if (isAppleWinWebKit()) {
        $env->{PATH} = join(':', productDir(), dirname(installedSafariPath()), appleApplicationSupportPath(), $env->{PATH} || "");
    } elsif (isQt()) {
        my $qtLibs = `$qmakebin -query QT_INSTALL_LIBS`;
        $qtLibs =~ s/[\n|\r]$//g;
        $env->{PATH} = join(';', $qtLibs, productDir() . "/lib", $env->{PATH} || "");
    }
}

sub runMacWebKitApp($;$)
{
    my ($appPath, $useOpenCommand) = @_;
    my $productDir = productDir();
    print "Starting @{[basename($appPath)]} with DYLD_FRAMEWORK_PATH set to point to built WebKit in $productDir.\n";
    $ENV{DYLD_FRAMEWORK_PATH} = $productDir;
    $ENV{WEBKIT_UNSET_DYLD_FRAMEWORK_PATH} = "YES";
    if (defined($useOpenCommand) && $useOpenCommand == USE_OPEN_COMMAND) {
        return system("open", "-W", "-a", $appPath, "--args", @ARGV);
    }
    if (architecture()) {
        return system "arch", "-" . architecture(), $appPath, @ARGV;
    }
    return system { $appPath } $appPath, @ARGV;
}

sub execMacWebKitAppForDebugging($)
{
    my ($appPath) = @_;

    my $gdbPath = "/usr/bin/gdb";
    die "Can't find gdb executable. Is gdb installed?\n" unless -x $gdbPath;

    my $productDir = productDir();
    print "Starting @{[basename($appPath)]} under gdb with DYLD_FRAMEWORK_PATH set to point to built WebKit in $productDir.\n";
    $ENV{DYLD_FRAMEWORK_PATH} = $productDir;
    $ENV{WEBKIT_UNSET_DYLD_FRAMEWORK_PATH} = "YES";
    my @architectureFlags = ("-arch", architecture());
    exec { $gdbPath } $gdbPath, @architectureFlags, $appPath or die;
}

sub debugSafari
{
    if (isAppleMacWebKit()) {
        checkFrameworks();
        execMacWebKitAppForDebugging(safariPath());
    }

    if (isAppleWinWebKit()) {
        setupCygwinEnv();
        my $productDir = productDir();
        chomp($ENV{WEBKITNIGHTLY} = `cygpath -wa "$productDir"`);
        my $safariPath = safariPath();
        chomp($safariPath = `cygpath -wa "$safariPath"`);
        return system { $vcBuildPath } $vcBuildPath, "/debugexe", "\"$safariPath\"", @ARGV;
    }

    return 1; # Unsupported platform; can't debug Safari on this platform.
}

sub runSafari
{

    if (isAppleMacWebKit()) {
        return runMacWebKitApp(safariPath());
    }

    if (isAppleWinWebKit()) {
        my $result;
        my $productDir = productDir();
        my $webKitLauncherPath = File::Spec->catfile(productDir(), "WebKit.exe");
        return system { $webKitLauncherPath } $webKitLauncherPath, @ARGV;
    }

    return 1; # Unsupported platform; can't run Safari on this platform.
}

sub runMiniBrowser
{
    if (isAppleMacWebKit()) {
        return runMacWebKitApp(File::Spec->catfile(productDir(), "MiniBrowser.app", "Contents", "MacOS", "MiniBrowser"));
    }

    return 1;
}

sub debugMiniBrowser
{
    if (isAppleMacWebKit()) {
        execMacWebKitAppForDebugging(File::Spec->catfile(productDir(), "MiniBrowser.app", "Contents", "MacOS", "MiniBrowser"));
    }
    
    return 1;
}

sub runWebKitTestRunner
{
    if (isAppleMacWebKit()) {
        return runMacWebKitApp(File::Spec->catfile(productDir(), "WebKitTestRunner"));
    } elsif (isGtk()) {
        my $productDir = productDir();
        my $injectedBundlePath = "$productDir/Libraries/.libs/libTestRunnerInjectedBundle";
        print "Starting WebKitTestRunner with TEST_RUNNER_INJECTED_BUNDLE_FILENAME set to point to $injectedBundlePath.\n";
        $ENV{TEST_RUNNER_INJECTED_BUNDLE_FILENAME} = $injectedBundlePath;
        my @args = ("$productDir/Programs/WebKitTestRunner", @ARGV);
        return system {$args[0] } @args;
    }

    return 1;
}

sub debugWebKitTestRunner
{
    if (isAppleMacWebKit()) {
        execMacWebKitAppForDebugging(File::Spec->catfile(productDir(), "WebKitTestRunner"));
    }

    return 1;
}

sub runTestWebKitAPI
{
    if (isAppleMacWebKit()) {
        return runMacWebKitApp(File::Spec->catfile(productDir(), "TestWebKitAPI"));
    }

    return 1;
}

sub readRegistryString
{
    my ($valueName) = @_;
    chomp(my $string = `regtool --wow32 get "$valueName"`);
    return $string;
}

sub writeRegistryString
{
    my ($valueName, $string) = @_;

    my $error = system "regtool", "--wow32", "set", "-s", $valueName, $string;

    # On Windows Vista/7 with UAC enabled, regtool will fail to modify the registry, but will still
    # return a successful exit code. So we double-check here that the value we tried to write to the
    # registry was really written.
    return !$error && readRegistryString($valueName) eq $string;
}

1;
