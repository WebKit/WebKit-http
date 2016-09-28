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

`git status` =~ "nothing to commit, working tree clean" or die "Target working tree is dirty";

my $src_repo = dirname(dirname(dirname(__FILE__)));
my $target_repo = getcwd();

chdir $src_repo;
(-f "ChangeLog" && -x "Tools/gtk/make-dist.py") or usage("Target repository path is invalid!\n");
my $commit = `git rev-parse HEAD` or usage("Cannot get HEAD revision in target repo!\n");
chomp $commit;

my @commands = (
    "Tools/gtk/make-dist.py -t snapshot Tools/qt/manifest.txt",
    "cd $target_repo",
    "git rm -rf *",
    "tar -xf $src_repo/snapshot.tar --strip-components=1",
    "git add -A",
    "rm $src_repo/snapshot.tar",
    "git commit -m 'Imported WebKit commit $commit'"
);

my $cmd = join " && ", @commands;
print "Executing $cmd\n";

exec $cmd;
