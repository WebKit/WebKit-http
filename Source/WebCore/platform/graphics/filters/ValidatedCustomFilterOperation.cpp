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

#include "config.h"

#if ENABLE(CSS_SHADERS)
#include "ValidatedCustomFilterOperation.h"

#include "CustomFilterParameter.h"
#include "CustomFilterValidatedProgram.h"
#include "LayoutSize.h"
#include <wtf/UnusedParam.h>

namespace WebCore {

ValidatedCustomFilterOperation::ValidatedCustomFilterOperation(PassRefPtr<CustomFilterValidatedProgram> validatedProgram, 
    const CustomFilterParameterList& sortedParameters, unsigned meshRows, unsigned meshColumns, CustomFilterMeshType meshType)
    : FilterOperation(VALIDATED_CUSTOM)
    , m_validatedProgram(validatedProgram)
    , m_parameters(sortedParameters)
    , m_meshRows(meshRows)
    , m_meshColumns(meshColumns)
    , m_meshType(meshType)
{
}

ValidatedCustomFilterOperation::~ValidatedCustomFilterOperation()
{
}

PassRefPtr<FilterOperation> ValidatedCustomFilterOperation::blend(const FilterOperation*, double progress, const LayoutSize& size, bool blendToPassthrough)
{
    UNUSED_PARAM(progress);
    UNUSED_PARAM(size);
    UNUSED_PARAM(blendToPassthrough);

    ASSERT_NOT_REACHED();
    return this;
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)
