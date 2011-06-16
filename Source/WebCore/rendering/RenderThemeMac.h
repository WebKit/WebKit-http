/*
 * This file is part of the theme implementation for form controls in WebCore.
 *
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Apple Computer, Inc.
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

#ifndef RenderThemeMac_h
#define RenderThemeMac_h

#import "RenderTheme.h"
#import <wtf/HashMap.h>
#import <wtf/RetainPtr.h>

class RenderProgress;

#ifdef __OBJC__
@class WebCoreRenderThemeNotificationObserver;
#else
class WebCoreRenderThemeNotificationObserver;
#endif

namespace WebCore {

class RenderStyle;

class RenderThemeMac : public RenderTheme {
public:
    static PassRefPtr<RenderTheme> create();

    // A method asking if the control changes its tint when the window has focus or not.
    virtual bool controlSupportsTints(const RenderObject*) const;

    // A general method asking if any control tinting is supported at all.
    virtual bool supportsControlTints() const { return true; }

    virtual void adjustRepaintRect(const RenderObject*, IntRect&);

    virtual bool isControlStyled(const RenderStyle*, const BorderData&,
                                 const FillLayer&, const Color& backgroundColor) const;

    virtual Color platformActiveSelectionBackgroundColor() const;
    virtual Color platformInactiveSelectionBackgroundColor() const;
    virtual Color platformActiveListBoxSelectionBackgroundColor() const;
    virtual Color platformActiveListBoxSelectionForegroundColor() const;
    virtual Color platformInactiveListBoxSelectionBackgroundColor() const;
    virtual Color platformInactiveListBoxSelectionForegroundColor() const;
    virtual Color platformFocusRingColor() const;

    virtual ScrollbarControlSize scrollbarControlSizeForPart(ControlPart) { return SmallScrollbar; }
    
    virtual void platformColorsDidChange();

    // System fonts.
    virtual void systemFont(int cssValueId, FontDescription&) const;

    virtual int minimumMenuListSize(RenderStyle*) const;

    virtual void adjustSliderThumbSize(RenderStyle*) const;
    
    virtual int popupInternalPaddingLeft(RenderStyle*) const;
    virtual int popupInternalPaddingRight(RenderStyle*) const;
    virtual int popupInternalPaddingTop(RenderStyle*) const;
    virtual int popupInternalPaddingBottom(RenderStyle*) const;
    
    virtual bool paintCapsLockIndicator(RenderObject*, const PaintInfo&, const IntRect&);

#if ENABLE(METER_TAG)
    virtual IntSize meterSizeForBounds(const RenderMeter*, const IntRect&) const;
    virtual bool paintMeter(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool supportsMeter(ControlPart) const;
#endif

#if ENABLE(PROGRESS_TAG)
    // Returns the repeat interval of the animation for the progress bar.
    virtual double animationRepeatIntervalForProgressBar(RenderProgress*) const;
    // Returns the duration of the animation for the progress bar.
    virtual double animationDurationForProgressBar(RenderProgress*) const;
#endif

    virtual Color systemColor(int cssValueId) const;
    // Controls color values returned from platformFocusRingColor(). systemColor() will be used when false.
    virtual bool usesTestModeFocusRingColor() const;
    // A view associated to the contained document. Subclasses may not have such a view and return a fake.
    virtual NSView* documentViewFor(RenderObject*) const;
protected:
    RenderThemeMac();
    virtual ~RenderThemeMac();

    virtual bool supportsSelectionForegroundColors() const { return false; }

    virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustTextFieldStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintTextArea(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustTextAreaStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustMenuListStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustMenuListButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

#if ENABLE(PROGRESS_TAG)
    virtual void adjustProgressBarStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintProgressBar(RenderObject*, const PaintInfo&, const IntRect&);
#endif

    virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSliderTrackStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSliderThumbStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void adjustSearchFieldStyle(CSSStyleSelector*, RenderStyle*, Element*) const;

    virtual void adjustSearchFieldCancelButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldCancelButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldDecorationStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldDecoration(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldResultsDecorationStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsDecoration(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldResultsButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldResultsButton(RenderObject*, const PaintInfo&, const IntRect&);

#if ENABLE(VIDEO)
    virtual bool paintMediaFullscreenButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaPlayButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaMuteButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSeekBackButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSeekForwardButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaRewindButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaReturnToRealtimeButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaToggleClosedCaptionsButton(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaControlsBackground(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaCurrentTime(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaTimeRemaining(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderContainer(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintMediaVolumeSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);

    // Media controls
    virtual String extraMediaControlsStyleSheet();
#if ENABLE(FULLSCREEN_API)
    virtual String extraFullScreenStyleSheet();
#endif

    virtual bool supportsClosedCaptioning() const { return true; }
    virtual bool hasOwnDisabledStateHandlingFor(ControlPart) const;
    virtual bool usesMediaControlStatusDisplay();
    virtual bool usesMediaControlVolumeSlider() const;
    virtual void adjustMediaSliderThumbSize(RenderStyle*) const;
    virtual IntPoint volumeSliderOffsetFromMuteButton(RenderBox*, const IntSize&) const;
#endif
    
    virtual bool shouldShowPlaceholderWhenFocused() const;

private:
    virtual String fileListNameForWidth(const Vector<String>& filenames, const Font&, int width);

    IntRect inflateRect(const IntRect&, const IntSize&, const int* margins, float zoomLevel = 1.0f) const;

    FloatRect convertToPaintingRect(const RenderObject* inputRenderer, const RenderObject* partRenderer, const FloatRect& inputRect, const IntRect& r) const;
    
    // Get the control size based off the font.  Used by some of the controls (like buttons).
    NSControlSize controlSizeForFont(RenderStyle*) const;
    NSControlSize controlSizeForSystemFont(RenderStyle*) const;
    void setControlSize(NSCell*, const IntSize* sizes, const IntSize& minSize, float zoomLevel = 1.0f);
    void setSizeFromFont(RenderStyle*, const IntSize* sizes) const;
    IntSize sizeForFont(RenderStyle*, const IntSize* sizes) const;
    IntSize sizeForSystemFont(RenderStyle*, const IntSize* sizes) const;
    void setFontFromControlSize(CSSStyleSelector*, RenderStyle*, NSControlSize) const;

    void updateCheckedState(NSCell*, const RenderObject*);
    void updateEnabledState(NSCell*, const RenderObject*);
    void updateFocusedState(NSCell*, const RenderObject*);
    void updatePressedState(NSCell*, const RenderObject*);
    // An optional hook for subclasses to update the control tint of NSCell.
    virtual void updateActiveState(NSCell*, const RenderObject*) {}

    // Helpers for adjusting appearance and for painting

    void setPopupButtonCellState(const RenderObject*, const IntRect&);
    const IntSize* popupButtonSizes() const;
    const int* popupButtonMargins() const;
    const int* popupButtonPadding(NSControlSize) const;
    void paintMenuListButtonGradients(RenderObject*, const PaintInfo&, const IntRect&);
    const IntSize* menuListSizes() const;

    const IntSize* searchFieldSizes() const;
    const IntSize* cancelButtonSizes() const;
    const IntSize* resultsButtonSizes() const;
    void setSearchCellState(RenderObject*, const IntRect&);
    void setSearchFieldSize(RenderStyle*) const;
    
    NSPopUpButtonCell* popupButton() const;
    NSSearchFieldCell* search() const;
    NSMenu* searchMenuTemplate() const;
    NSSliderCell* sliderThumbHorizontal() const;
    NSSliderCell* sliderThumbVertical() const;

#if ENABLE(METER_TAG)
    NSLevelIndicatorStyle levelIndicatorStyleFor(ControlPart) const;
    NSLevelIndicatorCell* levelIndicatorFor(const RenderMeter*) const;
#endif

private:
    mutable RetainPtr<NSPopUpButtonCell> m_popupButton;
    mutable RetainPtr<NSSearchFieldCell> m_search;
    mutable RetainPtr<NSMenu> m_searchMenuTemplate;
    mutable RetainPtr<NSSliderCell> m_sliderThumbHorizontal;
    mutable RetainPtr<NSSliderCell> m_sliderThumbVertical;
    mutable RetainPtr<NSLevelIndicatorCell> m_levelIndicator;

    bool m_isSliderThumbHorizontalPressed;
    bool m_isSliderThumbVerticalPressed;

    mutable HashMap<int, RGBA32> m_systemColorCache;

    RetainPtr<WebCoreRenderThemeNotificationObserver> m_notificationObserver;
};

} // namespace WebCore

#endif // RenderThemeMac_h
