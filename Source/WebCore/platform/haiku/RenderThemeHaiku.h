/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
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

#ifndef RenderThemeHaiku_h
#define RenderThemeHaiku_h

#include "RenderTheme.h"

namespace WebCore {

class PaintInfo;

class RenderThemeHaiku : public RenderTheme {
private:
    RenderThemeHaiku();
    virtual ~RenderThemeHaiku();

public:
    static PassRefPtr<RenderTheme> create();

    // A method asking if the theme is able to draw the focus ring.
    bool supportsFocusRing(const RenderStyle*) const override;

    // The platform selection color.
    Color platformActiveSelectionBackgroundColor() const override;
    Color platformInactiveSelectionBackgroundColor() const override;
    Color platformActiveSelectionForegroundColor() const override;
    Color platformInactiveSelectionForegroundColor() const override;

    // System fonts.
    void systemFont(CSSValueID propId, FontDescription&) const override;

#if ENABLE(VIDEO)
    String mediaControlsStyleSheet() override;
    String mediaControlsScript() override;
#endif
protected:
    bool paintCheckbox(RenderObject*, const PaintInfo&, const IntRect&) override;
    void setCheckboxSize(RenderStyle*) const override;

    bool paintRadio(RenderObject*, const PaintInfo&, const IntRect&) override;
    void setRadioSize(RenderStyle*) const override;

    bool paintButton(RenderObject*, const PaintInfo&, const IntRect&) override;

    void adjustTextFieldStyle(StyleResolver*, RenderStyle*, Element*) const override;
    bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&) override;

    void adjustTextAreaStyle(StyleResolver*, RenderStyle*, Element*) const override;
    bool paintTextArea(RenderObject*, const PaintInfo&, const IntRect&) override;

    void adjustMenuListStyle(StyleResolver*, RenderStyle*, Element*) const override;
    bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&) override;

    void adjustMenuListButtonStyle(StyleResolver*, RenderStyle*, Element*) const override;

private:
	unsigned flagsForObject(RenderObject*) const;
};

} // namespace WebCore

#endif // RenderThemeHaiku_h
