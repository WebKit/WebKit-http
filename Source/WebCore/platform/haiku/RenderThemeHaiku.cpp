/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 *               2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "RenderThemeHaiku.h"

#include "GraphicsContext.h"
#include "NotImplemented.h"
#include "PaintInfo.h"
#include "UserAgentScripts.h"
#include "UserAgentStyleSheets.h"
#include <ControlLook.h>
#include <View.h>


namespace WebCore {

PassRefPtr<RenderTheme> RenderThemeHaiku::create()
{
    return adoptRef(new RenderThemeHaiku());
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page*)
{
    static RenderTheme* renderTheme = RenderThemeHaiku::create().leakRef();
    return renderTheme;
}

RenderThemeHaiku::RenderThemeHaiku()
{
}

RenderThemeHaiku::~RenderThemeHaiku()
{
}

bool RenderThemeHaiku::supportsFocusRing(const RenderStyle* style) const
{
    switch (style->appearance()) {
    case PushButtonPart:
    case ButtonPart:
    case TextFieldPart:
    case TextAreaPart:
    case SearchFieldPart:
    case MenulistPart:
    case RadioPart:
    case CheckboxPart:
        return true;
    default:
        return false;
    }
}

Color RenderThemeHaiku::platformActiveSelectionBackgroundColor() const
{
    return Color(255, 80, 40, 200);
}

Color RenderThemeHaiku::platformInactiveSelectionBackgroundColor() const
{
    return Color(255, 80, 40, 200);
}

Color RenderThemeHaiku::platformActiveSelectionForegroundColor() const
{
    return Color(0, 0, 0, 255);
}

Color RenderThemeHaiku::platformInactiveSelectionForegroundColor() const
{
    return Color(0, 0, 0, 255);
}

void RenderThemeHaiku::systemFont(CSSValueID propId, FontDescription&) const
{
    notImplemented();
}

String RenderThemeHaiku::mediaControlsStyleSheet()
{
    return ASCIILiteral(mediaControlsAppleUserAgentStyleSheet);
}

String RenderThemeHaiku::mediaControlsScript()
{
    return ASCIILiteral(mediaControlsAppleJavaScript);
}

bool RenderThemeHaiku::paintCheckbox(RenderObject* object, const PaintInfo& info, const IntRect& intRect)
{
    if (info.context->paintingDisabled())
        return true;

    if (!be_control_look)
        return true;

    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    BRect rect = intRect;
    BView* view = info.context->platformContext();
    unsigned flags = flagsForObject(object);

	view->PushState();
    be_control_look->DrawCheckBox(view, rect, rect, base, flags);
	view->PopState();
    return false;
}

void RenderThemeHaiku::setCheckboxSize(RenderStyle* style) const
{
    int size = 14;

    // If the width and height are both specified, then we have nothing to do.
    if (!style->width().isIntrinsicOrAuto() && !style->height().isAuto())
        return;

    // FIXME: A hard-coded size of 'size' is used. This is wrong but necessary for now.
    if (style->width().isIntrinsicOrAuto())
        style->setWidth(Length(size, Fixed));

    if (style->height().isAuto())
        style->setHeight(Length(size, Fixed));
}

bool RenderThemeHaiku::paintRadio(RenderObject* object, const PaintInfo& info,
	const IntRect& intRect)
{
    if (info.context->paintingDisabled())
        return true;

    if (!be_control_look)
        return true;

    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    BRect rect = intRect;
    BView* view = info.context->platformContext();
    unsigned flags = flagsForObject(object);

	view->PushState();
    be_control_look->DrawRadioButton(view, rect, rect, base, flags);
	view->PopState();
    return false;
}

void RenderThemeHaiku::setRadioSize(RenderStyle* style) const
{
    // This is the same as checkboxes.
    setCheckboxSize(style);
}

bool RenderThemeHaiku::paintButton(RenderObject* object, const PaintInfo& info, const IntRect& intRect)
{
    if (info.context->paintingDisabled())
        return true;

    if (!be_control_look)
        return true;

    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    rgb_color background = base;
    	// TODO: From PaintInfo?
    BRect rect = intRect;
    BView* view = info.context->platformContext();
    unsigned flags = flagsForObject(object);
    if (isPressed(object))
    	flags |= BControlLook::B_ACTIVATED;
    if (isDefault(object))
    	flags |= BControlLook::B_DEFAULT_BUTTON;

	view->PushState();
    be_control_look->DrawButtonFrame(view, rect, rect, base, background, flags);
    be_control_look->DrawButtonBackground(view, rect, rect, base, flags);
    view->PopState();
    return false;
}

void RenderThemeHaiku::adjustTextFieldStyle(StyleResolver* selector, RenderStyle* style, Element* element) const
{
    style->setBackgroundColor(Color::transparent);
}

bool RenderThemeHaiku::paintTextField(RenderObject* object, const PaintInfo& info, const IntRect& intRect)
{
    if (info.context->paintingDisabled())
        return true;

    if (!be_control_look)
        return true;

    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    //rgb_color background = base;
    	// TODO: From PaintInfo?
    BRect rect = intRect;
    BView* view = info.context->platformContext();
    unsigned flags = flagsForObject(object) & ~BControlLook::B_CLICKED;

	view->PushState();
    be_control_look->DrawTextControlBorder(view, rect, rect, base, flags);
    // Fill the background as well (we set it to transparent in
    // adjustTextFieldStyle), otherwise it would draw over the border.
    view->SetHighColor(255, 255, 255);
    view->FillRect(rect);
    view->PopState();
    return false;
}

void RenderThemeHaiku::adjustTextAreaStyle(StyleResolver* selector, RenderStyle* style, Element* element) const
{
	adjustTextFieldStyle(selector, style, element);
}

bool RenderThemeHaiku::paintTextArea(RenderObject* object, const PaintInfo& info, const IntRect& intRect)
{
    return paintTextField(object, info, intRect);
}

void RenderThemeHaiku::adjustMenuListStyle(StyleResolver* selector, RenderStyle* style, Element* element) const
{
    adjustMenuListButtonStyle(selector, style, element);
}

void RenderThemeHaiku::adjustMenuListButtonStyle(StyleResolver* selector, RenderStyle* style, Element* element) const
{
    style->resetBorder();
    style->resetBorderRadius();

	int labelSpacing = be_control_look ? static_cast<int>(be_control_look->DefaultLabelSpacing()) : 3;
    // Position the text correctly within the select box and make the box wide enough to fit the dropdown button
    style->setPaddingTop(Length(3, Fixed));
    style->setPaddingLeft(Length(3 + labelSpacing, Fixed));
    style->setPaddingRight(Length(22, Fixed));
    style->setPaddingBottom(Length(3, Fixed));

    // Height is locked to auto
    style->setHeight(Length(Auto));

    // Calculate our min-height
    const int menuListButtonMinHeight = 20;
    int minHeight = style->fontSize();
    minHeight = std::max(minHeight, menuListButtonMinHeight);

    style->setMinHeight(Length(minHeight, Fixed));
}

bool RenderThemeHaiku::paintMenuList(RenderObject* object, const PaintInfo& info, const IntRect& intRect)
{
    if (info.context->paintingDisabled())
        return true;

    if (!be_control_look)
        return true;

    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    //rgb_color background = base;
    	// TODO: From PaintInfo?
    BRect rect = intRect;
    BView* view = info.context->platformContext();
    unsigned flags = flagsForObject(object) & ~BControlLook::B_CLICKED;

	view->PushState();
    be_control_look->DrawMenuFieldFrame(view, rect, rect, base, base, flags);
    be_control_look->DrawMenuFieldBackground(view, rect, rect, base, true, flags);
    view->PopState();
    return false;
}

unsigned RenderThemeHaiku::flagsForObject(RenderObject* object) const
{
    unsigned flags = BControlLook::B_BLEND_FRAME;
    if (!isEnabled(object))
    	flags |= BControlLook::B_DISABLED;
    if (isFocused(object))
    	flags |= BControlLook::B_FOCUSED;
    if (isPressed(object))
    	flags |= BControlLook::B_CLICKED;
    if (isChecked(object))
    	flags |= BControlLook::B_ACTIVATED;
	return flags;
}

} // namespace WebCore
