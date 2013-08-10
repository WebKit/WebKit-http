#!/bin/sh

#
# Copyright 2012, Alexandre Deckner, alexandre.deckner@uzzl.com.
# Distributed under the terms of the MIT License.
#

# Usage: generate-build-info.sh <source top dir>
#			[ <webkit revision> <haiku webkit revision> ]


if [ $# -gt 2 ]; then
	WEBKIT_REVISION=$2
	HAIKU_WEBKIT_REVISION=$3
else
	MERGE_HASH=`git log --pretty=format:'%H' --date-order --merges -n 1`
	REV_1=`git log $MERGE_HASH^1 -n 1 | perl -nle '/webkit\.org.+trunk@(\d+)/ and print $1'`
	REV_2=`git log $MERGE_HASH^2 -n 1 | perl -nle '/webkit\.org.+trunk@(\d+)/ and print $1'`
	WEBKIT_REVISION=r$REV_1$REV_2
	HAIKU_WEBKIT_REVISION=`git log --pretty=format:'%h' -n 1`
fi

HAIKU_WEBKIT_VERSION=1.1

TOP="$1"
WEBKIT_MAJOR=`cat $TOP/Source/WebKit/mac/Configurations/Version.xcconfig | perl -nle '/MAJOR_VERSION = (\d+);/ and print $1'`
WEBKIT_MINOR=`cat $TOP/Source/WebKit/mac/Configurations/Version.xcconfig | perl -nle '/MINOR_VERSION = (\d+);/ and print $1'`

echo '/*
 * THIS FILE WAS GENERATED AUTOMATICALLY BY generate-build-info.sh
 */
#ifndef BuildInfo_h
#define BuildInfo_h

#define HAIKU_WEBKIT_REVISION "'$HAIKU_WEBKIT_REVISION'"
#define HAIKU_WEBKIT_VERSION "'$HAIKU_WEBKIT_VERSION'"

#define WEBKIT_REVISION "'$WEBKIT_REVISION'"
#define WEBKIT_MAJOR_VERSION '$WEBKIT_MAJOR'
#define WEBKIT_MINOR_VERSION '$WEBKIT_MINOR'

#endif //BuildInfo_h'
