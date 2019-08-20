#!/usr/bin/env perl
# Copyright (C) 2019 Konstantin Tokarev <annulen@yandex.ru>
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

use Cwd;
use File::Basename;
use strict;
use warnings;

sub usage {
    my $msg = shift || "";
    die $msg . "Usage: cd snapshot/repo/path; $0 <version>";
}

scalar @ARGV == 1 || usage();

my $version = shift;

-f "ChangeLog" && die "This script must be run in snapshots repository";
-d ".git" || usage();

`LC_ALL=C LANG=C git status` =~ "nothing to commit, working tree clean" or die "Snapshots working tree is dirty";

my $snapshotTag = "v$version";
my $originalTag = "qtwebkit-$version";

`git tag -l $snapshotTag` && print "Found $snapshotTag\n" || die "Tag $snapshotTag is not found";
`git tag -l $originalTag` && print "Found $originalTag\n" || die "Tag $originalTag is not found";

my $originalTagHash = `git log -1 --format=format:"%H" $originalTag`;
chomp $originalTagHash;

open (my $gitShow, "-|", "git show -s --format=%B $snapshotTag") or die "Failed to run git show: $!";
while (<$gitShow>) {
    if (/^Import WebKit commit ([0-9a-f]+)$/) {
        print "Snaphot imported tag = $1\n";
        unless ($1 eq $originalTagHash) {
            die "Imported tag '$snapshotTag' does not point to '$originalTag' ('$originalTagHash')";
        }
        last;
    }
}

# Now we are ok, let's finally make tarball
my $cmd = "git archive --prefix=$originalTag/ $snapshotTag | xz -9 > $originalTag.tar.xz";
print "$cmd\n";
system($cmd) == 0 or die "git archive failed: $!";

# TODO: Upload to GitHub?
