/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#if ENABLE(CSS_FILTERS)

#include "FilterEffectRenderer.h"

#include "ColorSpace.h"
#include "Document.h"
#include "FEColorMatrix.h"
#include "FEComponentTransfer.h"
#include "FEDropShadow.h"
#include "FEGaussianBlur.h"
#include "FEMerge.h"
#include "FloatConversion.h"
#include "RenderLayer.h"

#include <algorithm>
#include <wtf/MathExtras.h>

#if ENABLE(CSS_SHADERS) && ENABLE(WEBGL)
#include "CustomFilterGlobalContext.h"
#include "CustomFilterProgram.h"
#include "CustomFilterOperation.h"
#include "CustomFilterValidatedProgram.h"
#include "FECustomFilter.h"
#include "RenderView.h"
#include "Settings.h"
#endif

#if ENABLE(SVG)
#include "CachedSVGDocument.h"
#include "SVGElement.h"
#include "SVGFilterPrimitiveStandardAttributes.h"
#include "SourceAlpha.h"
#endif

namespace WebCore {

static inline void endMatrixRow(Vector<float>& parameters)
{
    parameters.append(0);
    parameters.append(0);
}

static inline void lastMatrixRow(Vector<float>& parameters)
{
    parameters.append(0);
    parameters.append(0);
    parameters.append(0);
    parameters.append(1);
    parameters.append(0);
}

inline bool isFilterSizeValid(FloatRect rect)
{
    if (rect.width() < 0 || rect.width() > kMaxFilterSize
        || rect.height() < 0 || rect.height() > kMaxFilterSize)
        return false;
    return true;
}

#if ENABLE(CSS_SHADERS) && ENABLE(WEBGL)
static bool isCSSCustomFilterEnabled(Document* document)
{
    // We only want to enable shaders if WebGL is also enabled on this platform.
    Settings* settings = document->settings();
    return settings && settings->isCSSCustomFilterEnabled() && settings->webGLEnabled();
}

static PassRefPtr<FECustomFilter> createCustomFilterEffect(Filter* filter, Document* document, CustomFilterOperation* operation)
{
    if (!isCSSCustomFilterEnabled(document))
        return 0;
    
    RefPtr<CustomFilterProgram> program = operation->program();
    if (!program->isLoaded())
        return 0;

    CustomFilterGlobalContext* globalContext = document->renderView()->customFilterGlobalContext();
    globalContext->prepareContextIfNeeded(document->view()->hostWindow());
    RefPtr<CustomFilterValidatedProgram> validatedProgram = globalContext->getValidatedProgram(program->programInfo());
    if (!validatedProgram->isInitialized())
        return 0;

    return FECustomFilter::create(filter, globalContext, validatedProgram, operation->parameters(),
                                  operation->meshRows(), operation->meshColumns(),
                                  operation->meshBoxType(), operation->meshType());
}
#endif

FilterEffectRenderer::FilterEffectRenderer()
    : m_topOutset(0)
    , m_rightOutset(0)
    , m_bottomOutset(0)
    , m_leftOutset(0)
    , m_graphicsBufferAttached(false)
    , m_hasFilterThatMovesPixels(false)
#if ENABLE(CSS_SHADERS)
    , m_hasCustomShaderFilter(false)
#endif
{
    setFilterResolution(FloatSize(1, 1));
    m_sourceGraphic = SourceGraphic::create(this);
}

FilterEffectRenderer::~FilterEffectRenderer()
{
}

GraphicsContext* FilterEffectRenderer::inputContext()
{
    return sourceImage() ? sourceImage()->context() : 0;
}

PassRefPtr<FilterEffect> FilterEffectRenderer::buildReferenceFilter(Document* document, PassRefPtr<FilterEffect> previousEffect, ReferenceFilterOperation* op)
{
#if ENABLE(SVG)
    CachedSVGDocument* cachedSVGDocument = static_cast<CachedSVGDocument*>(op->data());

    // If we have an SVG document, this is an external reference. Otherwise
    // we look up the referenced node in the current document.
    if (cachedSVGDocument)
        document = cachedSVGDocument->document();

    if (!document)
        return 0;

    Element* filter = document->getElementById(op->fragment());
    if (!filter)
        return 0;

    RefPtr<FilterEffect> effect;

    // FIXME: Figure out what to do with SourceAlpha. Right now, we're
    // using the alpha of the original input layer, which is obviously
    // wrong. We should probably be extracting the alpha from the 
    // previousEffect, but this requires some more processing.  
    // This may need a spec clarification.
    RefPtr<SVGFilterBuilder> builder = SVGFilterBuilder::create(previousEffect, SourceAlpha::create(this));

    for (Node* node = filter->firstChild(); node; node = node->nextSibling()) {
        if (!node->isSVGElement())
            continue;

        SVGElement* element = static_cast<SVGElement*>(node);
        if (!element->isFilterEffect())
            continue;

        SVGFilterPrimitiveStandardAttributes* effectElement = static_cast<SVGFilterPrimitiveStandardAttributes*>(element);

        effect = effectElement->build(builder.get(), this);
        if (!effect)
            continue;

        effectElement->setStandardAttributes(effect.get());
        builder->add(effectElement->result(), effect);
        m_effects.append(effect);
    }
    return effect;
#else
    UNUSED_PARAM(document);
    UNUSED_PARAM(previousEffect);
    UNUSED_PARAM(op);
    return 0;
#endif
}

bool FilterEffectRenderer::build(Document* document, const FilterOperations& operations)
{
#if !ENABLE(CSS_SHADERS) || !ENABLE(WEBGL)
    UNUSED_PARAM(document);
#endif

#if ENABLE(CSS_SHADERS)
    m_hasCustomShaderFilter = false;
#endif
    m_hasFilterThatMovesPixels = operations.hasFilterThatMovesPixels();
    if (m_hasFilterThatMovesPixels)
        operations.getOutsets(m_topOutset, m_rightOutset, m_bottomOutset, m_leftOutset);
    
    // Keep the old effects on the stack until we've created the new effects.
    // New FECustomFilters can reuse cached resources from old FECustomFilters.
    FilterEffectList oldEffects;
    m_effects.swap(oldEffects);

    RefPtr<FilterEffect> previousEffect = m_sourceGraphic;
    for (size_t i = 0; i < operations.operations().size(); ++i) {
        RefPtr<FilterEffect> effect;
        FilterOperation* filterOperation = operations.operations().at(i).get();
        switch (filterOperation->getOperationType()) {
        case FilterOperation::REFERENCE: {
            effect = buildReferenceFilter(document, previousEffect, static_cast<ReferenceFilterOperation*>(filterOperation));
            break;
        }
        case FilterOperation::GRAYSCALE: {
            BasicColorMatrixFilterOperation* colorMatrixOperation = static_cast<BasicColorMatrixFilterOperation*>(filterOperation);
            Vector<float> inputParameters;
            double oneMinusAmount = clampTo(1 - colorMatrixOperation->amount(), 0.0, 1.0);

            // See https://dvcs.w3.org/hg/FXTF/raw-file/tip/filters/index.html#grayscaleEquivalent
            // for information on parameters.

            inputParameters.append(narrowPrecisionToFloat(0.2126 + 0.7874 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.7152 - 0.7152 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.0722 - 0.0722 * oneMinusAmount));
            endMatrixRow(inputParameters);

            inputParameters.append(narrowPrecisionToFloat(0.2126 - 0.2126 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.7152 + 0.2848 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.0722 - 0.0722 * oneMinusAmount));
            endMatrixRow(inputParameters);

            inputParameters.append(narrowPrecisionToFloat(0.2126 - 0.2126 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.7152 - 0.7152 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.0722 + 0.9278 * oneMinusAmount));
            endMatrixRow(inputParameters);

            lastMatrixRow(inputParameters);

            effect = FEColorMatrix::create(this, FECOLORMATRIX_TYPE_MATRIX, inputParameters);
            break;
        }
        case FilterOperation::SEPIA: {
            BasicColorMatrixFilterOperation* colorMatrixOperation = static_cast<BasicColorMatrixFilterOperation*>(filterOperation);
            Vector<float> inputParameters;
            double oneMinusAmount = clampTo(1 - colorMatrixOperation->amount(), 0.0, 1.0);

            // See https://dvcs.w3.org/hg/FXTF/raw-file/tip/filters/index.html#sepiaEquivalent
            // for information on parameters.

            inputParameters.append(narrowPrecisionToFloat(0.393 + 0.607 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.769 - 0.769 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.189 - 0.189 * oneMinusAmount));
            endMatrixRow(inputParameters);

            inputParameters.append(narrowPrecisionToFloat(0.349 - 0.349 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.686 + 0.314 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.168 - 0.168 * oneMinusAmount));
            endMatrixRow(inputParameters);

            inputParameters.append(narrowPrecisionToFloat(0.272 - 0.272 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.534 - 0.534 * oneMinusAmount));
            inputParameters.append(narrowPrecisionToFloat(0.131 + 0.869 * oneMinusAmount));
            endMatrixRow(inputParameters);

            lastMatrixRow(inputParameters);

            effect = FEColorMatrix::create(this, FECOLORMATRIX_TYPE_MATRIX, inputParameters);
            break;
        }
        case FilterOperation::SATURATE: {
            BasicColorMatrixFilterOperation* colorMatrixOperation = static_cast<BasicColorMatrixFilterOperation*>(filterOperation);
            Vector<float> inputParameters;
            inputParameters.append(narrowPrecisionToFloat(colorMatrixOperation->amount()));
            effect = FEColorMatrix::create(this, FECOLORMATRIX_TYPE_SATURATE, inputParameters);
            break;
        }
        case FilterOperation::HUE_ROTATE: {
            BasicColorMatrixFilterOperation* colorMatrixOperation = static_cast<BasicColorMatrixFilterOperation*>(filterOperation);
            Vector<float> inputParameters;
            inputParameters.append(narrowPrecisionToFloat(colorMatrixOperation->amount()));
            effect = FEColorMatrix::create(this, FECOLORMATRIX_TYPE_HUEROTATE, inputParameters);
            break;
        }
        case FilterOperation::INVERT: {
            BasicComponentTransferFilterOperation* componentTransferOperation = static_cast<BasicComponentTransferFilterOperation*>(filterOperation);
            ComponentTransferFunction transferFunction;
            transferFunction.type = FECOMPONENTTRANSFER_TYPE_TABLE;
            Vector<float> transferParameters;
            transferParameters.append(narrowPrecisionToFloat(componentTransferOperation->amount()));
            transferParameters.append(narrowPrecisionToFloat(1 - componentTransferOperation->amount()));
            transferFunction.tableValues = transferParameters;

            ComponentTransferFunction nullFunction;
            effect = FEComponentTransfer::create(this, transferFunction, transferFunction, transferFunction, nullFunction);
            break;
        }
        case FilterOperation::OPACITY: {
            BasicComponentTransferFilterOperation* componentTransferOperation = static_cast<BasicComponentTransferFilterOperation*>(filterOperation);
            ComponentTransferFunction transferFunction;
            transferFunction.type = FECOMPONENTTRANSFER_TYPE_TABLE;
            Vector<float> transferParameters;
            transferParameters.append(0);
            transferParameters.append(narrowPrecisionToFloat(componentTransferOperation->amount()));
            transferFunction.tableValues = transferParameters;

            ComponentTransferFunction nullFunction;
            effect = FEComponentTransfer::create(this, nullFunction, nullFunction, nullFunction, transferFunction);
            break;
        }
        case FilterOperation::BRIGHTNESS: {
            BasicComponentTransferFilterOperation* componentTransferOperation = static_cast<BasicComponentTransferFilterOperation*>(filterOperation);
            ComponentTransferFunction transferFunction;
            transferFunction.type = FECOMPONENTTRANSFER_TYPE_LINEAR;
            transferFunction.slope = 1;
            transferFunction.intercept = narrowPrecisionToFloat(componentTransferOperation->amount());

            ComponentTransferFunction nullFunction;
            effect = FEComponentTransfer::create(this, transferFunction, transferFunction, transferFunction, nullFunction);
            break;
        }
        case FilterOperation::CONTRAST: {
            BasicComponentTransferFilterOperation* componentTransferOperation = static_cast<BasicComponentTransferFilterOperation*>(filterOperation);
            ComponentTransferFunction transferFunction;
            transferFunction.type = FECOMPONENTTRANSFER_TYPE_LINEAR;
            float amount = narrowPrecisionToFloat(componentTransferOperation->amount());
            transferFunction.slope = amount;
            transferFunction.intercept = -0.5 * amount + 0.5;
            
            ComponentTransferFunction nullFunction;
            effect = FEComponentTransfer::create(this, transferFunction, transferFunction, transferFunction, nullFunction);
            break;
        }
        case FilterOperation::BLUR: {
            BlurFilterOperation* blurOperation = static_cast<BlurFilterOperation*>(filterOperation);
            float stdDeviation = floatValueForLength(blurOperation->stdDeviation(), 0);
            effect = FEGaussianBlur::create(this, stdDeviation, stdDeviation);
            break;
        }
        case FilterOperation::DROP_SHADOW: {
            DropShadowFilterOperation* dropShadowOperation = static_cast<DropShadowFilterOperation*>(filterOperation);
            effect = FEDropShadow::create(this, dropShadowOperation->stdDeviation(), dropShadowOperation->stdDeviation(),
                                                dropShadowOperation->x(), dropShadowOperation->y(), dropShadowOperation->color(), 1);
            break;
        }
#if ENABLE(CSS_SHADERS) && ENABLE(WEBGL)
        case FilterOperation::CUSTOM: {
            CustomFilterOperation* customFilterOperation = static_cast<CustomFilterOperation*>(filterOperation);
            effect = createCustomFilterEffect(this, document, customFilterOperation);
            if (effect)
                m_hasCustomShaderFilter = true;
            break;
        }
#endif
        default:
            break;
        }

        if (effect) {
            // Unlike SVG, filters applied here should not clip to their primitive subregions.
            effect->setClipsToBounds(false);
            effect->setColorSpace(ColorSpaceDeviceRGB);
            
            if (filterOperation->getOperationType() != FilterOperation::REFERENCE) {
                effect->inputEffects().append(previousEffect);
                m_effects.append(effect);
            }
            previousEffect = effect.release();
        }
    }

    // If we didn't make any effects, tell our caller we are not valid
    if (!m_effects.size())
        return false;

    setMaxEffectRects(m_sourceDrawingRegion);
    
    return true;
}

bool FilterEffectRenderer::updateBackingStoreRect(const FloatRect& filterRect)
{
    if (!filterRect.isZero() && isFilterSizeValid(filterRect)) {
        FloatRect currentSourceRect = sourceImageRect();
        if (filterRect != currentSourceRect) {
            setSourceImageRect(filterRect);
            return true;
        }
    }
    return false;
}

void FilterEffectRenderer::allocateBackingStoreIfNeeded()
{
    // At this point the effect chain has been built, and the
    // source image sizes set. We just need to attach the graphic
    // buffer if we have not yet done so.
    if (!m_graphicsBufferAttached) {
        IntSize logicalSize(m_sourceDrawingRegion.width(), m_sourceDrawingRegion.height());
        if (!sourceImage() || sourceImage()->logicalSize() != logicalSize)
            setSourceImage(ImageBuffer::create(logicalSize, 1, ColorSpaceDeviceRGB, renderingMode()));
        m_graphicsBufferAttached = true;
    }
}

void FilterEffectRenderer::clearIntermediateResults()
{
    m_sourceGraphic->clearResult();
    for (size_t i = 0; i < m_effects.size(); ++i)
        m_effects[i]->clearResult();
}

void FilterEffectRenderer::apply()
{
    lastEffect()->apply();

#if !USE(CG)
    output()->transformColorSpace(lastEffect()->colorSpace(), ColorSpaceDeviceRGB);
#endif
}

LayoutRect FilterEffectRenderer::computeSourceImageRectForDirtyRect(const LayoutRect& filterBoxRect, const LayoutRect& dirtyRect)
{
#if ENABLE(CSS_SHADERS)
    if (hasCustomShaderFilter()) {
        // When we have at least a custom shader in the chain, we need to compute the whole source image, because the shader can
        // reference any pixel and we cannot control that.
        return filterBoxRect;
    }
#endif
    // The result of this function is the area in the "filterBoxRect" that needs to be repainted, so that we fully cover the "dirtyRect".
    LayoutRect rectForRepaint = dirtyRect;
    if (hasFilterThatMovesPixels()) {
        // Note that the outsets are reversed here because we are going backwards -> we have the dirty rect and
        // need to find out what is the rectangle that might influence the result inside that dirty rect.
        rectForRepaint.move(-m_rightOutset, -m_bottomOutset);
        rectForRepaint.expand(m_leftOutset + m_rightOutset, m_topOutset + m_bottomOutset);
    }
    rectForRepaint.intersect(filterBoxRect);
    return rectForRepaint;
}

bool FilterEffectRendererHelper::prepareFilterEffect(RenderLayer* renderLayer, const LayoutRect& filterBoxRect, const LayoutRect& dirtyRect, const LayoutRect& layerRepaintRect)
{
    ASSERT(m_haveFilterEffect && renderLayer->filterRenderer());
    m_renderLayer = renderLayer;
    m_repaintRect = dirtyRect;

    FilterEffectRenderer* filter = renderLayer->filterRenderer();
    LayoutRect filterSourceRect = filter->computeSourceImageRectForDirtyRect(filterBoxRect, dirtyRect);
    m_paintOffset = filterSourceRect.location();

    if (filterSourceRect.isEmpty()) {
        // The dirty rect is not in view, just bail out.
        m_haveFilterEffect = false;
        return false;
    }
    
    bool hasUpdatedBackingStore = filter->updateBackingStoreRect(filterSourceRect);
    if (filter->hasFilterThatMovesPixels()) {
        if (hasUpdatedBackingStore)
            m_repaintRect = filterSourceRect;
        else {
            m_repaintRect.unite(layerRepaintRect);
            m_repaintRect.intersect(filterSourceRect);
        }
    }
    return true;
}
   
GraphicsContext* FilterEffectRendererHelper::beginFilterEffect(GraphicsContext* oldContext)
{
    ASSERT(m_renderLayer);
    
    FilterEffectRenderer* filter = m_renderLayer->filterRenderer();
    filter->allocateBackingStoreIfNeeded();
    // Paint into the context that represents the SourceGraphic of the filter.
    GraphicsContext* sourceGraphicsContext = filter->inputContext();
    if (!sourceGraphicsContext || !isFilterSizeValid(filter->filterRegion())) {
        // Disable the filters and continue.
        m_haveFilterEffect = false;
        return oldContext;
    }
    
    m_savedGraphicsContext = oldContext;
    
    // Translate the context so that the contents of the layer is captuterd in the offscreen memory buffer.
    sourceGraphicsContext->save();
    sourceGraphicsContext->translate(-m_paintOffset.x(), -m_paintOffset.y());
    sourceGraphicsContext->clearRect(m_repaintRect);
    sourceGraphicsContext->clip(m_repaintRect);
    
    return sourceGraphicsContext;
}

GraphicsContext* FilterEffectRendererHelper::applyFilterEffect()
{
    ASSERT(m_haveFilterEffect && m_renderLayer->filterRenderer());
    FilterEffectRenderer* filter = m_renderLayer->filterRenderer();
    filter->inputContext()->restore();

    filter->apply();
    
    // Get the filtered output and draw it in place.
    LayoutRect destRect = filter->outputRect();
    destRect.move(m_paintOffset.x(), m_paintOffset.y());
    
    m_savedGraphicsContext->drawImageBuffer(filter->output(), m_renderLayer->renderer()->style()->colorSpace(), pixelSnappedIntRect(destRect), CompositeSourceOver);
    
    filter->clearIntermediateResults();
    
    return m_savedGraphicsContext;
}

} // namespace WebCore

#endif // ENABLE(CSS_FILTERS)
