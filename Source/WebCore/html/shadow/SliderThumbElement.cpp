/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "SliderThumbElement.h"

#include "CSSValueKeywords.h"
#include "Event.h"
#include "EventHandler.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLParserIdioms.h"
#include "MouseEvent.h"
#include "RenderFlexibleBox.h"
#include "RenderSlider.h"
#include "RenderTheme.h"
#include "ShadowRoot.h"
#include "StepRange.h"

namespace WebCore {

using namespace HTMLNames;

inline static Decimal sliderPosition(HTMLInputElement* element)
{
    const StepRange stepRange(element->createStepRange(RejectAny));
    const Decimal oldValue = parseToDecimalForNumberType(element->value(), stepRange.defaultValue());
    return stepRange.proportionFromValue(stepRange.clampValue(oldValue));
}

inline static bool hasVerticalAppearance(HTMLInputElement* input)
{
    ASSERT(input->renderer());
    const RenderStyle& sliderStyle = input->renderer()->style();

#if ENABLE(VIDEO)
    if (sliderStyle.appearance() == MediaVolumeSliderPart && input->renderer()->theme()->usesVerticalVolumeSlider())
        return true;
#endif

    return sliderStyle.appearance() == SliderVerticalPart;
}

// --------------------------------

RenderSliderThumb::RenderSliderThumb(SliderThumbElement& element, PassRef<RenderStyle> style)
    : RenderBlockFlow(element, std::move(style))
{
}

void RenderSliderThumb::updateAppearance(RenderStyle* parentStyle)
{
    if (parentStyle->appearance() == SliderVerticalPart)
        style().setAppearance(SliderThumbVerticalPart);
    else if (parentStyle->appearance() == SliderHorizontalPart)
        style().setAppearance(SliderThumbHorizontalPart);
    else if (parentStyle->appearance() == MediaSliderPart)
        style().setAppearance(MediaSliderThumbPart);
    else if (parentStyle->appearance() == MediaVolumeSliderPart)
        style().setAppearance(MediaVolumeSliderThumbPart);
    else if (parentStyle->appearance() == MediaFullScreenVolumeSliderPart)
        style().setAppearance(MediaFullScreenVolumeSliderThumbPart);
    if (style().hasAppearance())
        theme()->adjustSliderThumbSize(&style(), element());
}

bool RenderSliderThumb::isSliderThumb() const
{
    return true;
}

// --------------------------------

// FIXME: Find a way to cascade appearance and adjust heights, and get rid of this class.
// http://webkit.org/b/62535
class RenderSliderContainer : public RenderFlexibleBox {
public:
    RenderSliderContainer(SliderContainerElement& element, PassRef<RenderStyle> style)
        : RenderFlexibleBox(element, std::move(style))
    {
    }

public:
    virtual void computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues&) const OVERRIDE;

private:
    virtual void layout() OVERRIDE;
};

void RenderSliderContainer::computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues& computedValues) const
{
    HTMLInputElement* input = element()->shadowHost()->toInputElement();
    bool isVertical = hasVerticalAppearance(input);

#if ENABLE(DATALIST_ELEMENT)
    if (input->renderer()->isSlider() && !isVertical && input->list()) {
        int offsetFromCenter = theme()->sliderTickOffsetFromTrackCenter();
        LayoutUnit trackHeight = 0;
        if (offsetFromCenter < 0)
            trackHeight = -2 * offsetFromCenter;
        else {
            int tickLength = theme()->sliderTickSize().height();
            trackHeight = 2 * (offsetFromCenter + tickLength);
        }
        float zoomFactor = style().effectiveZoom();
        if (zoomFactor != 1.0)
            trackHeight *= zoomFactor;

        RenderBox::computeLogicalHeight(trackHeight, logicalTop, computedValues);
        return;
    }
#endif
    if (isVertical)
        logicalHeight = RenderSlider::defaultTrackLength;
    RenderBox::computeLogicalHeight(logicalHeight, logicalTop, computedValues);
}

void RenderSliderContainer::layout()
{
    HTMLInputElement* input = element()->shadowHost()->toInputElement();
    bool isVertical = hasVerticalAppearance(input);
    style().setFlexDirection(isVertical ? FlowColumn : FlowRow);
    TextDirection oldTextDirection = style().direction();
    if (isVertical) {
        // FIXME: Work around rounding issues in RTL vertical sliders. We want them to
        // render identically to LTR vertical sliders. We can remove this work around when
        // subpixel rendering is enabled on all ports.
        style().setDirection(LTR);
    }

    RenderBox* thumb = input->sliderThumbElement() ? input->sliderThumbElement()->renderBox() : 0;
    RenderBox* track = input->sliderTrackElement() ? input->sliderTrackElement()->renderBox() : 0;
    // Force a layout to reset the position of the thumb so the code below doesn't move the thumb to the wrong place.
    // FIXME: Make a custom Render class for the track and move the thumb positioning code there.
    if (track)
        track->setChildNeedsLayout(MarkOnlyThis);

    RenderFlexibleBox::layout();

    style().setDirection(oldTextDirection);
    // These should always exist, unless someone mutates the shadow DOM (e.g., in the inspector).
    if (!thumb || !track)
        return;

    double percentageOffset = sliderPosition(input).toDouble();
    LayoutUnit availableExtent = isVertical ? track->contentHeight() : track->contentWidth();
    availableExtent -= isVertical ? thumb->height() : thumb->width();
    LayoutUnit offset = percentageOffset * availableExtent;
    LayoutPoint thumbLocation = thumb->location();
    if (isVertical)
        thumbLocation.setY(thumbLocation.y() + track->contentHeight() - thumb->height() - offset);
    else if (style().isLeftToRightDirection())
        thumbLocation.setX(thumbLocation.x() + offset);
    else
        thumbLocation.setX(thumbLocation.x() - offset);
    thumb->setLocation(thumbLocation);
}

// --------------------------------

SliderThumbElement::SliderThumbElement(Document& document)
    : HTMLDivElement(HTMLNames::divTag, document)
    , m_inDragMode(false)
{
    setHasCustomStyleResolveCallbacks();
}

void SliderThumbElement::setPositionFromValue()
{
    // Since the code to calculate position is in the RenderSliderThumb layout
    // path, we don't actually update the value here. Instead, we poke at the
    // renderer directly to trigger layout.
    if (renderer())
        renderer()->setNeedsLayout();
}

RenderElement* SliderThumbElement::createRenderer(PassRef<RenderStyle> style)
{
    return new RenderSliderThumb(*this, std::move(style));
}

bool SliderThumbElement::isDisabledFormControl() const
{
    HTMLInputElement* input = hostInput();
    return !input || input->isDisabledFormControl();
}

bool SliderThumbElement::matchesReadOnlyPseudoClass() const
{
    HTMLInputElement* input = hostInput();
    return input && input->matchesReadOnlyPseudoClass();
}

bool SliderThumbElement::matchesReadWritePseudoClass() const
{
    HTMLInputElement* input = hostInput();
    return input && input->matchesReadWritePseudoClass();
}

Element* SliderThumbElement::focusDelegate()
{
    return hostInput();
}

void SliderThumbElement::dragFrom(const LayoutPoint& point)
{
    setPositionFromPoint(point);
    startDragging();
}

void SliderThumbElement::setPositionFromPoint(const LayoutPoint& absolutePoint)
{
    RefPtr<HTMLInputElement> input = hostInput();
    if (!input || !input->renderer() || !renderBox())
        return;

    HTMLElement* trackElement = input->sliderTrackElement();
    if (!trackElement->renderBox())
        return;

    input->setTextAsOfLastFormControlChangeEvent(input->value());

    // Do all the tracking math relative to the input's renderer's box.
    RenderBox& inputRenderer = *toRenderBox(input->renderer());
    RenderBox& trackRenderer = *trackElement->renderBox();

    bool isVertical = hasVerticalAppearance(input.get());
    bool isLeftToRightDirection = renderBox()->style().isLeftToRightDirection();
    
    LayoutPoint offset = roundedLayoutPoint(inputRenderer.absoluteToLocal(absolutePoint, UseTransforms));
    FloatRect trackBoundingBox = trackRenderer.localToContainerQuad(FloatRect(0, 0, trackRenderer.width(), trackRenderer.height()), &inputRenderer).enclosingBoundingBox();

    LayoutUnit trackLength;
    LayoutUnit position;
    if (isVertical) {
        trackLength = trackRenderer.contentHeight() - renderBox()->height();
        position = offset.y() - renderBox()->height() / 2 - trackBoundingBox.y() - renderBox()->marginBottom();
    } else {
        trackLength = trackRenderer.contentWidth() - renderBox()->width();
        position = offset.x() - renderBox()->width() / 2 - trackBoundingBox.x();
        position -= isLeftToRightDirection ? renderBox()->marginLeft() : renderBox()->marginRight();
    }

    position = std::max<LayoutUnit>(0, std::min(position, trackLength));
    const Decimal ratio = Decimal::fromDouble(static_cast<double>(position) / trackLength);
    const Decimal fraction = isVertical || !isLeftToRightDirection ? Decimal(1) - ratio : ratio;
    StepRange stepRange(input->createStepRange(RejectAny));
    Decimal value = stepRange.clampValue(stepRange.valueFromProportion(fraction));

#if ENABLE(DATALIST_ELEMENT)
    const LayoutUnit snappingThreshold = renderer()->theme()->sliderTickSnappingThreshold();
    if (snappingThreshold > 0) {
        Decimal closest = input->findClosestTickMarkValue(value);
        if (closest.isFinite()) {
            double closestFraction = stepRange.proportionFromValue(closest).toDouble();
            double closestRatio = isVertical || !isLeftToRightDirection ? 1.0 - closestFraction : closestFraction;
            LayoutUnit closestPosition = trackLength * closestRatio;
            if ((closestPosition - position).abs() <= snappingThreshold)
                value = closest;
        }
    }
#endif

    String valueString = serializeForNumberType(value);
    if (valueString == input->value())
        return;

    // FIXME: This is no longer being set from renderer. Consider updating the method name.
    input->setValueFromRenderer(valueString);
    if (renderer())
        renderer()->setNeedsLayout();
    input->dispatchFormControlChangeEvent();
}

void SliderThumbElement::startDragging()
{
    if (Frame* frame = document().frame()) {
        frame->eventHandler().setCapturingMouseEventsElement(this);
        m_inDragMode = true;
    }
}

void SliderThumbElement::stopDragging()
{
    if (!m_inDragMode)
        return;

    if (Frame* frame = document().frame())
        frame->eventHandler().setCapturingMouseEventsElement(nullptr);
    m_inDragMode = false;
    if (renderer())
        renderer()->setNeedsLayout();
}

void SliderThumbElement::defaultEventHandler(Event* event)
{
    if (!event->isMouseEvent()) {
        HTMLDivElement::defaultEventHandler(event);
        return;
    }

    // FIXME: Should handle this readonly/disabled check in more general way.
    // Missing this kind of check is likely to occur elsewhere if adding it in each shadow element.
    HTMLInputElement* input = hostInput();
    if (!input || input->isDisabledOrReadOnly()) {
        stopDragging();
        HTMLDivElement::defaultEventHandler(event);
        return;
    }

    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
    bool isLeftButton = mouseEvent->button() == LeftButton;
    const AtomicString& eventType = event->type();

    // We intentionally do not call event->setDefaultHandled() here because
    // MediaControlTimelineElement::defaultEventHandler() wants to handle these
    // mouse events.
    if (eventType == eventNames().mousedownEvent && isLeftButton) {
        startDragging();
        return;
    } else if (eventType == eventNames().mouseupEvent && isLeftButton) {
        stopDragging();
        return;
    } else if (eventType == eventNames().mousemoveEvent) {
        if (m_inDragMode)
            setPositionFromPoint(mouseEvent->absoluteLocation());
        return;
    }

    HTMLDivElement::defaultEventHandler(event);
}

bool SliderThumbElement::willRespondToMouseMoveEvents()
{
    const HTMLInputElement* input = hostInput();
    if (input && !input->isDisabledOrReadOnly() && m_inDragMode)
        return true;

    return HTMLDivElement::willRespondToMouseMoveEvents();
}

bool SliderThumbElement::willRespondToMouseClickEvents()
{
    const HTMLInputElement* input = hostInput();
    if (input && !input->isDisabledOrReadOnly())
        return true;

    return HTMLDivElement::willRespondToMouseClickEvents();
}

void SliderThumbElement::willDetachRenderers()
{
    if (m_inDragMode) {
        if (Frame* frame = document().frame())
            frame->eventHandler().setCapturingMouseEventsElement(nullptr);
    }
}

HTMLInputElement* SliderThumbElement::hostInput() const
{
    // Only HTMLInputElement creates SliderThumbElement instances as its shadow nodes.
    // So, shadowHost() must be an HTMLInputElement.
    Element* host = shadowHost();
    return host ? host->toInputElement() : 0;
}

static const AtomicString& sliderThumbShadowPseudoId()
{
    DEFINE_STATIC_LOCAL(const AtomicString, sliderThumb, ("-webkit-slider-thumb", AtomicString::ConstructFromLiteral));
    return sliderThumb;
}

static const AtomicString& mediaSliderThumbShadowPseudoId()
{
    DEFINE_STATIC_LOCAL(const AtomicString, mediaSliderThumb, ("-webkit-media-slider-thumb", AtomicString::ConstructFromLiteral));
    return mediaSliderThumb;
}

const AtomicString& SliderThumbElement::shadowPseudoId() const
{
    HTMLInputElement* input = hostInput();
    if (!input)
        return sliderThumbShadowPseudoId();

    const RenderStyle& sliderStyle = input->renderer()->style();
    switch (sliderStyle.appearance()) {
    case MediaSliderPart:
    case MediaSliderThumbPart:
    case MediaVolumeSliderPart:
    case MediaVolumeSliderThumbPart:
    case MediaFullScreenVolumeSliderPart:
    case MediaFullScreenVolumeSliderThumbPart:
        return mediaSliderThumbShadowPseudoId();
    default:
        return sliderThumbShadowPseudoId();
    }
}

PassRefPtr<Element> SliderThumbElement::cloneElementWithoutAttributesAndChildren()
{
    return create(document());
}

// --------------------------------

inline SliderContainerElement::SliderContainerElement(Document& document)
    : HTMLDivElement(HTMLNames::divTag, document)
{
}

PassRefPtr<SliderContainerElement> SliderContainerElement::create(Document& document)
{
    return adoptRef(new SliderContainerElement(document));
}

RenderElement* SliderContainerElement::createRenderer(PassRef<RenderStyle> style)
{
    return new RenderSliderContainer(*this, std::move(style));
}

const AtomicString& SliderContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, mediaSliderContainer, ("-webkit-media-slider-container", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, sliderContainer, ("-webkit-slider-container", AtomicString::ConstructFromLiteral));

    HTMLInputElement* input = shadowHost()->toInputElement();
    if (!input)
        return sliderContainer;

    const RenderStyle& sliderStyle = input->renderer()->style();
    switch (sliderStyle.appearance()) {
    case MediaSliderPart:
    case MediaSliderThumbPart:
    case MediaVolumeSliderPart:
    case MediaVolumeSliderThumbPart:
    case MediaFullScreenVolumeSliderPart:
    case MediaFullScreenVolumeSliderThumbPart:
        return mediaSliderContainer;
    default:
        return sliderContainer;
    }
}

}
