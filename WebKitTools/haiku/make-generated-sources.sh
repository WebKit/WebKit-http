#!/bin/sh

# Copyright (C) 2009 Maxime Simon  All rights reserved.
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
# THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# Create the symbolic links for perl and gcc in /usr/bin
mkdir -p /usr/bin

test -x /usr/bin/perl || ln -sf `which perl` /usr/bin/perl
test -x /usr/bin/gcc  || ln -sf `which gcc`  /usr/bin/gcc

# Check gperf is available and executable
GPERF=`which gperf`
if [ ! -f "$GPERF" ]; then
	echo "Error: gperf tool not found!"
	exit 1
fi
test -x "$GPERF" || chmod a+x "$GPERF"

scriptDir="$(cd $(dirname $0);pwd)"
WK_ROOT=$scriptDir/../..
WK_ROOTDIR="$WK_ROOT"

DEFINES="ENABLE_DATABASE ENABLE_JAVASCRIPT_DEBUGGER ENABLE_DOM_STORAGE \
ENABLE_SVG ENABLE_SVG_ANIMATION ENABLE_SVG_FONTS ENABLE_SVG_FOREIGN_OBJECT ENABLE_SVG_USE"

cd $scriptDir

export CREATE_HASH_TABLE="$WK_ROOT/JavaScriptCore/create_hash_table"
cd $WK_ROOT/JavaScriptCore
mkdir -p DerivedSources/JavaScriptCore
cd DerivedSources/JavaScriptCore

make -f ../../DerivedSources.make JavaScriptCore=../.. BUILT_PRODUCTS_DIR=../.. all FEATURE_DEFINES="$DEFINES"
if [ $? != 0 ]; then
    exit 1
fi

cd $WK_ROOT/WebCore
mkdir -p DerivedSources/WebCore
cd DerivedSources/WebCore
make -f ../../DerivedSources.make all WebCore=../.. SOURCE_ROOT=../.. FEATURE_DEFINES="$DEFINES"
if [ $? != 0 ]; then
    exit 1
fi

# __inline seems to be unsupported by Haiku GCC 4
# Patch generated sources to use inline instead:

#for file in CSSPropertyNames.cpp CSSValueKeywords.c ColorData.c DocTypeStrings.cpp HTMLEntityNames.c
#do
#	echo "Patching $file: __inline -> inline"
#	sed --in-place=.unpatched -e 's/^__inline$/inline/g' "$file"
#done

# modify the checks around gnu_inline to include NDEBUG so that symbols are
# generated for debug builds
for file in CSSPropertyNames.cpp CSSValueKeywords.c ColorData.c DocTypeStrings.cpp HTMLEntityNames.c
do
	echo "Patching $file: wrap gnu_inline in NDEBUG check"
	sed --in-place=.unpatched -e 's/__GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__/NDEBUG \&\& (__GNUC_STDC_INLINE__ || defined __GNUC_GNU_INLINE__)/g' "$file"
done
