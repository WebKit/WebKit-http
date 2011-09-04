/*
 * Copyright (C) 2007 Kevin Ollivier  All rights reserved.
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

#include "config.h"
#include "AffineTransform.h"
#include "TransformationMatrix.h"

#include "Assertions.h"
#include "FloatRect.h"
#include "IntRect.h"

#include <stdio.h>
#include <wx/defs.h>
#include <wx/graphics.h>

namespace WebCore {

#if USE(WXGC)
TransformationMatrix::operator wxGraphicsMatrix() const
{
    wxGraphicsRenderer* renderer = 0;
#if wxUSE_CAIRO
    renderer = wxGraphicsRenderer::GetCairoRenderer();
#else
    renderer = wxGraphicsRenderer::GetDefaultRenderer();
#endif
    ASSERT(renderer);
    
    wxGraphicsMatrix matrix = renderer->CreateMatrix(a(), b(), c(), d(), e(), f());
    return matrix;
}

AffineTransform::operator wxGraphicsMatrix() const
{
    wxGraphicsRenderer* renderer = 0;
#if wxUSE_CAIRO
    renderer = wxGraphicsRenderer::GetCairoRenderer();
#else
    renderer = wxGraphicsRenderer::GetDefaultRenderer();
#endif
    ASSERT(renderer);
    
    wxGraphicsMatrix matrix = renderer->CreateMatrix(a(), b(), c(), d(), e(), f());
    return matrix;
}
#endif

}
