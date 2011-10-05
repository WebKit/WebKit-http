# Copyright (C) 2010 Apple Inc. All rights reserved.
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

VPATH = \
    $(WebKitTestRunner)/InjectedBundle/Bindings \
#

INTERFACES = \
    EventSendingController \
    GCController \
    LayoutTestController \
    TextInputController \
#

SCRIPTS = \
    $(WebCoreScripts)/CodeGenerator.pm \
    $(WebKitTestRunner)/InjectedBundle/Bindings/CodeGeneratorTestRunner.pm \
    $(WebCoreScripts)/IDLParser.pm \
    $(WebCoreScripts)/IDLStructure.pm \
    $(WebCoreScripts)/generate-bindings.pl \
#

.PHONY : all

JS%.h JS%.cpp : %.idl $(SCRIPTS)
	@echo Generating bindings for $*...
	@perl -I $(WebCoreScripts) -I $(WebKitTestRunner)/InjectedBundle/Bindings $(WebCoreScripts)/generate-bindings.pl --defines "" --include InjectedBundle/Bindings --outputDir . --generator TestRunner $<

all : \
    $(INTERFACES:%=JS%.h) \
    $(INTERFACES:%=JS%.cpp) \
#
