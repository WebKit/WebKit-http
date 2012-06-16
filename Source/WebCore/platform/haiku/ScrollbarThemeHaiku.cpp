/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright 2009 Maxime Simon <simon.maxime@gmail.com> All Rights Reserved.
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de> All Rights Reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ScrollbarThemeHaiku.h"

#include "GraphicsContext.h"
#include "Scrollbar.h"
#include <ControlLook.h>
#include <InterfaceDefs.h>
#include <Shape.h>


static int buttonWidth(int scrollbarWidth, int thickness)
{
    return scrollbarWidth < 2 * thickness ? scrollbarWidth / 2 : thickness;
}

namespace WebCore {

ScrollbarTheme* ScrollbarTheme::nativeTheme()
{
	// FIXME: If the ScrollView is embedded in the main frame, we don't want to
	// draw the outer frame, since that is already drawn be the suroundings.
	// So it would be cool if we would instantiate one theme per ScrollView. On
	// the other hand, it looks better with most web sites anyway, since they also
	// draw an outer frame around a scroll area.
    static ScrollbarThemeHaiku theme(false);
    return &theme;
}

ScrollbarThemeHaiku::ScrollbarThemeHaiku(bool drawOuterFrame)
    : m_drawOuterFrame(drawOuterFrame)
{
}

ScrollbarThemeHaiku::~ScrollbarThemeHaiku()
{
}

int ScrollbarThemeHaiku::scrollbarThickness(ScrollbarControlSize controlSize)
{
    // FIXME: Should we make a distinction between a Small and a Regular Scrollbar?
    if (m_drawOuterFrame)
    	return 15;
    return 14;
}

bool ScrollbarThemeHaiku::hasButtons(ScrollbarThemeClient* scrollbar)
{
    return scrollbar->enabled();
}

bool ScrollbarThemeHaiku::hasThumb(ScrollbarThemeClient* scrollbar)
{
    return scrollbar->enabled() && thumbLength(scrollbar) > 0;
}

IntRect ScrollbarThemeHaiku::backButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    if (part == BackButtonEndPart)
        return IntRect();

    int thickness = scrollbarThickness();
    IntPoint buttonOrigin(scrollbar->x(), scrollbar->y());
    IntSize buttonSize = scrollbar->orientation() == HorizontalScrollbar
        ? IntSize(buttonWidth(scrollbar->width(), thickness), thickness)
        : IntSize(thickness, buttonWidth(scrollbar->height(), thickness));
    IntRect buttonRect(buttonOrigin, buttonSize);

    return buttonRect;
}

IntRect ScrollbarThemeHaiku::forwardButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    if (part == ForwardButtonStartPart)
        return IntRect();

    int thickness = scrollbarThickness();
    if (scrollbar->orientation() == HorizontalScrollbar) {
        int width = buttonWidth(scrollbar->width(), thickness);
        return IntRect(scrollbar->x() + scrollbar->width() - width, scrollbar->y(), width, thickness);
    }

    int height = buttonWidth(scrollbar->height(), thickness);
    return IntRect(scrollbar->x(), scrollbar->y() + scrollbar->height() - height, thickness, height);
}

IntRect ScrollbarThemeHaiku::trackRect(ScrollbarThemeClient* scrollbar, bool)
{
    int thickness = scrollbarThickness();
    if (scrollbar->orientation() == HorizontalScrollbar) {
        if (scrollbar->width() < 2 * thickness)
            return IntRect();
        return IntRect(scrollbar->x() + thickness, scrollbar->y(), scrollbar->width() - 2 * thickness, thickness);
    }
    if (scrollbar->height() < 2 * thickness)
        return IntRect();
    return IntRect(scrollbar->x(), scrollbar->y() + thickness, thickness, scrollbar->height() - 2 * thickness);
}

void ScrollbarThemeHaiku::paintScrollbarBackground(GraphicsContext* context, ScrollbarThemeClient* scrollbar)
{
    if (!be_control_look)
        return;

    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    BRect rect = trackRect(scrollbar, false);
    BView* view = context->platformContext();
    view->SetHighColor(tint_color(base, B_DARKEN_2_TINT));

    enum orientation orientation;
    if (scrollbar->orientation() == HorizontalScrollbar) {
        orientation = B_HORIZONTAL;
        view->StrokeLine(rect.LeftTop(), rect.RightTop());
        if (m_drawOuterFrame)
            view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
        else
            rect.bottom++;
        rect.InsetBy(-1, 1);
    } else {
        orientation = B_VERTICAL;
        view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
        if (m_drawOuterFrame)
            view->StrokeLine(rect.RightTop(), rect.RightBottom());
        else
            rect.right++;
        rect.InsetBy(1, -1);
    }

    be_control_look->DrawScrollBarBackground(view, rect, rect, base, 0, orientation);
}

void ScrollbarThemeHaiku::paintButton(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& intRect, ScrollbarPart part)
{
    if (!be_control_look)
        return;

    BRect rect = BRect(intRect);
    BView* view = context->platformContext();
	bool down = scrollbar->pressedPart() == part;

    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    rgb_color dark1 = tint_color(base, B_DARKEN_1_TINT);
    rgb_color dark2 = tint_color(base, B_DARKEN_2_TINT);
    rgb_color dark3 = tint_color(base, B_DARKEN_3_TINT);
    rgb_color darkMax;
    if (down)
        darkMax = tint_color(base, B_DARKEN_MAX_TINT);
    else
        darkMax = tint_color(base, (B_DARKEN_MAX_TINT + B_DARKEN_4_TINT) / 2);

    enum orientation orientation;
    int arrowDirection;
    if (scrollbar->orientation() == VerticalScrollbar) {
        orientation = B_VERTICAL;
        arrowDirection = part == BackButtonStartPart ? BControlLook::B_UP_ARROW : BControlLook::B_DOWN_ARROW;
        view->SetHighColor(dark2);
        view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
        if (m_drawOuterFrame)
            view->StrokeLine(rect.RightTop(), rect.RightBottom());
        else
            rect.right++;
        rect.InsetBy(1, 0);
        if (part == ForwardButtonEndPart)
            view->StrokeLine(rect.LeftTop(), rect.RightTop());
        else {
            if (m_drawOuterFrame)
                view->StrokeLine(rect.LeftTop(), rect.RightTop());
            else
                rect.top--;
        }
        if (part == BackButtonStartPart) {
       	    view->SetHighColor(dark3);
            view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
        } else {
            if (m_drawOuterFrame)
                view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
            else
                rect.bottom++;
        }
        rect.InsetBy(0, 1);
    } else {
        orientation = B_HORIZONTAL;
        arrowDirection = part == BackButtonStartPart ? BControlLook::B_LEFT_ARROW : BControlLook::B_RIGHT_ARROW;
        view->SetHighColor(dark2);
        view->StrokeLine(rect.LeftTop(), rect.RightTop());
        if (m_drawOuterFrame)
            view->StrokeLine(rect.LeftBottom(), rect.RightBottom());
        else
            rect.bottom++;
        rect.InsetBy(0, 1);
        if (part == ForwardButtonEndPart)
        	view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
        else {
            if (m_drawOuterFrame)
                view->StrokeLine(rect.LeftTop(), rect.LeftBottom());
            else
                rect.left--;
        }
        if (part == BackButtonStartPart) {
	        view->SetHighColor(dark3);
	        view->StrokeLine(rect.RightTop(), rect.RightBottom());
        } else {
            if (m_drawOuterFrame)
                view->StrokeLine(rect.RightTop(), rect.RightBottom());
            else
                rect.right++;
        }
        rect.InsetBy(1, 0);
    }

	BPoint tri1, tri2, tri3;
	float hInset = rect.Width() / 3;
	float vInset = rect.Height() / 3;
	rect.InsetBy(hInset, vInset);

	switch (arrowDirection) {
		case BControlLook::B_LEFT_ARROW:
			tri1.Set(rect.right, rect.top);
			tri2.Set(rect.right - rect.Width() / 1.33, (rect.top + rect.bottom + 1) /2 );
			tri3.Set(rect.right, rect.bottom + 1);
			break;
		case BControlLook::B_RIGHT_ARROW:
			tri1.Set(rect.left, rect.bottom + 1);
			tri2.Set(rect.left + rect.Width() / 1.33, (rect.top + rect.bottom + 1) / 2);
			tri3.Set(rect.left, rect.top);
			break;
		case BControlLook::B_UP_ARROW:
			tri1.Set(rect.left, rect.bottom);
			tri2.Set((rect.left + rect.right + 1) / 2, rect.bottom - rect.Height() / 1.33);
			tri3.Set(rect.right + 1, rect.bottom);
			break;
		default:
			tri1.Set(rect.left, rect.top);
			tri2.Set((rect.left + rect.right + 1) / 2, rect.top + rect.Height() / 1.33);
			tri3.Set(rect.right + 1, rect.top);
			break;
	}
	// offset triangle if down
	if (down) {
		BPoint offset(1.0, 1.0);
		tri1 = tri1 + offset;
		tri2 = tri2 + offset;
		tri3 = tri3 + offset;
	}

	rect.InsetBy(-(hInset - 1), -(vInset - 1));
	BRect temp(rect.InsetByCopy(-1, -1));
	unsigned flags = 0;
	if (down)
		flags |= BControlLook::B_ACTIVATED;
	be_control_look->DrawButtonBackground(view, temp, rect, down ? dark1 : base, flags, BControlLook::B_ALL_BORDERS, orientation);

	BShape arrowShape;
	arrowShape.MoveTo(tri1);
	arrowShape.LineTo(tri2);
	arrowShape.LineTo(tri3);

	view->SetHighColor(darkMax);
	view->SetPenSize(ceilf(hInset / 2.0));
	view->MovePenTo(B_ORIGIN);
	view->StrokeShape(&arrowShape);
	view->SetPenSize(1.0);
}

void ScrollbarThemeHaiku::paintThumb(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    if (!be_control_look)
        return;

    BRect drawRect = BRect(rect);
    BView* view = context->platformContext();
    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    rgb_color dark2 = tint_color(base, B_DARKEN_2_TINT);
    rgb_color dark3 = tint_color(base, B_DARKEN_3_TINT);

    enum orientation orientation;
    if (scrollbar->orientation() == VerticalScrollbar) {
        orientation = B_VERTICAL;
        drawRect.InsetBy(1, -1);
        if (!m_drawOuterFrame)
        	drawRect.right++;
        view->SetHighColor(dark2);
        view->StrokeLine(drawRect.LeftTop(), drawRect.RightTop());
        view->SetHighColor(dark3);
        view->StrokeLine(drawRect.LeftBottom(), drawRect.RightBottom());
        drawRect.InsetBy(0, 1);
    } else {
        orientation = B_HORIZONTAL;
        drawRect.InsetBy(-1, 1);
        if (!m_drawOuterFrame)
        	drawRect.bottom++;
        view->SetHighColor(dark2);
        view->StrokeLine(drawRect.LeftTop(), drawRect.LeftBottom());
        view->SetHighColor(dark3);
        view->StrokeLine(drawRect.RightTop(), drawRect.RightBottom());
        drawRect.InsetBy(1, 0);
    }

    be_control_look->DrawButtonBackground(view, drawRect, drawRect, base, 0, BControlLook::B_ALL_BORDERS, orientation);
}

void ScrollbarThemeHaiku::paintScrollCorner(ScrollView* scrollView, GraphicsContext* context, const IntRect& rect)
{
	if (rect.width() == 0 || rect.height() == 0)
		return;

    BRect drawRect = BRect(rect);
    BView* view = context->platformContext();
    rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
    if (!m_drawOuterFrame) {
    	view->SetHighColor(tint_color(base, B_DARKEN_2_TINT));
    	view->StrokeLine(drawRect.LeftBottom(), drawRect.LeftTop());
    	drawRect.left++;
    	view->StrokeLine(drawRect.LeftTop(), drawRect.RightTop());
    	drawRect.top++;
    }
    view->SetHighColor(base);
    view->FillRect(drawRect);
}


} // namespace WebCore

