/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This all-in-one cpp file cuts down on template bloat to allow us to build our Windows release build.

#include "InspectorAgent.cpp"
#include "InspectorApplicationCacheAgent.cpp"
#include "InspectorBaseAgent.cpp"
#include "InspectorCSSAgent.cpp"
#include "InspectorClient.cpp"
#include "InspectorConsoleAgent.cpp"
#include "InspectorController.cpp"
#include "InspectorDOMAgent.cpp"
#include "InspectorDOMDebuggerAgent.cpp"
#include "InspectorDOMStorageAgent.cpp"
#include "InspectorDOMStorageResource.cpp"
#include "InspectorDatabaseAgent.cpp"
#include "InspectorDatabaseResource.cpp"
#include "InspectorDebuggerAgent.cpp"
#include "InspectorFileSystemAgent.cpp"
#include "InspectorFrontendClientLocal.cpp"
#include "InspectorFrontendHost.cpp"
#include "InspectorIndexedDBAgent.cpp"
#include "InspectorInstrumentation.cpp"
#include "InspectorMemoryAgent.cpp"
#include "InspectorPageAgent.cpp"
#include "InspectorProfilerAgent.cpp"
#include "InspectorResourceAgent.cpp"
#include "InspectorRuntimeAgent.cpp"
#include "InspectorState.cpp"
#include "InspectorStyleSheet.cpp"
#include "InspectorStyleTextEditor.cpp"
#include "InspectorTimelineAgent.cpp"
#include "InspectorValues.cpp"
#include "InspectorWorkerAgent.cpp"
