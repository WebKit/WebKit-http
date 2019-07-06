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
#include <QHash>
#include <QPixmapCache>

QT_BEGIN_NAMESPACE
class QColor;
class QSize;
QT_END_NAMESPACE

typedef QPixmapCache::Key CacheKey;

namespace WebCore {

class RenderThemeQtMobile final : public RenderThemeQt {
public:
    RenderThemeQtMobile(Page*);
    ~RenderThemeQtMobile();

    static RenderTheme& singleton();

    void adjustSliderThumbSize(RenderStyle&, const Element*) const final;

    bool isControlStyled(const RenderStyle&, const BorderData&, const FillLayer&, const Color& backgroundColor) const final;

    LengthBox popupInternalPaddingBox(const RenderStyle&) const final;

    bool delegatesMenuListRendering() const final { return true; }

    // We don't want the focus ring to be drawn by the graphics context so we
    // always claim to support it in the theme.
    // FIXME: This could be a usability problem in the case of contenteditable divs.
    bool supportsFocusRing(const RenderStyle&) const final { return true; }

protected:

    void adjustButtonStyle(StyleResolver&, RenderStyle&, const Element*) const final;
    bool paintButton(const RenderObject&, const PaintInfo&, const IntRect&) final;

    bool paintTextField(const RenderObject&, const PaintInfo&, const FloatRect&) final;
    void adjustTextFieldStyle(StyleResolver&, RenderStyle&, const Element*) const final;

    bool paintMenuList(const RenderObject&, const PaintInfo&, const FloatRect&) final;
    void adjustMenuListStyle(StyleResolver&, RenderStyle&, const Element*) const final;

    virtual bool paintMenuListButton(RenderObject&, const PaintInfo&, const IntRect&);

    // Returns the duration of the animation for the progress bar.
    Seconds animationDurationForProgressBar(RenderProgress&) const final;
    bool paintProgressBar(const RenderObject&, const PaintInfo&, const IntRect&) final;

    bool paintSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) final;
    bool paintSliderThumb(const RenderObject&, const PaintInfo&, const IntRect&) final;

    void computeSizeBasedOnStyle(RenderStyle&) const final;
    QSharedPointer<StylePainter> getStylePainter(const PaintInfo&) final;

    QPalette colorPalette() const final;

private:
    bool checkMultiple(const RenderObject&) const;
    void setButtonPadding(RenderStyle&) const;
    void setPopupPadding(RenderStyle&) const final;
};

struct KeyIdentifier {

    KeyIdentifier()
        : type(Undefined)
        , width(0)
        , height(0)
        , trait1(0)
        , trait2(0)
        , trait3(0)
    {
    }

    enum ControlType {
        Undefined,
        CheckBox,
        Radio,
        ComboButton,
        LineEdit,
        PushButton,
        Progress,
        SliderThumb
    };

    ControlType type : 3;
    uint width : 11;
    uint height : 9;
    uint trait1 : 1;
    uint trait2 : 1;
    uint trait3 : 7;

    inline bool operator==(const KeyIdentifier& other) const
    {
        return (type == other.type && width == other.width
                && height == other.height && trait1 == other.trait1
                && trait2 == other.trait2 && trait3 == other.trait3);
    }
};

class StylePainterMobile final : public StylePainter {

public:
    explicit StylePainterMobile(RenderThemeQtMobile*, const PaintInfo&);
    ~StylePainterMobile();

    void drawLineEdit(const QRectF&, bool focused, bool enabled = true);
    void drawCheckBox(const QRect&, bool checked, bool enabled = true);
    void drawRadioButton(const QRect&, bool checked, bool enabled = true);
    void drawPushButton(const QRect&, bool sunken, bool enabled = true);
    void drawComboBox(const QRect&, bool multiple, bool enabled = true);
    void drawComboBox(const QRectF&, bool multiple, bool enabled = true);
    void drawProgress(const QRect&, double progress, bool leftToRight = true, bool animated = false, bool vertical = false) const;
    void drawSliderThumb(const QRect&, bool pressed) const;

private:
    void drawCheckableBackground(QPainter*, const QRect&, bool checked, bool enabled) const;
    void drawChecker(QPainter*, const QRect&, const QColor&) const;
    QPixmap findCheckBox(const QSize&, bool checked, bool enabled) const;

    void drawRadio(QPainter*, const QSize&, bool checked, bool enabled) const;
    QPixmap findRadio(const QSize&, bool checked, bool enabled) const;

    QSizeF getButtonImageSize(int , bool multiple) const;
    void drawSimpleComboButton(QPainter*, const QSizeF&, const QColor&) const;
    void drawMultipleComboButton(QPainter*, const QSizeF&, const QColor&) const;
    QPixmap findComboButton(const QSize&, bool multiple, bool enabled) const;

    QPixmap findLineEdit(const QSize&, bool focused) const;
    QPixmap findLineEdit(const QSizeF&, bool focused) const;
    QPixmap findPushButton(const QSize&, bool sunken, bool enabled) const;
    QPixmap findPushButton(const QSizeF&, bool sunken, bool enabled) const;

    QSize sizeForPainterScale(const QRect&) const;
    QSizeF sizeForPainterScale(const QRectF&) const;

    static bool findCachedControl(const KeyIdentifier&, QPixmap*);
    static void insertIntoCache(const KeyIdentifier&, const QPixmap&);

    bool m_previousSmoothPixmapTransform;

    Q_DISABLE_COPY(StylePainterMobile)
};

}

#endif // RenderThemeQtMobile_h
