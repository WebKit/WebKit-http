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

    // A method asking if the theme's controls actually care about redrawing when hovered.
    virtual bool supportsHover(const RenderStyle&) const override { return true; }

    // A method asking if the theme is able to draw the focus ring.
    bool supportsFocusRing(const RenderStyle&) const override;

    // The platform selection color.
    Color platformActiveSelectionBackgroundColor() const override;
    Color platformInactiveSelectionBackgroundColor() const override;
    Color platformActiveSelectionForegroundColor() const override;
    Color platformInactiveSelectionForegroundColor() const override;

    Color platformFocusRingColor() const override;

    // System fonts.
    void updateCachedSystemFontDescription(CSSValueID propId, FontDescription&) const override;

#if ENABLE(VIDEO)
    String mediaControlsStyleSheet() override;
    String mediaControlsScript() override;
#endif
protected:
#if USE(NEW_THEME)
#else
    bool paintCheckbox(const RenderObject&, const PaintInfo&, const IntRect&) override;
    void setCheckboxSize(RenderStyle&) const override;

    bool paintRadio(const RenderObject&, const PaintInfo&, const IntRect&) override;
    void setRadioSize(RenderStyle&) const override;

    bool paintButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
#endif

    void adjustTextFieldStyle(StyleResolver&, RenderStyle&, Element*) const override;
    bool paintTextField(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    void adjustTextAreaStyle(StyleResolver&, RenderStyle&, Element*) const override;
    bool paintTextArea(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    void adjustMenuListStyle(StyleResolver&, RenderStyle&, Element*) const override;
    bool paintMenuList(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    void adjustMenuListButtonStyle(StyleResolver&, RenderStyle&, Element*) const override;
    bool paintMenuListButtonDecorations(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    void adjustSliderTrackStyle(StyleResolver&, RenderStyle&, Element*) const override;
    bool paintSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;

    void adjustSliderThumbStyle(StyleResolver&, RenderStyle&, Element*) const override;

    void adjustSliderThumbSize(RenderStyle&, Element*) const override;

#if ENABLE(DATALIST_ELEMENT)
    // Returns size of one slider tick mark for a horizontal track.
    // For vertical tracks we rotate it and use it. i.e. Width is always length along the track.
    IntSize sliderTickSize() const override;
    // Returns the distance of slider tick origin from the slider track center.
    int sliderTickOffsetFromTrackCenter() const override;
#endif

    bool supportsDataListUI(const AtomicString&) const override;

    bool paintSliderThumb(const RenderObject&, const PaintInfo&, const IntRect&) override;

private:
	unsigned flagsForObject(const RenderObject&) const;
};

} // namespace WebCore

#endif // RenderThemeHaiku_h
