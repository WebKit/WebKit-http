/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2015 The Qt Company Ltd.
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
#ifndef QStyleFacadeImp_h
#define QStyleFacadeImp_h

#include <QPointer>
#include <WebCore/QStyleFacade.h>

QT_BEGIN_NAMESPACE
class QStyle;
class QLineEdit;
class QStyleOption;
class QStyleOptionSlider;
class QPainter;
class QObject;
QT_END_NAMESPACE

class QWebPageAdapter;

namespace WebKit {

class QStyleFacadeImp : public WebCore::QStyleFacade {
public:
    QStyleFacadeImp(QWebPageAdapter* = 0);
    ~QStyleFacadeImp() override;

    static WebCore::QStyleFacade* create(QWebPageAdapter* page)
    { return new QStyleFacadeImp(page); }

    QRect buttonSubElementRect(ButtonSubElement, State, const QRect& originalRect) const override;

    int findFrameLineWidth() const override;
    int simplePixelMetric(PixelMetric, State = State_None) const override;
    int buttonMargin(State, const QRect& originalRect) const override;
    int sliderLength(Qt::Orientation) const override;
    int sliderThickness(Qt::Orientation) const override;
    int progressBarChunkWidth(const QSize&) const override;
    void getButtonMetrics(QString* buttonFontFamily, int* buttonFontPixelSize) const override;

    QSize comboBoxSizeFromContents(State, const QSize& contentsSize) const override;
    QSize pushButtonSizeFromContents(State, const QSize& contentsSize) const override;

    void paintButton(QPainter*, ButtonType, const WebCore::QStyleFacadeOption &proxyOption) override;
    void paintTextField(QPainter*, const WebCore::QStyleFacadeOption&) override;
    void paintComboBox(QPainter*, const WebCore::QStyleFacadeOption&) override;
    void paintComboBoxArrow(QPainter*, const WebCore::QStyleFacadeOption&) override;

    void paintSliderTrack(QPainter*, const WebCore::QStyleFacadeOption&) override;
    void paintSliderThumb(QPainter*, const WebCore::QStyleFacadeOption&) override;
    void paintInnerSpinButton(QPainter*, const WebCore::QStyleFacadeOption&, bool spinBoxUp) override;
    void paintProgressBar(QPainter*, const WebCore::QStyleFacadeOption&, double progress, double animationProgress) override;

    int scrollBarExtent(bool mini) override;
    bool scrollBarMiddleClickAbsolutePositionStyleHint() const override;
    void paintScrollCorner(QPainter*, const QRect&) override;

    SubControl hitTestScrollBar(const WebCore::QStyleFacadeOption&, const QPoint& pos) override;
    QRect scrollBarSubControlRect(const WebCore::QStyleFacadeOption&, SubControl) override;
    void paintScrollBar(QPainter*, const WebCore::QStyleFacadeOption&) override;

    QObject* widgetForPainter(QPainter*) override;

    bool isValid() const override { return style(); }

private:
    QStyle* style() const;

    QWebPageAdapter* m_page;
    mutable QPointer<QStyle> m_style;
    QStyle* m_fallbackStyle;
    bool m_ownFallbackStyle;
    mutable QScopedPointer<QLineEdit> m_lineEdit;
};

}

#endif // QStyleFacadeImp_h
