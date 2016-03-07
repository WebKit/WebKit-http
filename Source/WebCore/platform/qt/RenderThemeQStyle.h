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

#include "QStyleFacade.h"
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

    RenderThemeQStyle(Page*);
    virtual ~RenderThemeQStyle();

public:
    static PassRefPtr<RenderTheme> create(Page*);

    static void setStyleFactoryFunction(QtStyleFactoryFunction);
    static QtStyleFactoryFunction styleFactory();

    void adjustSliderThumbSize(RenderStyle&, Element*) const override;

    QStyleFacade* qStyle() { return m_qStyle.get(); }

protected:
    void adjustButtonStyle(StyleResolver&, RenderStyle&, Element*) const override;
    bool paintButton(const RenderObject&, const PaintInfo&, const IntRect&) override;

    bool paintTextField(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    bool paintTextArea(const RenderObject&, const PaintInfo&, const FloatRect&) override;
    void adjustTextAreaStyle(StyleResolver&, RenderStyle&, Element*) const override;

    bool paintMenuList(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    bool paintMenuListButtonDecorations(const RenderBox&, const PaintInfo&, const FloatRect&) override;
    void adjustMenuListButtonStyle(StyleResolver&, RenderStyle&, Element*) const override;

#if ENABLE(PROGRESS_ELEMENT)
    // Returns the duration of the animation for the progress bar.
    virtual double animationDurationForProgressBar(RenderProgress*) const;
    bool paintProgressBar(const RenderObject&, const PaintInfo&, const IntRect&) override;
#endif

    bool paintSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;
    void adjustSliderTrackStyle(StyleResolver&, RenderStyle&, Element*) const override;

    bool paintSliderThumb(const RenderObject&, const PaintInfo&, const IntRect&) override;
    void adjustSliderThumbStyle(StyleResolver&, RenderStyle&, Element*) const override;

    bool paintSearchField(const RenderObject&, const PaintInfo&, const IntRect&) override;

    void adjustSearchFieldDecorationPartStyle(StyleResolver&, RenderStyle&, Element*) const override;
    bool paintSearchFieldDecorationPart(const RenderObject&, const PaintInfo&, const IntRect&) override;

    void adjustSearchFieldResultsDecorationPartStyle(StyleResolver&, RenderStyle&, Element*) const override;
    bool paintSearchFieldResultsDecorationPart(const RenderBox&, const PaintInfo&, const IntRect&) override;

#ifndef QT_NO_SPINBOX
    bool paintInnerSpinButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
#endif

protected:
    void computeSizeBasedOnStyle(RenderStyle&) const override;

    QSharedPointer<StylePainter> getStylePainter(const PaintInfo&) override;

    QRect inflateButtonRect(const QRect& originalRect) const override;
    QRectF inflateButtonRect(const QRectF& originalRect) const override;

    void setPopupPadding(RenderStyle&) const override;

    QPalette colorPalette() const override;

private:
    ControlPart initializeCommonQStyleOptions(QStyleFacadeOption&, const RenderObject&) const;

    void setButtonPadding(RenderStyle&) const;

    void setPaletteFromPageClientIfExists(QPalette&) const;

#ifdef Q_OS_MAC
    int m_buttonFontPixelSize;
#endif

    std::unique_ptr<QStyleFacade> m_qStyle;
};

class StylePainterQStyle : public StylePainter {
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
