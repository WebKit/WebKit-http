#!/usr/bin/perl
# Copyright (C) 2016 Konstantin Tokavev <annulen@yandex.ru>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

use File::Basename;
use File::Copy;
use File::Path qw(make_path);
use File::Spec;
use Getopt::Long qw(:config pass_through no_auto_abbrev);

use v5.10;

use strict;
use warnings;

my @sources;
my @extraSources;
my @qrcSources;
my $baseDir;
my $outDir;
my $prefix;
my $rccExecutable;
my $resourceName;

processArgs();
copySources();
addExtraSources();
writeQrc();
compileQrc();

sub processArgs {
    GetOptions (
        "add=s" => \@extraSources,
        "baseDir=s" => \$baseDir,
        "outDir=s" => \$outDir,
        "prefix=s" => \$prefix,
        "rccExecutable=s" => \$rccExecutable,
        "resourceName=s" => \$resourceName
    );
    @sources = @ARGV;
    checkDir($baseDir);
    make_path($outDir);
    -x $rccExecutable or die "File $rccExecutable is not executable";
}

sub checkDir {
    my $dir = shift;
    -e $dir or die "Directory $dir does not exist";
    -d $dir or die "$dir is not a directory";
}

sub copySources {
    for my $src (@sources) {
        my $relSrc = getDstFile(File::Spec->abs2rel($src, $baseDir));
        my $dst = File::Spec->catfile($outDir, $relSrc);
        doCopy($src, $dst);
        addToQrc($relSrc);
    }
}

sub getDstFile {
    my $path = shift;
    my ($volume, $directories, $file) = File::Spec->splitpath($path);
    my @dirs = File::Spec->splitdir($directories);

    # Images/gtk/Something -> Images/Something
    @dirs = grep { !/^gtk$/ } @dirs;

    $directories = File::Spec->catdir(@dirs);
    return File::Spec->catpath($volume, $directories, $file);
}

sub addExtraSources {
    for my $src (@extraSources) {
        addToQrc($src);
    }
}

sub doCopy {
    my ($src, $dst) = @_;
    say "Copying $src -> $dst";
    make_path(dirname($dst));
    copy($src, $dst);
}

sub addToQrc {
    my $src = shift;
    push @qrcSources, $src;
}

sub writeQrc {
    my $filename = File::Spec->catfile($outDir, "$resourceName.qrc");

    open my $qrc, ">", $filename or die "Failed to open $filename for writing: $!";

    say $qrc '<!DOCTYPE RCC><RCC version="1.0">';
    if ($prefix) {
        say $qrc qq%<qresource prefix="$prefix">%;
    } else {
        say $qrc '<qresource>';
    }

    for my $src (@qrcSources) {
        say $qrc "    <file>$src</file>";
    }
    say $qrc '</qresource>';
    say $qrc '</RCC>';

    close $qrc or die $!;
}

sub compileQrc {
    chdir($outDir);
    my $cmd = "$rccExecutable -name $resourceName -o qrc_$resourceName.cpp $resourceName.qrc";
    say $cmd;
    system($cmd) == 0 or die "rcc failed: $!";
}
