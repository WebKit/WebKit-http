/*
 * Copyright (C) 2007 Apple Inc.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2008, 2009 Google Inc.
 * Copyright (C) 2009 Kenneth Rohde Christiansen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "RenderThemeChromiumLinux.h"

#include "CSSValueKeywords.h"
#include "Color.h"
#include "PaintInfo.h"
#include "PlatformSupport.h"
#include "RenderObject.h"
#include "RenderProgress.h"
#include "RenderSlider.h"
#include "ScrollbarTheme.h"
#include "UserAgentStyleSheets.h"

namespace WebCore {

unsigned RenderThemeChromiumLinux::m_activeSelectionBackgroundColor =
    0xff1e90ff;
unsigned RenderThemeChromiumLinux::m_activeSelectionForegroundColor =
    Color::black;
unsigned RenderThemeChromiumLinux::m_inactiveSelectionBackgroundColor =
    0xffc8c8c8;
unsigned RenderThemeChromiumLinux::m_inactiveSelectionForegroundColor =
    0xff323232;

double RenderThemeChromiumLinux::m_caretBlinkInterval;

static const unsigned defaultButtonBackgroundColor = 0xffdddddd;

static PlatformSupport::ThemePaintState getWebThemeState(const RenderTheme* theme, const RenderObject* o)
{
    if (!theme->isEnabled(o))
        return PlatformSupport::StateDisabled;
    if (theme->isPressed(o))
        return PlatformSupport::StatePressed;
    if (theme->isHovered(o))
        return PlatformSupport::StateHover;

    return PlatformSupport::StateNormal;
}


PassRefPtr<RenderTheme> RenderThemeChromiumLinux::create()
{
    return adoptRef(new RenderThemeChromiumLinux());
}

// RenderTheme::themeForPage for Android is defined in RenderThemeChromiumAndroid.cpp.
#if !OS(ANDROID)
PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page* page)
{
    static RenderTheme* rt = RenderThemeChromiumLinux::create().leakRef();
    return rt;
}
#endif

RenderThemeChromiumLinux::RenderThemeChromiumLinux()
{
    m_caretBlinkInterval = RenderTheme::caretBlinkInterval();
}

RenderThemeChromiumLinux::~RenderThemeChromiumLinux()
{
}

Color RenderThemeChromiumLinux::systemColor(int cssValueId) const
{
    static const Color linuxButtonGrayColor(0xffdddddd);

    if (cssValueId == CSSValueButtonface)
        return linuxButtonGrayColor;
    return RenderTheme::systemColor(cssValueId);
}

String RenderThemeChromiumLinux::extraDefaultStyleSheet()
{
    return RenderThemeChromiumSkia::extraDefaultStyleSheet() +
           String(themeChromiumLinuxUserAgentStyleSheet, sizeof(themeChromiumLinuxUserAgentStyleSheet));
}

bool RenderThemeChromiumLinux::controlSupportsTints(const RenderObject* o) const
{
    return isEnabled(o);
}

Color RenderThemeChromiumLinux::activeListBoxSelectionBackgroundColor() const
{
    return Color(0x28, 0x28, 0x28);
}

Color RenderThemeChromiumLinux::activeListBoxSelectionForegroundColor() const
{
    return Color::black;
}

Color RenderThemeChromiumLinux::inactiveListBoxSelectionBackgroundColor() const
{
    return Color(0xc8, 0xc8, 0xc8);
}

Color RenderThemeChromiumLinux::inactiveListBoxSelectionForegroundColor() const
{
    return Color(0x32, 0x32, 0x32);
}

Color RenderThemeChromiumLinux::platformActiveSelectionBackgroundColor() const
{
    return m_activeSelectionBackgroundColor;
}

Color RenderThemeChromiumLinux::platformInactiveSelectionBackgroundColor() const
{
    return m_inactiveSelectionBackgroundColor;
}

Color RenderThemeChromiumLinux::platformActiveSelectionForegroundColor() const
{
    return m_activeSelectionForegroundColor;
}

Color RenderThemeChromiumLinux::platformInactiveSelectionForegroundColor() const
{
    return m_inactiveSelectionForegroundColor;
}

#if ENABLE(DATALIST_ELEMENT)
IntSize RenderThemeChromiumLinux::sliderTickSize() const
{
    return IntSize(1, 6);
}

int RenderThemeChromiumLinux::sliderTickOffsetFromTrackCenter() const
{
    return -16;
}
#endif

void RenderThemeChromiumLinux::adjustSliderThumbSize(RenderStyle* style, Element* element) const
{
    IntSize size = PlatformSupport::getThemePartSize(PlatformSupport::PartSliderThumb);

    if (style->appearance() == SliderThumbHorizontalPart) {
        style->setWidth(Length(size.width(), Fixed));
        style->setHeight(Length(size.height(), Fixed));
    } else if (style->appearance() == SliderThumbVerticalPart) {
        style->setWidth(Length(size.height(), Fixed));
        style->setHeight(Length(size.width(), Fixed));
    } else
        RenderThemeChromiumSkia::adjustSliderThumbSize(style, element);
}

bool RenderThemeChromiumLinux::supportsControlTints() const
{
    return true;
}

void RenderThemeChromiumLinux::setCaretBlinkInterval(double interval)
{
    m_caretBlinkInterval = interval;
}

double RenderThemeChromiumLinux::caretBlinkIntervalInternal() const
{
    return m_caretBlinkInterval;
}

void RenderThemeChromiumLinux::setSelectionColors(
    unsigned activeBackgroundColor,
    unsigned activeForegroundColor,
    unsigned inactiveBackgroundColor,
    unsigned inactiveForegroundColor)
{
    m_activeSelectionBackgroundColor = activeBackgroundColor;
    m_activeSelectionForegroundColor = activeForegroundColor;
    m_inactiveSelectionBackgroundColor = inactiveBackgroundColor;
    m_inactiveSelectionForegroundColor = inactiveForegroundColor;
}

bool RenderThemeChromiumLinux::paintCheckbox(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    PlatformSupport::ThemePaintExtraParams extraParams;
    extraParams.button.checked = isChecked(o);
    extraParams.button.indeterminate = isIndeterminate(o);

    PlatformSupport::paintThemePart(i.context, PlatformSupport::PartCheckbox, getWebThemeState(this, o), rect, &extraParams);
    return false;
}

void RenderThemeChromiumLinux::setCheckboxSize(RenderStyle* style) const
{
    // If the width and height are both specified, then we have nothing to do.
    if (!style->width().isIntrinsicOrAuto() && !style->height().isAuto())
        return;

    IntSize size = PlatformSupport::getThemePartSize(PlatformSupport::PartCheckbox);
    setSizeIfAuto(style, size);
}

bool RenderThemeChromiumLinux::paintRadio(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    PlatformSupport::ThemePaintExtraParams extraParams;
    extraParams.button.checked = isChecked(o);

    PlatformSupport::paintThemePart(i.context, PlatformSupport::PartRadio, getWebThemeState(this, o), rect, &extraParams);
    return false;
}

void RenderThemeChromiumLinux::setRadioSize(RenderStyle* style) const
{
    // If the width and height are both specified, then we have nothing to do.
    if (!style->width().isIntrinsicOrAuto() && !style->height().isAuto())
        return;

    IntSize size = PlatformSupport::getThemePartSize(PlatformSupport::PartRadio);
    setSizeIfAuto(style, size);
}

bool RenderThemeChromiumLinux::paintButton(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    PlatformSupport::ThemePaintExtraParams extraParams;
    extraParams.button.isDefault = isDefault(o);
    extraParams.button.hasBorder = true;
    extraParams.button.backgroundColor = defaultButtonBackgroundColor;
    if (o->hasBackground())
        extraParams.button.backgroundColor = o->style()->visitedDependentColor(CSSPropertyBackgroundColor).rgb();

    PlatformSupport::paintThemePart(i.context, PlatformSupport::PartButton, getWebThemeState(this, o), rect, &extraParams);
    return false;
}

bool RenderThemeChromiumLinux::paintTextField(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    // WebThemeEngine does not handle border rounded corner and background image
    // so return true to draw CSS border and background.
    if (o->style()->hasBorderRadius() || o->style()->hasBackgroundImage())
        return true;

    ControlPart part = o->style()->appearance();

    PlatformSupport::ThemePaintExtraParams extraParams;
    extraParams.textField.isTextArea = part == TextAreaPart;
    extraParams.textField.isListbox = part == ListboxPart;

    // Fallback to white if the specified color object is invalid.
    Color backgroundColor(Color::white);
    if (o->style()->visitedDependentColor(CSSPropertyBackgroundColor).isValid())
        backgroundColor = o->style()->visitedDependentColor(CSSPropertyBackgroundColor);
    extraParams.textField.backgroundColor = backgroundColor.rgb();

    PlatformSupport::paintThemePart(i.context, PlatformSupport::PartTextField, getWebThemeState(this, o), rect, &extraParams);
    return false;
}

bool RenderThemeChromiumLinux::paintMenuList(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    if (!o->isBox())
        return false;

    const int right = rect.x() + rect.width();
    const int middle = rect.y() + rect.height() / 2;

    PlatformSupport::ThemePaintExtraParams extraParams;
    extraParams.menuList.arrowX = (o->style()->direction() == RTL) ? rect.x() + 7 : right - 13;
    extraParams.menuList.arrowY = middle;
    const RenderBox* box = toRenderBox(o);
    // Match Chromium Win behaviour of showing all borders if any are shown.
    extraParams.menuList.hasBorder = box->borderRight() || box->borderLeft() || box->borderTop() || box->borderBottom();
    extraParams.menuList.hasBorderRadius = o->style()->hasBorderRadius();
    // Fallback to transparent if the specified color object is invalid.
    extraParams.menuList.backgroundColor = Color::transparent;
    if (o->hasBackground())
        extraParams.menuList.backgroundColor = o->style()->visitedDependentColor(CSSPropertyBackgroundColor).rgb();

    PlatformSupport::paintThemePart(i.context, PlatformSupport::PartMenuList, getWebThemeState(this, o), rect, &extraParams);
    return false;
}

bool RenderThemeChromiumLinux::paintSliderTrack(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    PlatformSupport::ThemePaintExtraParams extraParams;
    extraParams.slider.vertical = o->style()->appearance() == SliderVerticalPart;

    PlatformSupport::paintThemePart(i.context, PlatformSupport::PartSliderTrack, getWebThemeState(this, o), rect, &extraParams);

#if ENABLE(DATALIST_ELEMENT)
    paintSliderTicks(o, i, rect);
#endif

    return false;
}

bool RenderThemeChromiumLinux::paintSliderThumb(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    PlatformSupport::ThemePaintExtraParams extraParams;
    extraParams.slider.vertical = o->style()->appearance() == SliderThumbVerticalPart;
    extraParams.slider.inDrag = isPressed(o);

    PlatformSupport::paintThemePart(i.context, PlatformSupport::PartSliderThumb, getWebThemeState(this, o), rect, &extraParams);
    return false;
}

void RenderThemeChromiumLinux::adjustInnerSpinButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    IntSize size = PlatformSupport::getThemePartSize(PlatformSupport::PartInnerSpinButton);

    style->setWidth(Length(size.width(), Fixed));
    style->setMinWidth(Length(size.width(), Fixed));
}

bool RenderThemeChromiumLinux::paintInnerSpinButton(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    PlatformSupport::ThemePaintExtraParams extraParams;
    extraParams.innerSpin.spinUp = (controlStatesForRenderer(o) & SpinUpState);
    extraParams.innerSpin.readOnly = isReadOnlyControl(o);

    PlatformSupport::paintThemePart(i.context, PlatformSupport::PartInnerSpinButton, getWebThemeState(this, o), rect, &extraParams);
    return false;
}

#if ENABLE(PROGRESS_ELEMENT)

bool RenderThemeChromiumLinux::paintProgressBar(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    if (!o->isProgress())
        return true;

    RenderProgress* renderProgress = toRenderProgress(o);
    IntRect valueRect = progressValueRectFor(renderProgress, rect);

    PlatformSupport::ThemePaintExtraParams extraParams;
    extraParams.progressBar.determinate = renderProgress->isDeterminate();
    extraParams.progressBar.valueRectX = valueRect.x();
    extraParams.progressBar.valueRectY = valueRect.y();
    extraParams.progressBar.valueRectWidth = valueRect.width();
    extraParams.progressBar.valueRectHeight = valueRect.height();

    DirectionFlippingScope scope(o, i, rect);
    PlatformSupport::paintThemePart(i.context, PlatformSupport::PartProgressBar, getWebThemeState(this, o), rect, &extraParams);
    return false;
}

#endif

} // namespace WebCore
