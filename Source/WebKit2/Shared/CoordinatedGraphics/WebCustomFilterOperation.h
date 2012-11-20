/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef WebCustomFilterOperation_h
#define WebCustomFilterOperation_h

#if ENABLE(CSS_SHADERS)
#include "CustomFilterOperation.h"

namespace WebCore {

// This class is only adding the programId on top of CustomFilterOperation. The argument decoder has no
// context, so we cannot search in our cached CustomFilterPrograms while decoding. In that case
// it will just store the programId and no CustomFilterProgram instance. The receiver is supposed to
// iterate on this structure and inject the right CustomFilterPrograms.

class WebCustomFilterOperation : public CustomFilterOperation {
public:
    static PassRefPtr<WebCustomFilterOperation> create(PassRefPtr<CustomFilterProgram> program, int programID, const CustomFilterParameterList& sortedParameters, unsigned meshRows, unsigned meshColumns, CustomFilterMeshBoxType meshBoxType, CustomFilterMeshType meshType)
    {
        return adoptRef(new WebCustomFilterOperation(program, programID, sortedParameters, meshRows, meshColumns, meshBoxType, meshType));
    }

    int programID() const { return m_programID; }

private:
    WebCustomFilterOperation(PassRefPtr<CustomFilterProgram> program, int programID, const CustomFilterParameterList& sortedParameters, unsigned meshRows, unsigned meshColumns, CustomFilterMeshBoxType meshBoxType, CustomFilterMeshType meshType)
        : CustomFilterOperation(program, sortedParameters, meshRows, meshColumns, meshBoxType, meshType)
        , m_programID(programID)
    {
    }

    int m_programID;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

#endif // CustomFilterOperation_h
