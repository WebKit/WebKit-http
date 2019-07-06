/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2011-2012 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef RenderThemeQStyle_h
#define RenderThemeQStyle_h

#include "RenderThemeQt.h"

namespace WebCore {

class ScrollbarThemeQStyle;

class Page;
class QStyleFacade;
struct QStyleFacadeOption;

typedef QStyleFacade* (*QtStyleFactoryFunction)(Page*);

class RenderThemeQStyle final : public RenderThemeQt {
private:
    friend class StylePainterQStyle;

public:
    RenderThemeQStyle(Page*);
    ~RenderThemeQStyle();

    static RenderTheme& singleton();

    static void setStyleFactoryFunction(QtStyleFactoryFunction);
    static QtStyleFactoryFunction styleFactory();

    void adjustSliderThumbSize(RenderStyle&, const Element*) const final;

    QStyleFacade* qStyle() { return m_qStyle.get(); }

protected:
    void adjustButtonStyle(StyleResolver&, RenderStyle&, const Element*) const final;
    bool paintButton(const RenderObject&, const PaintInfo&, const IntRect&) final;

    bool paintTextField(const RenderObject&, const PaintInfo&, const FloatRect&) final;

    bool paintTextArea(const RenderObject&, const PaintInfo&, const FloatRect&) final;
    void adjustTextAreaStyle(StyleResolver&, RenderStyle&, const Element*) const final;

    bool paintMenuList(const RenderObject&, const PaintInfo&, const FloatRect&) final;

    bool paintMenuListButtonDecorations(const RenderBox&, const PaintInfo&, const FloatRect&) final;
    void adjustMenuListButtonStyle(StyleResolver&, RenderStyle&, const Element*) const final;

    // Returns the duration of the animation for the progress bar.
    Seconds animationDurationForProgressBar(RenderProgress&) const final;
    bool paintProgressBar(const RenderObject&, const PaintInfo&, const IntRect&) final;

    bool paintSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) final;
    void adjustSliderTrackStyle(StyleResolver&, RenderStyle&, const Element*) const final;

    bool paintSliderThumb(const RenderObject&, const PaintInfo&, const IntRect&) final;
    void adjustSliderThumbStyle(StyleResolver&, RenderStyle&, const Element*) const final;

    bool paintSearchField(const RenderObject&, const PaintInfo&, const IntRect&) final;

    void adjustSearchFieldDecorationPartStyle(StyleResolver&, RenderStyle&, const Element*) const final;
    bool paintSearchFieldDecorationPart(const RenderObject&, const PaintInfo&, const IntRect&) final;

    void adjustSearchFieldResultsDecorationPartStyle(StyleResolver&, RenderStyle&, const Element*) const final;
    bool paintSearchFieldResultsDecorationPart(const RenderBox&, const PaintInfo&, const IntRect&) final;

#ifndef QT_NO_SPINBOX
    bool paintInnerSpinButton(const RenderObject&, const PaintInfo&, const IntRect&) final;
#endif

protected:
    void computeSizeBasedOnStyle(RenderStyle&) const final;

    QSharedPointer<StylePainter> getStylePainter(const PaintInfo&) final;

    QRect inflateButtonRect(const QRect& originalRect) const final;
    QRectF inflateButtonRect(const QRectF& originalRect) const final;
    void computeControlRect(QStyleFacade::ButtonType, QRect& originalRect) const final;
    void computeControlRect(QStyleFacade::ButtonType, FloatRect& originalRect) const final;

    void setPopupPadding(RenderStyle&) const final;

    QPalette colorPalette() const final;

private:
    ControlPart initializeCommonQStyleOptions(QStyleFacadeOption&, const RenderObject&) const;

    void setButtonPadding(RenderStyle&) const;

    void setPaletteFromPageClientIfExists(QPalette&) const;

    QRect indicatorRect(QStyleFacade::ButtonType part, const QRect& originalRect) const;

#ifdef Q_OS_MACOS
    int m_buttonFontPixelSize;
#endif

    std::unique_ptr<QStyleFacade> m_qStyle;
};

class StylePainterQStyle final : public StylePainter {
public:
    explicit StylePainterQStyle(RenderThemeQStyle*, const PaintInfo&);
    explicit StylePainterQStyle(RenderThemeQStyle*, const PaintInfo&, const RenderObject&);
    explicit StylePainterQStyle(ScrollbarThemeQStyle*, GraphicsContext&);

    bool isValid() const { return qStyle && qStyle->isValid() && StylePainter::isValid(); }

    QStyleFacade* qStyle;
    QStyleFacadeOption styleOption;
    ControlPart appearance;

    void paintButton(QStyleFacade::ButtonType type)
    { qStyle->paintButton(painter, type, styleOption); }
    void paintTextField()
    { qStyle->paintTextField(painter, styleOption); }
    void paintComboBox()
    { qStyle->paintComboBox(painter, styleOption); }
    void paintComboBoxArrow()
    { qStyle->paintComboBoxArrow(painter, styleOption); }
    void paintSliderTrack()
    { qStyle->paintSliderTrack(painter, styleOption); }
    void paintSliderThumb()
    { qStyle->paintSliderThumb(painter, styleOption); }
    void paintInnerSpinButton(bool spinBoxUp)
    { qStyle->paintInnerSpinButton(painter, styleOption, spinBoxUp); }
    void paintProgressBar(double progress, double animationProgress)
    { qStyle->paintProgressBar(painter, styleOption, progress, animationProgress); }
    void paintScrollCorner(const QRect& rect)
    { qStyle->paintScrollCorner(painter, rect); }
    void paintScrollBar()
    { qStyle->paintScrollBar(painter, styleOption); }

private:
    void setupStyleOption();

    Q_DISABLE_COPY(StylePainterQStyle)
};

}

#endif // RenderThemeQStyle_h
