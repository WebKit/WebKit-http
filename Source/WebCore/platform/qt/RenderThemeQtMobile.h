/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef RenderThemeQtMobile_h
#define RenderThemeQtMobile_h

#include "RenderThemeQt.h"

#include <QBrush>

QT_BEGIN_NAMESPACE
class QColor;
class QPixmap;
class QSize;
QT_END_NAMESPACE

namespace WebCore {

class RenderThemeQtMobile : public RenderThemeQt {
private:
    RenderThemeQtMobile(Page*);
    virtual ~RenderThemeQtMobile();

public:
    static PassRefPtr<RenderTheme> create(Page*);

    virtual void adjustSliderThumbSize(RenderStyle*) const;

    virtual bool isControlStyled(const RenderStyle*, const BorderData&, const FillLayer&, const Color& backgroundColor) const;

    virtual int popupInternalPaddingBottom(RenderStyle*) const;

protected:

    virtual void adjustButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustTextFieldStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustMenuListStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&);

#if ENABLE(PROGRESS_TAG)
    // Returns the duration of the animation for the progress bar.
    virtual double animationDurationForProgressBar(RenderProgress*) const;
    virtual bool paintProgressBar(RenderObject*, const PaintInfo&, const IntRect&);
#endif

    virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void computeSizeBasedOnStyle(RenderStyle*) const;
    virtual QSharedPointer<StylePainter> getStylePainter(const PaintInfo&);

private:
    bool checkMultiple(RenderObject*) const;
    void setButtonPadding(RenderStyle*) const;
    void setPopupPadding(RenderStyle*) const;

    void setPaletteFromPageClientIfExists(QPalette&) const;
};

class StylePainterMobile : public StylePainter {
public:
    explicit StylePainterMobile(RenderThemeQtMobile*, const PaintInfo&);

    void drawLineEdit(const QRect&, bool sunken, bool enabled = true);
    void drawCheckBox(const QRect&, bool checked, bool enabled = true);
    void drawRadioButton(const QRect&, bool checked, bool enabled = true);
    void drawPushButton(const QRect&, bool sunken, bool enabled = true);
    void drawComboBox(const QRect&, bool multiple, bool enabled = true);

private:
    void drawChecker(QPainter*, int size, QColor) const;
    QPixmap findChecker(const QRect&, bool disabled) const;

    void drawRadio(QPainter*, const QSize&, bool checked, QColor) const;
    QPixmap findRadio(const QSize&, bool checked, bool disabled) const;

    QSize getButtonImageSize(const QSize&) const;
    void drawSimpleComboButton(QPainter*, const QSize&, QColor) const;
    void drawMultipleComboButton(QPainter*, const QSize&, QColor) const;
    QPixmap findComboButton(const QSize&, bool multiple, bool disabled) const;

    Q_DISABLE_COPY(StylePainterMobile);
};
}

#endif // RenderThemeQtMobile_h
