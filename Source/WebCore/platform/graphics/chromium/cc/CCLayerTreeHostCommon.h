/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CCLayerTreeHostCommon_h
#define CCLayerTreeHostCommon_h

#include "IntSize.h"
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class CCLayerImpl;
class CCLayerSorter;
class IntRect;
class LayerChromium;
class TransformationMatrix;

class CCLayerTreeHostCommon {
public:
    static IntRect calculateVisibleLayerRect(const IntRect& targetSurfaceRect, const IntSize& bounds, const IntSize& contentBounds, const TransformationMatrix& tilingTransform);
    static IntRect calculateVisibleRect(const IntRect& targetSurfaceRect, const IntRect& layerBoundRect, const TransformationMatrix&);

    static void calculateDrawTransformsAndVisibility(LayerChromium*, LayerChromium* rootLayer, const TransformationMatrix& parentMatrix, const TransformationMatrix& fullHierarchyMatrix, Vector<RefPtr<LayerChromium> >& renderSurfaceLayerList, Vector<RefPtr<LayerChromium> >& layerList, int maxTextureSize);

    static void calculateDrawTransformsAndVisibility(CCLayerImpl*, CCLayerImpl* rootLayer, const TransformationMatrix& parentMatrix, const TransformationMatrix& fullHierarchyMatrix, Vector<RefPtr<CCLayerImpl> >& renderSurfaceLayerList, Vector<RefPtr<CCLayerImpl> >& layerList, CCLayerSorter*, int maxTextureSize);

    struct ScrollUpdateInfo {
        int layerId;
        IntSize scrollDelta;
    };
};

typedef Vector<CCLayerTreeHostCommon::ScrollUpdateInfo> CCScrollUpdateSet;

} // namespace WebCore

#endif
