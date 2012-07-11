#!/bin/sh

#
# Copyright 2012, Alexandre Deckner, alexandre.deckner@uzzl.com.
# Distributed under the terms of the MIT License.
#

CURENT_HASH=`git log --pretty=format:'%h' -n 1`

MERGE_HASH=`git log --pretty=format:'%H' --date-order --merges -n 1`
REV_1=`git log $MERGE_HASH^1 -n 1 | perl -nle '/webkit\.org.+trunk@(\d+)/ and print $1'`
REV_2=`git log $MERGE_HASH^2 -n 1 | perl -nle '/webkit\.org.+trunk@(\d+)/ and print $1'`

HAIKU_WEBKIT_VERSION=1.1

TOP=`git rev-parse --show-toplevel`
WEBKIT_MAJOR=`cat $TOP/Source/WebKit/mac/Configurations/Version.xcconfig | perl -nle '/MAJOR_VERSION = (\d+);/ and print $1'`
WEBKIT_MINOR=`cat $TOP/Source/WebKit/mac/Configurations/Version.xcconfig | perl -nle '/MINOR_VERSION = (\d+);/ and print $1'`

echo '/*
 * THIS FILE WAS GENERATED AUTOMATICALLY BY generate-build-info.sh
 */
#ifndef BuildInfo_h
#define BuildInfo_h

#define HAIKU_WEBKIT_REVISION "'$CURENT_HASH'"
#define HAIKU_WEBKIT_VERSION "'$HAIKU_WEBKIT_VERSION'"

#define WEBKIT_REVISION "r'$REV_1$REV_2'"
#define WEBKIT_MAJOR_VERSION '$WEBKIT_MAJOR'
#define WEBKIT_MINOR_VERSION '$WEBKIT_MINOR'

#endif //BuildInfo_h'
