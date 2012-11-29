/*
 * Copyright (C) 2006, 2007 Apple Inc.
 * Copyright (C) 2009 Google Inc.
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "RenderThemeBlackBerry.h"

#include "CSSValueKeywords.h"
#include "Frame.h"
#include "HTMLMediaElement.h"
#include "HostWindow.h"
#include "MediaControlElements.h"
#include "MediaPlayerPrivateBlackBerry.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderFullScreen.h"
#include "RenderProgress.h"
#include "RenderSlider.h"
#include "RenderView.h"
#include "UserAgentStyleSheets.h"

#include <BlackBerryPlatformLog.h>

namespace WebCore {

// Sizes (unit px)
const unsigned smallRadius = 1;
const unsigned largeRadius = 3;
const unsigned lineWidth = 1;
const float marginSize = 4;
const float mediaControlsHeight = 32;
const float mediaSliderThumbWidth = 40;
const float mediaSliderThumbHeight = 13;
const float mediaSliderThumbRadius = 5;
const float sliderThumbWidth = 15;
const float sliderThumbHeight = 25;

// Checkbox check scalers
const float checkboxLeftX = 7 / 40.0;
const float checkboxLeftY = 1 / 2.0;
const float checkboxMiddleX = 19 / 50.0;
const float checkboxMiddleY = 7 / 25.0;
const float checkboxRightX = 33 / 40.0;
const float checkboxRightY = 1 / 5.0;
const float checkboxStrokeThickness = 6.5;

// Radio button scaler
const float radioButtonCheckStateScaler = 7 / 30.0;

// Multipliers
const unsigned paddingDivisor = 10;
const unsigned fullScreenEnlargementFactor = 2;
const float scaleFactorThreshold = 2.0;

// Slice length
const int smallSlice = 8;
const int mediumSlice = 10;
const int largeSlice = 13;

// Slider Aura, calculated from UX spec
const float auraRatio = 1.62;

// Dropdown arrow position, calculated from UX spec
const float xPositionRatio = 3;
const float yPositionRatio = 0.38;
const float widthRatio = 3;
const float heightRatio = 0.23;

// Colors
const RGBA32 caretBottom = 0xff2163bf;
const RGBA32 caretTop = 0xff69a5fa;

const RGBA32 regularBottom = 0xffdcdee4;
const RGBA32 regularTop = 0xfff7f2ee;
const RGBA32 hoverBottom = 0xffb5d3fc;
const RGBA32 hoverTop = 0xffcceaff;
const RGBA32 depressedBottom = 0xff3388ff;
const RGBA32 depressedTop = 0xff66a0f2;
const RGBA32 disabledBottom = 0xffe7e7e7;
const RGBA32 disabledTop = 0xffefefef;

const RGBA32 regularBottomOutline = 0xff6e7073;
const RGBA32 regularTopOutline = 0xffb9b8b8;
const RGBA32 hoverBottomOutline = 0xff2163bf;
const RGBA32 hoverTopOutline = 0xff69befa;
const RGBA32 depressedBottomOutline = 0xff0c3d81;
const RGBA32 depressedTopOutline = 0xff1d4d70;
const RGBA32 disabledOutline = 0xffd5d9de;

const RGBA32 progressRegularBottom = caretTop;
const RGBA32 progressRegularTop = caretBottom;

const RGBA32 rangeSliderRegularBottom = 0xfff6f2ee;
const RGBA32 rangeSliderRegularTop = 0xffdee0e5;
const RGBA32 rangeSliderRollBottom = 0xffc9e8fe;
const RGBA32 rangeSliderRollTop = 0xffb5d3fc;

const RGBA32 rangeSliderRegularBottomOutline = 0xffb9babd;
const RGBA32 rangeSliderRegularTopOutline = 0xffb7b7b7;
const RGBA32 rangeSliderRollBottomOutline = 0xff67abe0;
const RGBA32 rangeSliderRollTopOutline = 0xff69adf9;

const RGBA32 dragRegularLight = 0xfffdfdfd;
const RGBA32 dragRegularDark = 0xffbababa;
const RGBA32 dragRollLight = 0xfff2f2f2;
const RGBA32 dragRollDark = 0xff69a8ff;

const RGBA32 blackPen = Color::black;
const RGBA32 focusRingPen = 0xffa3c8fe;

float RenderThemeBlackBerry::defaultFontSize = 16;

const String& RenderThemeBlackBerry::defaultGUIFont()
{
    DEFINE_STATIC_LOCAL(String, fontFace, (ASCIILiteral("Slate Pro")));
    return fontFace;
}

static PassRefPtr<Gradient> createLinearGradient(RGBA32 top, RGBA32 bottom, const IntPoint& a, const IntPoint& b)
{
    RefPtr<Gradient> gradient = Gradient::create(a, b);
    gradient->addColorStop(0.0, Color(top));
    gradient->addColorStop(1.0, Color(bottom));
    return gradient.release();
}

static RenderSlider* determineRenderSlider(RenderObject* object)
{
    ASSERT(object->isSliderThumb());
    // The RenderSlider is an ancestor of the slider thumb.
    while (object && !object->isSlider())
        object = object->parent();
    return toRenderSlider(object);
}

static float determineFullScreenMultiplier(Element* element)
{
    float fullScreenMultiplier = 1.0;
#if ENABLE(FULLSCREEN_API) && ENABLE(VIDEO)
    if (element && element->document()->webkitIsFullScreen() && element->document()->webkitCurrentFullScreenElement() == toParentMediaElement(element)) {
        if (element->document()->page()->deviceScaleFactor() < scaleFactorThreshold)
            fullScreenMultiplier = fullScreenEnlargementFactor;

        // The way the BlackBerry port implements the FULLSCREEN_API for media elements
        // might result in the controls being oversized, proportionally to the current page
        // scale. That happens because the fullscreen element gets sized to be as big as the
        // viewport size, and the viewport size might get outstretched to fit to the screen dimensions.
        // To fix that, lets strips out the Page scale factor from the media controls multiplier.
        float scaleFactor = element->document()->view()->hostWindow()->platformPageClient()->currentZoomFactor();
        static ViewportArguments defaultViewportArguments;
        float scaleFactorFudge = 1 / element->document()->page()->deviceScaleFactor();
        fullScreenMultiplier /= scaleFactor * scaleFactorFudge;
    }
#endif
    return fullScreenMultiplier;
}

static void drawControl(GraphicsContext* gc, const FloatRect& rect, Image* img)
{
    if (!img)
        return;
    FloatRect srcRect(0, 0, img->width(), img->height());
    gc->drawImage(img, ColorSpaceDeviceRGB, rect, srcRect);
}

static void drawThreeSliceHorizontal(GraphicsContext* gc, const IntRect& rect, Image* img, int slice)
{
    if (!img)
        return;

    FloatSize dstSlice(rect.height() / 2, rect.height());
    FloatRect srcRect(0, 0, slice, img->height());
    FloatRect dstRect(rect.location(), dstSlice);

    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(img->width() - srcRect.width(), 0);
    dstRect.move(rect.width() - dstRect.width(), 0);
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);

    srcRect = FloatRect(slice, 0, img->width() - 2 * slice, img->height());
    dstRect = FloatRect(rect.x() + dstSlice.width(), rect.y(), rect.width() - 2 * dstSlice.width(), dstSlice.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
}

static void drawThreeSliceVertical(GraphicsContext* gc, const IntRect& rect, Image* img, int slice)
{
    if (!img)
        return;

    FloatSize dstSlice(rect.width(), rect.width() / 2);
    FloatRect srcRect(0, 0, img->width(), slice);
    FloatRect dstRect(rect.location(), dstSlice);

    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(0, img->height() - srcRect.height());
    dstRect.move(0, rect.height() - dstRect.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);

    srcRect = FloatRect(0, slice, img->width(), img->height() - 2 * slice);
    dstRect = FloatRect(rect.x(), rect.y() + dstSlice.height(), dstSlice.width(), rect.height() - 2 * dstSlice.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
}

static void drawNineSlice(GraphicsContext* gc, const IntRect& rect, double scale, Image* img, int slice)
{
    if (!img)
        return;
    if (rect.height() * scale < 101.0)
        scale = 101.0 / rect.height();
    FloatSize dstSlice(slice / scale, slice / scale);
    FloatRect srcRect(0, 0, slice, slice);
    FloatRect dstRect(rect.location(), dstSlice);
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(img->width() - srcRect.width(), 0);
    dstRect.move(rect.width() - dstRect.width(), 0);
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(0, img->height() - srcRect.height());
    dstRect.move(0, rect.height() - dstRect.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(-(img->width() - srcRect.width()), 0);
    dstRect.move(-(rect.width() - dstRect.width()), 0);
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);

    srcRect = FloatRect(slice, 0, img->width() - 2 * slice, slice);
    dstRect = FloatRect(rect.x() + dstSlice.width(), rect.y(), rect.width() - 2 * dstSlice.width(), dstSlice.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(0, img->height() - srcRect.height());
    dstRect.move(0, rect.height() - dstRect.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);

    srcRect = FloatRect(0, slice, slice, img->height() - 2 * slice);
    dstRect = FloatRect(rect.x(), rect.y() + dstSlice.height(), dstSlice.width(), rect.height() - 2 * dstSlice.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
    srcRect.move(img->width() - srcRect.width(), 0);
    dstRect.move(rect.width() - dstRect.width(), 0);
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);

    srcRect = FloatRect(slice, slice, img->width() - 2 * slice, img->height() - 2 * slice);
    dstRect = FloatRect(rect.x() + dstSlice.width(), rect.y() + dstSlice.height(), rect.width() - 2 * dstSlice.width(), rect.height() - 2 * dstSlice.height());
    gc->drawImage(img, ColorSpaceDeviceRGB, dstRect, srcRect);
}

static RefPtr<Image> loadImage(const char* filename)
{
    RefPtr<Image> resource;
    resource = Image::loadPlatformResource(filename).leakRef();
    if (!resource) {
        BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelWarn, "RenderThemeBlackBerry failed to load %s.png", filename);
        return 0;
    }
    return resource;
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page* page)
{
    static RenderTheme* theme = RenderThemeBlackBerry::create().leakRef();
    return theme;
}

PassRefPtr<RenderTheme> RenderThemeBlackBerry::create()
{
    return adoptRef(new RenderThemeBlackBerry());
}

RenderThemeBlackBerry::RenderThemeBlackBerry()
{
}

RenderThemeBlackBerry::~RenderThemeBlackBerry()
{
}

String RenderThemeBlackBerry::extraDefaultStyleSheet()
{
    return String(themeBlackBerryUserAgentStyleSheet, sizeof(themeBlackBerryUserAgentStyleSheet));
}

#if ENABLE(VIDEO)
String RenderThemeBlackBerry::extraMediaControlsStyleSheet()
{
    return String(mediaControlsBlackBerryUserAgentStyleSheet, sizeof(mediaControlsBlackBerryUserAgentStyleSheet));
}

String RenderThemeBlackBerry::formatMediaControlsRemainingTime(float, float duration) const
{
    // This is a workaround to make the appearance of media time controller in
    // in-page mode the same as in fullscreen mode.
    return formatMediaControlsTime(duration);
}
#endif

double RenderThemeBlackBerry::caretBlinkInterval() const
{
    return 0; // Turn off caret blinking.
}

void RenderThemeBlackBerry::systemFont(int propId, FontDescription& fontDescription) const
{
    float fontSize = defaultFontSize;

    // Both CSSValueWebkitControl and CSSValueWebkitSmallControl should use default font size which looks better on the controls.
    if (propId == CSSValueWebkitMiniControl) {
        // Why 2 points smaller? Because that's what Gecko does. Note that we
        // are assuming a 96dpi screen, which is the default value we use on Windows.
        static const float pointsPerInch = 72.0f;
        static const float pixelsPerInch = 96.0f;
        fontSize -= (2.0f / pointsPerInch) * pixelsPerInch;
    }

    fontDescription.firstFamily().setFamily(defaultGUIFont());
    fontDescription.setSpecifiedSize(fontSize);
    fontDescription.setIsAbsoluteSize(true);
    fontDescription.setGenericFamily(FontDescription::NoFamily);
    fontDescription.setWeight(FontWeightNormal);
    fontDescription.setItalic(false);
}

void RenderThemeBlackBerry::setButtonStyle(RenderStyle* style) const
{
    Length vertPadding(int(style->fontSize() / paddingDivisor), Fixed);
    style->setPaddingTop(vertPadding);
    style->setPaddingBottom(vertPadding);
}

void RenderThemeBlackBerry::adjustButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    setButtonStyle(style);
    style->setCursor(CURSOR_WEBKIT_GRAB);
}

void RenderThemeBlackBerry::adjustTextAreaStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    setButtonStyle(style);
}

bool RenderThemeBlackBerry::paintTextArea(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintTextFieldOrTextAreaOrSearchField(object, info, rect);
}

void RenderThemeBlackBerry::adjustTextFieldStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    setButtonStyle(style);
}

bool RenderThemeBlackBerry::paintTextFieldOrTextAreaOrSearchField(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    GraphicsContext* context = info.context;

    static RefPtr<Image> bg, bgDisabled, bgHighlight;
    if (!bg) {
        bg = loadImage("core_textinput_bg");
        bgDisabled = loadImage("core_textinput_bg_disabled");
        bgHighlight = loadImage("core_textinput_bg_highlight");
    }

    AffineTransform ctm = context->getCTM();
    if (isEnabled(object) && bg)
        drawNineSlice(context, rect, ctm.xScale(), bg.get(), smallSlice);
    if (!isEnabled(object) && bgDisabled)
        drawNineSlice(context, rect, ctm.xScale(), bgDisabled.get(), smallSlice);

    if ((isHovered(object) || isFocused(object) || isPressed(object)) && bgHighlight)
        drawNineSlice(context, rect, ctm.xScale(), bgHighlight.get(), smallSlice);

    return false;
}

bool RenderThemeBlackBerry::paintTextField(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintTextFieldOrTextAreaOrSearchField(object, info, rect);
}

void RenderThemeBlackBerry::adjustSearchFieldStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    setButtonStyle(style);
}

void RenderThemeBlackBerry::adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    static const float defaultControlFontPixelSize = 10;
    static const float defaultCancelButtonSize = 13;
    static const float minCancelButtonSize = 5;

    // Scale the button size based on the font size
    float fontScale = style->fontSize() / defaultControlFontPixelSize;
    int cancelButtonSize = lroundf(std::max(minCancelButtonSize, defaultCancelButtonSize * fontScale));
    Length length(cancelButtonSize, Fixed);
    style->setWidth(length);
    style->setHeight(length);
}

bool RenderThemeBlackBerry::paintSearchField(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintTextFieldOrTextAreaOrSearchField(object, info, rect);
}

IntRect RenderThemeBlackBerry::convertToPaintingRect(RenderObject* inputRenderer, const RenderObject* partRenderer, LayoutRect partRect, const IntRect& localOffset) const
{
    // Compute an offset between the part renderer and the input renderer.
    LayoutSize offsetFromInputRenderer = -partRenderer->offsetFromAncestorContainer(inputRenderer);
    // Move the rect into partRenderer's coords.
    partRect.move(offsetFromInputRenderer);
    // Account for the local drawing offset.
    partRect.move(localOffset.x(), localOffset.y());

    return pixelSnappedIntRect(partRect);
}


bool RenderThemeBlackBerry::paintSearchFieldCancelButton(RenderObject* cancelButtonObject, const PaintInfo& paintInfo, const IntRect& r)
{
    Node* input = cancelButtonObject->node()->shadowAncestorNode();
    if (!input->renderer()->isBox())
        return false;

    RenderBox* inputRenderBox = toRenderBox(input->renderer());
    LayoutRect inputContentBox = inputRenderBox->contentBoxRect();

    // Make sure the scaled button stays square and will fit in its parent's box.
    LayoutUnit cancelButtonSize = std::min(inputContentBox.width(), std::min<LayoutUnit>(inputContentBox.height(), r.height()));
    // Calculate cancel button's coordinates relative to the input element.
    // Center the button vertically. Round up though, so if it has to be one pixel off-center, it will
    // be one pixel closer to the bottom of the field. This tends to look better with the text.
    LayoutRect cancelButtonRect(cancelButtonObject->offsetFromAncestorContainer(inputRenderBox).width(),
                                inputContentBox.y() + (inputContentBox.height() - cancelButtonSize + 1) / 2,
                                cancelButtonSize, cancelButtonSize);
    IntRect paintingRect = convertToPaintingRect(inputRenderBox, cancelButtonObject, cancelButtonRect, r);

    static Image* cancelImage = Image::loadPlatformResource("searchCancel").leakRef();
    static Image* cancelPressedImage = Image::loadPlatformResource("searchCancelPressed").leakRef();
    paintInfo.context->drawImage(isPressed(cancelButtonObject) ? cancelPressedImage : cancelImage,
                                 cancelButtonObject->style()->colorSpace(), paintingRect);
    return false;
}

void RenderThemeBlackBerry::adjustMenuListButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // These seem to be reasonable padding values from observation.
    const int paddingLeft = 8;
    const int paddingRight = 4;

    const int minHeight = style->fontSize() * 2;

    style->resetPadding();
    style->setMinHeight(Length(minHeight, Fixed));
    style->setLineHeight(RenderStyle::initialLineHeight());

    style->setPaddingRight(Length(minHeight + paddingRight, Fixed));
    style->setPaddingLeft(Length(paddingLeft, Fixed));
    style->setCursor(CURSOR_WEBKIT_GRAB);
}

void RenderThemeBlackBerry::calculateButtonSize(RenderStyle* style) const
{
    int size = style->fontSize();
    Length length(size, Fixed);
    if (style->appearance() == CheckboxPart || style->appearance() == RadioPart) {
        style->setWidth(length);
        style->setHeight(length);
        return;
    }

    // If the width and height are both specified, then we have nothing to do.
    if (!style->width().isIntrinsicOrAuto() && !style->height().isAuto())
        return;

    if (style->width().isIntrinsicOrAuto())
        style->setWidth(length);

    if (style->height().isAuto())
        style->setHeight(length);
}

bool RenderThemeBlackBerry::paintCheckbox(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled, background, inactive, pressed, active, activeMark, disableMark;
    if (!disabled) {
        disabled = loadImage("core_checkbox_disabled");
        background = loadImage("core_checkbox_moat");
        inactive = loadImage("core_checkbox_inactive");
        pressed = loadImage("core_checkbox_pressed");
        active = loadImage("core_checkbox_active");
        activeMark = loadImage("core_checkbox_active_mark");
        disableMark = loadImage("core_checkbox_disabled_mark");
    }

    // Caculate where to put center checkmark.
    FloatRect tmpRect(rect);

    float centerX = ((float(inactive->width()) - float(activeMark->width())) / float(inactive->width()) * tmpRect.width() / 2) + tmpRect.x();
    float centerY = ((float(inactive->height()) - float(activeMark->height())) / float(inactive->height()) * tmpRect.height() / 2) + tmpRect.y();
    float width = float(activeMark->width()) / float(inactive->width()) * tmpRect.width();
    float height = float(activeMark->height()) / float(inactive->height()) * tmpRect.height();
    FloatRect centerRect(centerX, centerY, width, height);

    drawControl(context, rect, background.get());

    if (isEnabled(object)) {
        if (isPressed(object)) {
            drawControl(context, rect, pressed.get());
            if (isChecked(object)) {
                // FIXME: need opacity 30% on activeMark
                drawControl(context, centerRect, activeMark.get());
            }
        } else {
            drawControl(context, rect, inactive.get());
            if (isChecked(object)) {
                drawControl(context, rect, active.get());
                drawControl(context, centerRect, activeMark.get());
            }
        }
    } else {
        drawControl(context, rect, disabled.get());
        if (isChecked(object))
            drawControl(context, rect, disableMark.get());
    }
    return false;
}

void RenderThemeBlackBerry::setCheckboxSize(RenderStyle* style) const
{
    calculateButtonSize(style);
}

bool RenderThemeBlackBerry::paintRadio(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled, disabledActive, inactive, pressed, active, activeMark;
    if (!disabled) {
        disabled = loadImage("core_radiobutton_disabled");
        disabledActive = loadImage("core_radiobutton_disabled_active");
        inactive = loadImage("core_radiobutton_inactive");
        pressed = loadImage("core_radiobutton_pressed");
        active = loadImage("core_radiobutton_active");
        activeMark = loadImage("core_radiobutton_active_mark");
    }

    // Caculate where to put center circle.
    FloatRect tmpRect(rect);

    float centerX = ((float(inactive->width()) - float(activeMark->width())) / float(inactive->width()) * tmpRect.width() / 2)+ tmpRect.x();
    float centerY = ((float(inactive->height()) - float(activeMark->height())) / float(inactive->height()) * tmpRect.height() / 2) + tmpRect.y();
    float width = float(activeMark->width()) / float(inactive->width()) * tmpRect.width();
    float height = float(activeMark->height()) / float(inactive->height()) * tmpRect.height();
    FloatRect centerRect(centerX, centerY, width, height);

    if (isEnabled(object)) {
        if (isPressed(object)) {
            drawControl(context, rect, pressed.get());
            if (isChecked(object)) {
                // FIXME: need opacity 30% on activeMark
                drawControl(context, centerRect, activeMark.get());
            }
        } else {
            drawControl(context, rect, inactive.get());
            if (isChecked(object)) {
                drawControl(context, rect, active.get());
                drawControl(context, centerRect, activeMark.get());
            }
        }
    } else {
        drawControl(context, rect, inactive.get());
        if (isChecked(object))
            drawControl(context, rect, disabledActive.get());
        else
            drawControl(context, rect, disabled.get());
    }
    return false;
}

void RenderThemeBlackBerry::setRadioSize(RenderStyle* style) const
{
    calculateButtonSize(style);
}

// If this function returns false, WebCore assumes the button is fully decorated
bool RenderThemeBlackBerry::paintButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    info.context->save();
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled, inactive, pressed;
    if (!disabled) {
        disabled = loadImage("core_button_disabled");
        inactive = loadImage("core_button_inactive");
        pressed = loadImage("core_button_pressed");
    }

    AffineTransform ctm = context->getCTM();
    if (!isEnabled(object)) {
        drawNineSlice(context, rect, ctm.xScale(), inactive.get(), largeSlice);
        drawNineSlice(context, rect, ctm.xScale(), disabled.get(), largeSlice);
    } else if (isPressed(object)) {
        drawNineSlice(context, rect, ctm.xScale(), pressed.get(), largeSlice);
    } else
        drawNineSlice(context, rect, ctm.xScale(), inactive.get(), largeSlice);

    context->restore();
    return false;
}

void RenderThemeBlackBerry::adjustMenuListStyle(StyleResolver* css, RenderStyle* style, Element* element) const
{
    adjustMenuListButtonStyle(css, style, element);
}

static IntRect computeMenuListArrowButtonRect(const IntRect& rect)
{
    // FIXME: The menu list arrow button should have a minimum and maximum width (to ensure usability) or
    // scale with respect to the font size used in the menu list control or some combination of both.
    return IntRect(IntPoint(rect.maxX() - rect.height(), rect.y()), IntSize(rect.height(), rect.height()));
}

bool RenderThemeBlackBerry::paintMenuList(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    info.context->save();
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled, inactive, pressed, arrowUp, arrowUpPressed;
    if (!disabled) {
        disabled = loadImage("core_button_disabled");
        inactive = loadImage("core_button_inactive");
        pressed = loadImage("core_button_pressed");
        arrowUp = loadImage("core_dropdown_button_arrowup");
        arrowUpPressed = loadImage("core_dropdown_button_arrowup_pressed");
    }

    FloatRect arrowButtonRectangle(computeMenuListArrowButtonRect(rect));
    float x = arrowButtonRectangle.x() + arrowButtonRectangle.width() / xPositionRatio;
    float y = arrowButtonRectangle.y() + arrowButtonRectangle.height() * yPositionRatio;
    float width = arrowButtonRectangle.width() / widthRatio;
    float height = arrowButtonRectangle.height() * heightRatio;
    FloatRect tmpRect(x, y, width, height);

    AffineTransform ctm = context->getCTM();
    if (!isEnabled(object)) {
        drawNineSlice(context, rect, ctm.xScale(), inactive.get(), largeSlice);
        drawNineSlice(context, rect, ctm.xScale(), disabled.get(), largeSlice);
        drawControl(context, tmpRect, arrowUp.get()); // FIXME: should have a disabled image.
    } else if (isPressed(object)) {
        drawNineSlice(context, rect, ctm.xScale(), pressed.get(), largeSlice);
        drawControl(context, tmpRect, arrowUpPressed.get());
    } else {
        drawNineSlice(context, rect, ctm.xScale(), inactive.get(), largeSlice);
        drawControl(context, tmpRect, arrowUp.get());
    }
    context->restore();
    return false;
}

bool RenderThemeBlackBerry::paintMenuListButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintMenuList(object, info, rect);
}

void RenderThemeBlackBerry::adjustSliderThumbSize(RenderStyle* style, Element* element) const
{
    float fullScreenMultiplier = 1;
    ControlPart part = style->appearance();

    if (part == MediaSliderThumbPart || part == MediaVolumeSliderThumbPart) {
        RenderSlider* slider = determineRenderSlider(element->renderer());
        if (slider)
            fullScreenMultiplier = determineFullScreenMultiplier(toElement(slider->node()));
    }

    if (part == MediaVolumeSliderThumbPart || part == SliderThumbHorizontalPart || part == SliderThumbVerticalPart) {
        style->setWidth(Length((part == SliderThumbVerticalPart ? sliderThumbHeight : sliderThumbWidth) * fullScreenMultiplier, Fixed));
        style->setHeight(Length((part == SliderThumbVerticalPart ? sliderThumbWidth : sliderThumbHeight) * fullScreenMultiplier, Fixed));
    } else if (part == MediaSliderThumbPart) {
        style->setWidth(Length(mediaSliderThumbWidth * fullScreenMultiplier, Fixed));
        style->setHeight(Length(mediaSliderThumbHeight * fullScreenMultiplier, Fixed));
    }
}

bool RenderThemeBlackBerry::paintSliderTrack(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    const static int SliderTrackHeight = 5;
    IntRect rect2;
    if (object->style()->appearance() == SliderHorizontalPart) {
        rect2.setHeight(SliderTrackHeight);
        rect2.setWidth(rect.width());
        rect2.setX(rect.x());
        rect2.setY(rect.y() + (rect.height() - SliderTrackHeight) / 2);
    } else {
        rect2.setHeight(rect.height());
        rect2.setWidth(SliderTrackHeight);
        rect2.setX(rect.x() + (rect.width() - SliderTrackHeight) / 2);
        rect2.setY(rect.y());
    }
    return paintSliderTrackRect(object, info, rect2);
}

bool RenderThemeBlackBerry::paintSliderTrackRect(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    static RefPtr<Image> background;
    if (!background)
        background = loadImage("core_slider_bg");
    return paintSliderTrackRect(object, info, rect, background.get());
}

bool RenderThemeBlackBerry::paintSliderTrackRect(RenderObject* object, const PaintInfo& info, const IntRect& rect, Image* inactive)
{
    ASSERT(info.context);
    info.context->save();
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled;
    if (!disabled)
        disabled = loadImage("core_slider_fill_disabled");

    if (rect.width() > rect.height()) {
        if (isEnabled(object))
            drawThreeSliceHorizontal(context, rect, inactive, mediumSlice);
        else
            drawThreeSliceHorizontal(context, rect, disabled.get(), (smallSlice - 1));
    } else {
        if (isEnabled(object))
            drawThreeSliceVertical(context, rect, inactive, mediumSlice);
        else
            drawThreeSliceVertical(context, rect, disabled.get(), (smallSlice - 1));
    }

    context->restore();
    return false;
}

bool RenderThemeBlackBerry::paintSliderThumb(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    info.context->save();
    GraphicsContext* context = info.context;

    static RefPtr<Image> disabled, inactive, pressed, aura;
    if (!disabled) {
        disabled = loadImage("core_slider_handle_disabled");
        inactive = loadImage("core_slider_handle");
        pressed = loadImage("core_slider_handle_pressed");
        aura = loadImage("core_slider_aura");
    }

    FloatRect tmpRect(rect);
    float length = std::max(tmpRect.width(), tmpRect.height());
    if (tmpRect.width() > tmpRect.height()) {
        tmpRect.setY(tmpRect.y() - (length - tmpRect.height()) / 2);
        tmpRect.setHeight(length);
    } else {
        tmpRect.setX(tmpRect.x() - (length - tmpRect.width()) / 2);
        tmpRect.setWidth(length);
    }

    float auraHeight = length * auraRatio;
    float auraWidth = auraHeight;
    float auraX = tmpRect.x() - (auraWidth - tmpRect.width()) / 2;
    float auraY = tmpRect.y() - (auraHeight - tmpRect.height()) / 2;
    FloatRect auraRect(auraX, auraY, auraWidth, auraHeight);

    if (!isEnabled(object))
        drawControl(context, tmpRect, disabled.get());
    else {
        if (isPressed(object) || isHovered(object) || isFocused(object)) {
            drawControl(context, tmpRect, pressed.get());
            drawControl(context, auraRect, aura.get());
        } else {
            drawControl(context, tmpRect, inactive.get());
        }
    }

    context->restore();
    return false;
}

void RenderThemeBlackBerry::adjustMediaControlStyle(StyleResolver*, RenderStyle* style, Element* element) const
{
    float fullScreenMultiplier = determineFullScreenMultiplier(element);
    HTMLMediaElement* mediaElement = toParentMediaElement(element);
    if (!mediaElement)
        return;

    // We use multiples of mediaControlsHeight to make all objects scale evenly
    Length zero(0, Fixed);
    Length controlsHeight(mediaControlsHeight * fullScreenMultiplier, Fixed);
    Length timeWidth(mediaControlsHeight * 3 / 2 * fullScreenMultiplier, Fixed);
    Length volumeHeight(mediaControlsHeight * 4 * fullScreenMultiplier, Fixed);
    Length padding(mediaControlsHeight / 8 * fullScreenMultiplier, Fixed);
    float fontSize = mediaControlsHeight / 2 * fullScreenMultiplier;

    switch (style->appearance()) {
    case MediaPlayButtonPart:
    case MediaEnterFullscreenButtonPart:
    case MediaExitFullscreenButtonPart:
    case MediaMuteButtonPart:
        style->setWidth(controlsHeight);
        style->setHeight(controlsHeight);
        break;
    case MediaCurrentTimePart:
    case MediaTimeRemainingPart:
        style->setWidth(timeWidth);
        style->setHeight(controlsHeight);
        style->setPaddingRight(padding);
        style->setFontSize(static_cast<int>(fontSize));
        break;
    case MediaVolumeSliderContainerPart:
        style->setWidth(controlsHeight);
        style->setHeight(volumeHeight);
        style->setBottom(controlsHeight);
        break;
    default:
        break;
    }

    if (!isfinite(mediaElement->duration())) {
        // Live streams have infinite duration with no timeline. Force the mute
        // and fullscreen buttons to the right. This is needed when webkit does
        // not render the timeline container because it has a webkit-box-flex
        // of 1 and normally allows those buttons to be on the right.
        switch (style->appearance()) {
        case MediaEnterFullscreenButtonPart:
        case MediaExitFullscreenButtonPart:
            style->setPosition(AbsolutePosition);
            style->setBottom(zero);
            style->setRight(controlsHeight);
            break;
        case MediaMuteButtonPart:
            style->setPosition(AbsolutePosition);
            style->setBottom(zero);
            style->setRight(zero);
            break;
        default:
            break;
        }
    }
}

void RenderThemeBlackBerry::adjustSliderTrackStyle(StyleResolver*, RenderStyle* style, Element* element) const
{
    float fullScreenMultiplier = determineFullScreenMultiplier(element);

    // We use multiples of mediaControlsHeight to make all objects scale evenly
    Length controlsHeight(mediaControlsHeight * fullScreenMultiplier, Fixed);
    Length volumeHeight(mediaControlsHeight * 4 * fullScreenMultiplier, Fixed);
    switch (style->appearance()) {
    case MediaSliderPart:
        style->setHeight(controlsHeight);
        break;
    case MediaVolumeSliderPart:
        style->setWidth(controlsHeight);
        style->setHeight(volumeHeight);
        break;
    case MediaFullScreenVolumeSliderPart:
    default:
        break;
    }
}

static bool paintMediaButton(GraphicsContext* context, const IntRect& rect, Image* image)
{
    context->drawImage(image, ColorSpaceDeviceRGB, rect);
    return false;
}

bool RenderThemeBlackBerry::paintMediaPlayButton(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);

    if (!mediaElement)
        return false;

    static Image* mediaPlay = Image::loadPlatformResource("play").leakRef();
    static Image* mediaPause = Image::loadPlatformResource("pause").leakRef();

    return paintMediaButton(paintInfo.context, rect, mediaElement->canPlay() ? mediaPlay : mediaPause);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaMuteButton(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);

    if (!mediaElement)
        return false;

    static Image* mediaMute = Image::loadPlatformResource("speaker").leakRef();
    static Image* mediaUnmute = Image::loadPlatformResource("speaker_mute").leakRef();

    return paintMediaButton(paintInfo.context, rect, mediaElement->muted() || !mediaElement->volume() ? mediaUnmute : mediaMute);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaFullscreenButton(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (!mediaElement)
        return false;

    static Image* mediaEnterFullscreen = Image::loadPlatformResource("fullscreen").leakRef();
    static Image* mediaExitFullscreen = Image::loadPlatformResource("exit_fullscreen").leakRef();

    Image* buttonImage = mediaEnterFullscreen;
#if ENABLE(FULLSCREEN_API)
    if (mediaElement->document()->webkitIsFullScreen() && mediaElement->document()->webkitCurrentFullScreenElement() == mediaElement)
        buttonImage = mediaExitFullscreen;
#endif
    return paintMediaButton(paintInfo.context, rect, buttonImage);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaSliderTrack(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (!mediaElement)
        return false;

    float fullScreenMultiplier = determineFullScreenMultiplier(mediaElement);
    float loaded = mediaElement->percentLoaded();
    float position = mediaElement->duration() > 0 ? (mediaElement->currentTime() / mediaElement->duration()) : 0;

    int x = ceil(rect.x() + 2 * fullScreenMultiplier - fullScreenMultiplier / 2);
    int y = ceil(rect.y() + 14 * fullScreenMultiplier + fullScreenMultiplier / 2);
    int w = ceil(rect.width() - 4 * fullScreenMultiplier + fullScreenMultiplier / 2);
    int h = ceil(2 * fullScreenMultiplier);
    IntRect rect2(x, y, w, h);

    int wPlayed = ceil(w * position);
    int wLoaded = ceil((w - mediaSliderThumbWidth * fullScreenMultiplier) * loaded + mediaSliderThumbWidth * fullScreenMultiplier);

    IntRect played(x, y, wPlayed, h);
    IntRect buffered(x, y, wLoaded, h);

    // This is to paint main slider bar.
    static RefPtr<Image> trackImage;
    if (!trackImage)
        trackImage = loadImage("core_slider_media_bg");
    bool result = paintSliderTrackRect(object, paintInfo, rect2, trackImage.get());

    if (loaded > 0 || position > 0) {
        // This is to paint buffered bar.
        static RefPtr<Image> bufferedImage, playedImage;
        if (!bufferedImage) {
            bufferedImage = loadImage("core_slider_buffered_bg");
            playedImage = loadImage("core_slider_played_bg");
        }
        paintSliderTrackRect(object, paintInfo, buffered, bufferedImage.get());

        // This is to paint played part of bar (left of slider thumb) using selection color.
        paintSliderTrackRect(object, paintInfo, played, playedImage.get());
    }
    return result;

#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaSliderThumb(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    RenderSlider* slider = determineRenderSlider(object);
    if (!slider)
        return false;

    static Image* mediaVolumeThumb = Image::loadPlatformResource("volume_thumb").leakRef();

    drawControl(paintInfo.context, rect, mediaVolumeThumb);

    return true;
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaVolumeSliderTrack(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    float pad = rect.width() * 0.45;
    float x = rect.x() + pad;
    float y = rect.y() + pad;
    float width = rect.width() * 0.1;
    float height = rect.height() - (2.0 * pad);

    IntRect rect2(x, y, width, height);

    static RefPtr<Image> trackImage;
    if (!trackImage)
        trackImage = loadImage("core_slider_media_bg");

    return paintSliderTrackRect(object, paintInfo, rect2, trackImage.get());
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBlackBerry::paintMediaVolumeSliderThumb(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    static Image* mediaVolumeThumb = Image::loadPlatformResource("volume_thumb").leakRef();

    return paintMediaButton(paintInfo.context, rect, mediaVolumeThumb);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

Color RenderThemeBlackBerry::platformFocusRingColor() const
{
    return focusRingPen;
}

#if ENABLE(TOUCH_EVENTS)
Color RenderThemeBlackBerry::platformTapHighlightColor() const
{
    return Color(0, 168, 223, 50);
}
#endif

Color RenderThemeBlackBerry::platformActiveSelectionBackgroundColor() const
{
    return Color(0, 168, 223, 50);
}

double RenderThemeBlackBerry::animationRepeatIntervalForProgressBar(RenderProgress* renderProgress) const
{
    return renderProgress->isDeterminate() ? 0.0 : 0.1;
}

double RenderThemeBlackBerry::animationDurationForProgressBar(RenderProgress* renderProgress) const
{
    return renderProgress->isDeterminate() ? 0.0 : 2.0;
}

bool RenderThemeBlackBerry::paintProgressBar(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    if (!object->isProgress())
        return true;

    RenderProgress* renderProgress = toRenderProgress(object);

    FloatSize smallCorner(smallRadius, smallRadius);

    info.context->save();
    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeThickness(lineWidth);

    info.context->setStrokeGradient(createLinearGradient(rangeSliderRegularTopOutline, rangeSliderRegularBottomOutline, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
    info.context->setFillGradient(createLinearGradient(rangeSliderRegularTop, rangeSliderRegularBottom, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));

    Path path;
    path.addRoundedRect(rect, smallCorner);
    info.context->fillPath(path);

    IntRect rect2 = rect;
    rect2.setX(rect2.x() + 1);
    rect2.setHeight(rect2.height() - 2);
    rect2.setY(rect2.y() + 1);
    info.context->setStrokeStyle(NoStroke);
    info.context->setStrokeThickness(0);
    if (renderProgress->isDeterminate()) {
        rect2.setWidth(rect2.width() * renderProgress->position() - 2);
        info.context->setFillGradient(createLinearGradient(progressRegularTop, progressRegularBottom, rect2.maxXMinYCorner(), rect2.maxXMaxYCorner()));
    } else {
        // Animating
        rect2.setWidth(rect2.width() - 2);
        RefPtr<Gradient> gradient = Gradient::create(rect2.minXMaxYCorner(), rect2.maxXMaxYCorner());
        gradient->addColorStop(0.0, Color(progressRegularBottom));
        gradient->addColorStop(renderProgress->animationProgress(), Color(progressRegularTop));
        gradient->addColorStop(1.0, Color(progressRegularBottom));
        info.context->setFillGradient(gradient);
    }
    Path path2;
    path2.addRoundedRect(rect2, smallCorner);
    info.context->fillPath(path2);

    info.context->restore();
    return false;
}

Color RenderThemeBlackBerry::platformActiveTextSearchHighlightColor() const
{
    return Color(255, 150, 50); // Orange.
}

Color RenderThemeBlackBerry::platformInactiveTextSearchHighlightColor() const
{
    return Color(255, 255, 0); // Yellow.
}

} // namespace WebCore
