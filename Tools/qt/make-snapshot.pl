#!/usr/bin/env perl

use Cwd;
use File::Basename;
use strict;
use warnings;

sub usage {
    my $msg = shift || "";
    die $msg . "Usage: cd target/repo/path; $0 src/repo/path";
}

scalar @ARGV == 0 || usage();
-f "ChangeLog" && die "This script must be run in snapshots repository";
-d ".git" || usage();

my $src_repo = dirname(dirname(dirname(__FILE__)));
my $target_repo = getcwd();

chdir $src_repo;
(-f "ChangeLog" && -x "Tools/gtk/make-dist.py") or usage("Target repository path is invalid!\n");

exec("Tools/gtk/make-dist.py -t snapshot Tools/qt/manifest.txt")
