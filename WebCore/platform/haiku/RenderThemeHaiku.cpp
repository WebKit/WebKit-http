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
#include <ControlLook.h>
#include <View.h>


namespace WebCore {

PassRefPtr<RenderTheme> RenderThemeHaiku::create()
{
    return adoptRef(new RenderThemeHaiku());
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page*)
{
    static RenderTheme* renderTheme = RenderThemeHaiku::create().releaseRef();
    return renderTheme;
}

RenderThemeHaiku::RenderThemeHaiku()
{
}

RenderThemeHaiku::~RenderThemeHaiku()
{
}

static bool supportsFocus(ControlPart appearance)
{
    switch (appearance) {
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

bool RenderThemeHaiku::supportsFocusRing(const RenderStyle* style) const
{
    return supportsFocus(style->appearance());
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

Color RenderThemeHaiku::platformTextSearchHighlightColor() const
{
    return Color(255, 80, 40, 200);
}

void RenderThemeHaiku::systemFont(int propId, FontDescription&) const
{
    notImplemented();
}

bool RenderThemeHaiku::paintCheckbox(RenderObject* object, const RenderObject::PaintInfo& info, const IntRect& intRect)
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

bool RenderThemeHaiku::paintRadio(RenderObject* object, const RenderObject::PaintInfo& info,
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

void RenderThemeHaiku::adjustButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* element) const
{
	// TODO: If this is the default button, extend the size.
	RenderTheme::adjustButtonStyle(selector, style, element);
}

bool RenderThemeHaiku::paintButton(RenderObject* object, const RenderObject::PaintInfo& info, const IntRect& intRect)
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

void RenderThemeHaiku::setButtonSize(RenderStyle* style) const
{
	RenderTheme::setButtonSize(style);
}

void RenderThemeHaiku::adjustTextFieldStyle(CSSStyleSelector* selector, RenderStyle* style, Element* element) const
{
#if 0
	RenderTheme::adjustTextFieldStyle(selector, style, element);
#else
    style->setBackgroundColor(Color::transparent);
#endif
}

bool RenderThemeHaiku::paintTextField(RenderObject* object, const RenderObject::PaintInfo& info, const IntRect& intRect)
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

void RenderThemeHaiku::adjustTextAreaStyle(CSSStyleSelector* selector, RenderStyle* style, Element* element) const
{
#if 0
	RenderTheme::adjustTextAreaStyle(selector, style, element);
#else
	adjustTextFieldStyle(selector, style, element);
#endif
}

bool RenderThemeHaiku::paintTextArea(RenderObject* object, const RenderObject::PaintInfo& info, const IntRect& intRect)
{
    return paintTextField(object, info, intRect);
}

void RenderThemeHaiku::adjustMenuListStyle(CSSStyleSelector* selector, RenderStyle* style, Element* element) const
{
    adjustMenuListButtonStyle(selector, style, element);
}

bool RenderThemeHaiku::paintMenuList(RenderObject* object, const RenderObject::PaintInfo& info, const IntRect& intRect)
{
    return paintMenuListButton(object, info, intRect);
}

void RenderThemeHaiku::adjustMenuListButtonStyle(CSSStyleSelector* selector, RenderStyle* style, Element* element) const
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
    int minHeight = style->font().height();
    minHeight = max(minHeight, menuListButtonMinHeight);

    style->setMinHeight(Length(minHeight, Fixed));
}

bool RenderThemeHaiku::paintMenuListButton(RenderObject* object, const RenderObject::PaintInfo& info, const IntRect& intRect)
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
