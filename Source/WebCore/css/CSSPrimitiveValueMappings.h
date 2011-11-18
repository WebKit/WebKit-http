/*
 * Copyright (C) 2007 Alexey Proskuryakov <ap@nypop.com>.
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2009 Jeff Schiller <codedread@gmail.com>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CSSPrimitiveValueMappings_h
#define CSSPrimitiveValueMappings_h

#include "ColorSpace.h"
#include "CSSPrimitiveValue.h"
#include "CSSValueKeywords.h"
#include "FontDescription.h"
#include "FontSmoothingMode.h"
#include "GraphicsTypes.h"
#include "Path.h"
#include "RenderStyleConstants.h"
#include "SVGRenderStyleDefs.h"
#include "TextDirection.h"
#include "TextOrientation.h"
#include "TextRenderingMode.h"
#include "ThemeTypes.h"
#include "UnicodeBidi.h"

#if ENABLE(CSS_SHADERS)
#include "CustomFilterOperation.h"
#endif

#include <wtf/MathExtras.h>

namespace WebCore {

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(short i)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_NUMBER;
    m_value.num = static_cast<double>(i);
}

template<> inline CSSPrimitiveValue::operator short() const
{
    if (m_primitiveUnitType == CSS_NUMBER)
        return clampTo<short>(m_value.num);

    ASSERT_NOT_REACHED();
    return 0;
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(unsigned short i)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_NUMBER;
    m_value.num = static_cast<double>(i);
}

template<> inline CSSPrimitiveValue::operator unsigned short() const
{
    if (m_primitiveUnitType == CSS_NUMBER)
        return clampTo<unsigned short>(m_value.num);

    ASSERT_NOT_REACHED();
    return 0;
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(int i)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_NUMBER;
    m_value.num = static_cast<double>(i);
}

template<> inline CSSPrimitiveValue::operator int() const
{
    if (m_primitiveUnitType == CSS_NUMBER)
        return clampTo<int>(m_value.num);

    ASSERT_NOT_REACHED();
    return 0;
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(unsigned i)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_NUMBER;
    m_value.num = static_cast<double>(i);
}

template<> inline CSSPrimitiveValue::operator unsigned() const
{
    if (m_primitiveUnitType == CSS_NUMBER)
        return clampTo<unsigned>(m_value.num);

    ASSERT_NOT_REACHED();
    return 0;
}


template<> inline CSSPrimitiveValue::CSSPrimitiveValue(float i)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_NUMBER;
    m_value.num = static_cast<double>(i);
}

template<> inline CSSPrimitiveValue::operator float() const
{
    if (m_primitiveUnitType == CSS_NUMBER)
        return clampTo<float>(m_value.num);

    ASSERT_NOT_REACHED();
    return 0.0f;
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ColumnSpan columnSpan)
    : CSSValue(PrimitiveClass)
{
    switch (columnSpan) {
    case ColumnSpanAll:
        m_primitiveUnitType = CSS_IDENT;
        m_value.ident = CSSValueAll;
        break;
    case ColumnSpanOne:
        m_primitiveUnitType = CSS_NUMBER;
        m_value.num = 1;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator ColumnSpan() const
{
    if (m_primitiveUnitType == CSS_IDENT && m_value.ident == CSSValueAll)
        return ColumnSpanAll;
    if (m_primitiveUnitType == CSS_NUMBER && m_value.num == 1)
        return ColumnSpanOne;
    ASSERT_NOT_REACHED();
    return ColumnSpanOne;
}


template<> inline CSSPrimitiveValue::CSSPrimitiveValue(PrintColorAdjust value)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (value) {
    case PrintColorAdjustExact:
        m_value.ident = CSSValueExact;
        break;
    case PrintColorAdjustEconomy:
        m_value.ident = CSSValueEconomy;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator PrintColorAdjust() const
{
    switch (m_value.ident) {
    case CSSValueEconomy:
        return PrintColorAdjustEconomy;
    case CSSValueExact:
        return PrintColorAdjustExact;
    default:
        ASSERT_NOT_REACHED();
        return PrintColorAdjustEconomy;
    }
}


template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EBorderStyle e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case BNONE:
            m_value.ident = CSSValueNone;
            break;
        case BHIDDEN:
            m_value.ident = CSSValueHidden;
            break;
        case INSET:
            m_value.ident = CSSValueInset;
            break;
        case GROOVE:
            m_value.ident = CSSValueGroove;
            break;
        case RIDGE:
            m_value.ident = CSSValueRidge;
            break;
        case OUTSET:
            m_value.ident = CSSValueOutset;
            break;
        case DOTTED:
            m_value.ident = CSSValueDotted;
            break;
        case DASHED:
            m_value.ident = CSSValueDashed;
            break;
        case SOLID:
            m_value.ident = CSSValueSolid;
            break;
        case DOUBLE:
            m_value.ident = CSSValueDouble;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EBorderStyle() const
{
    if (m_value.ident == CSSValueAuto) // Valid for CSS outline-style
        return DOTTED;
    return (EBorderStyle)(m_value.ident - CSSValueNone);
}

template<> inline CSSPrimitiveValue::operator OutlineIsAuto() const
{
    if (m_value.ident == CSSValueAuto)
        return AUTO_ON;
    return AUTO_OFF;
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(CompositeOperator e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case CompositeClear:
            m_value.ident = CSSValueClear;
            break;
        case CompositeCopy:
            m_value.ident = CSSValueCopy;
            break;
        case CompositeSourceOver:
            m_value.ident = CSSValueSourceOver;
            break;
        case CompositeSourceIn:
            m_value.ident = CSSValueSourceIn;
            break;
        case CompositeSourceOut:
            m_value.ident = CSSValueSourceOut;
            break;
        case CompositeSourceAtop:
            m_value.ident = CSSValueSourceAtop;
            break;
        case CompositeDestinationOver:
            m_value.ident = CSSValueDestinationOver;
            break;
        case CompositeDestinationIn:
            m_value.ident = CSSValueDestinationIn;
            break;
        case CompositeDestinationOut:
            m_value.ident = CSSValueDestinationOut;
            break;
        case CompositeDestinationAtop:
            m_value.ident = CSSValueDestinationAtop;
            break;
        case CompositeXOR:
            m_value.ident = CSSValueXor;
            break;
        case CompositePlusDarker:
            m_value.ident = CSSValuePlusDarker;
            break;
        case CompositePlusLighter:
            m_value.ident = CSSValuePlusLighter;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator CompositeOperator() const
{
    switch (m_value.ident) {
        case CSSValueClear:
            return CompositeClear;
        case CSSValueCopy:
            return CompositeCopy;
        case CSSValueSourceOver:
            return CompositeSourceOver;
        case CSSValueSourceIn:
            return CompositeSourceIn;
        case CSSValueSourceOut:
            return CompositeSourceOut;
        case CSSValueSourceAtop:
            return CompositeSourceAtop;
        case CSSValueDestinationOver:
            return CompositeDestinationOver;
        case CSSValueDestinationIn:
            return CompositeDestinationIn;
        case CSSValueDestinationOut:
            return CompositeDestinationOut;
        case CSSValueDestinationAtop:
            return CompositeDestinationAtop;
        case CSSValueXor:
            return CompositeXOR;
        case CSSValuePlusDarker:
            return CompositePlusDarker;
        case CSSValuePlusLighter:
            return CompositePlusLighter;
        default:
            ASSERT_NOT_REACHED();
            return CompositeClear;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ControlPart e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case NoControlPart:
            m_value.ident = CSSValueNone;
            break;
        case CheckboxPart:
            m_value.ident = CSSValueCheckbox;
            break;
        case RadioPart:
            m_value.ident = CSSValueRadio;
            break;
        case PushButtonPart:
            m_value.ident = CSSValuePushButton;
            break;
        case SquareButtonPart:
            m_value.ident = CSSValueSquareButton;
            break;
        case ButtonPart:
            m_value.ident = CSSValueButton;
            break;
        case ButtonBevelPart:
            m_value.ident = CSSValueButtonBevel;
            break;
        case DefaultButtonPart:
            m_value.ident = CSSValueDefaultButton;
            break;
        case InnerSpinButtonPart:
            m_value.ident = CSSValueInnerSpinButton;
            break;
        case ListboxPart:
            m_value.ident = CSSValueListbox;
            break;
        case ListButtonPart:
#if ENABLE(DATALIST)
            m_value.ident = CSSValueListButton;
#endif
            break;
        case ListItemPart:
            m_value.ident = CSSValueListitem;
            break;
        case MediaFullscreenButtonPart:
            m_value.ident = CSSValueMediaFullscreenButton;
            break;
        case MediaPlayButtonPart:
            m_value.ident = CSSValueMediaPlayButton;
            break;
        case MediaMuteButtonPart:
            m_value.ident = CSSValueMediaMuteButton;
            break;
        case MediaSeekBackButtonPart:
            m_value.ident = CSSValueMediaSeekBackButton;
            break;
        case MediaSeekForwardButtonPart:
            m_value.ident = CSSValueMediaSeekForwardButton;
            break;
        case MediaRewindButtonPart:
            m_value.ident = CSSValueMediaRewindButton;
            break;
        case MediaReturnToRealtimeButtonPart:
            m_value.ident = CSSValueMediaReturnToRealtimeButton;
            break;
        case MediaToggleClosedCaptionsButtonPart:
            m_value.ident = CSSValueMediaToggleClosedCaptionsButton;
            break;
        case MediaSliderPart:
            m_value.ident = CSSValueMediaSlider;
            break;
        case MediaSliderThumbPart:
            m_value.ident = CSSValueMediaSliderthumb;
            break;
        case MediaVolumeSliderContainerPart:
            m_value.ident = CSSValueMediaVolumeSliderContainer;
            break;
        case MediaVolumeSliderPart:
            m_value.ident = CSSValueMediaVolumeSlider;
            break;
        case MediaVolumeSliderMuteButtonPart:
            m_value.ident = CSSValueMediaVolumeSliderMuteButton;
            break;
        case MediaVolumeSliderThumbPart:
            m_value.ident = CSSValueMediaVolumeSliderthumb;
            break;
        case MediaControlsBackgroundPart:
            m_value.ident = CSSValueMediaControlsBackground;
            break;
        case MediaControlsFullscreenBackgroundPart:
            m_value.ident = CSSValueMediaControlsFullscreenBackground;
            break;
        case MediaCurrentTimePart:
            m_value.ident = CSSValueMediaCurrentTimeDisplay;
            break;
        case MediaTimeRemainingPart:
            m_value.ident = CSSValueMediaTimeRemainingDisplay;
            break;
        case MenulistPart:
            m_value.ident = CSSValueMenulist;
            break;
        case MenulistButtonPart:
            m_value.ident = CSSValueMenulistButton;
            break;
        case MenulistTextPart:
            m_value.ident = CSSValueMenulistText;
            break;
        case MenulistTextFieldPart:
            m_value.ident = CSSValueMenulistTextfield;
            break;
        case MeterPart:
            m_value.ident = CSSValueMeter;
            break;
        case RelevancyLevelIndicatorPart:
            m_value.ident = CSSValueRelevancyLevelIndicator;
            break;
        case ContinuousCapacityLevelIndicatorPart:
            m_value.ident = CSSValueContinuousCapacityLevelIndicator;
            break;
        case DiscreteCapacityLevelIndicatorPart:
            m_value.ident = CSSValueDiscreteCapacityLevelIndicator;
            break;
        case RatingLevelIndicatorPart:
            m_value.ident = CSSValueRatingLevelIndicator;
            break;
        case ProgressBarPart:
#if ENABLE(PROGRESS_TAG)
            m_value.ident = CSSValueProgressBar;
#endif
            break;
        case ProgressBarValuePart:
#if ENABLE(PROGRESS_TAG)
            m_value.ident = CSSValueProgressBarValue;
#endif
            break;
        case SliderHorizontalPart:
            m_value.ident = CSSValueSliderHorizontal;
            break;
        case SliderVerticalPart:
            m_value.ident = CSSValueSliderVertical;
            break;
        case SliderThumbHorizontalPart:
            m_value.ident = CSSValueSliderthumbHorizontal;
            break;
        case SliderThumbVerticalPart:
            m_value.ident = CSSValueSliderthumbVertical;
            break;
        case CaretPart:
            m_value.ident = CSSValueCaret;
            break;
        case SearchFieldPart:
            m_value.ident = CSSValueSearchfield;
            break;
        case SearchFieldDecorationPart:
            m_value.ident = CSSValueSearchfieldDecoration;
            break;
        case SearchFieldResultsDecorationPart:
            m_value.ident = CSSValueSearchfieldResultsDecoration;
            break;
        case SearchFieldResultsButtonPart:
            m_value.ident = CSSValueSearchfieldResultsButton;
            break;
        case SearchFieldCancelButtonPart:
            m_value.ident = CSSValueSearchfieldCancelButton;
            break;
        case TextFieldPart:
            m_value.ident = CSSValueTextfield;
            break;
        case TextAreaPart:
            m_value.ident = CSSValueTextarea;
            break;
        case CapsLockIndicatorPart:
            m_value.ident = CSSValueCapsLockIndicator;
            break;
        case InputSpeechButtonPart:
#if ENABLE(INPUT_SPEECH)
            m_value.ident = CSSValueWebkitInputSpeechButton;
#endif
            break;
    }
}

template<> inline CSSPrimitiveValue::operator ControlPart() const
{
    if (m_value.ident == CSSValueNone)
        return NoControlPart;
    else
        return ControlPart(m_value.ident - CSSValueCheckbox + 1);
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EBackfaceVisibility e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case BackfaceVisibilityVisible:
        m_value.ident = CSSValueVisible;
        break;
    case BackfaceVisibilityHidden:
        m_value.ident = CSSValueHidden;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EBackfaceVisibility() const
{
    switch (m_value.ident) {
    case CSSValueVisible:
        return BackfaceVisibilityVisible;
    case CSSValueHidden:
        return BackfaceVisibilityHidden;
    default:
        ASSERT_NOT_REACHED();
        return BackfaceVisibilityHidden;
    }
}


template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EFillAttachment e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case ScrollBackgroundAttachment:
            m_value.ident = CSSValueScroll;
            break;
        case LocalBackgroundAttachment:
            m_value.ident = CSSValueLocal;
            break;
        case FixedBackgroundAttachment:
            m_value.ident = CSSValueFixed;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EFillAttachment() const
{
    switch (m_value.ident) {
        case CSSValueScroll:
            return ScrollBackgroundAttachment;
        case CSSValueLocal:
            return LocalBackgroundAttachment;
        case CSSValueFixed:
            return FixedBackgroundAttachment;
        default:
            ASSERT_NOT_REACHED();
            return ScrollBackgroundAttachment;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EFillBox e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case BorderFillBox:
            m_value.ident = CSSValueBorderBox;
            break;
        case PaddingFillBox:
            m_value.ident = CSSValuePaddingBox;
            break;
        case ContentFillBox:
            m_value.ident = CSSValueContentBox;
            break;
        case TextFillBox:
            m_value.ident = CSSValueText;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EFillBox() const
{
    switch (m_value.ident) {
        case CSSValueBorder:
        case CSSValueBorderBox:
            return BorderFillBox;
        case CSSValuePadding:
        case CSSValuePaddingBox:
            return PaddingFillBox;
        case CSSValueContent:
        case CSSValueContentBox:
            return ContentFillBox;
        case CSSValueText:
        case CSSValueWebkitText:
            return TextFillBox;
        default:
            ASSERT_NOT_REACHED();
            return BorderFillBox;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EFillRepeat e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case RepeatFill:
            m_value.ident = CSSValueRepeat;
            break;
        case NoRepeatFill:
            m_value.ident = CSSValueNoRepeat;
            break;
        case RoundFill:
            m_value.ident = CSSValueRound;
            break;
        case SpaceFill:
            m_value.ident = CSSValueSpace;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EFillRepeat() const
{
    switch (m_value.ident) {
        case CSSValueRepeat:
            return RepeatFill;
        case CSSValueNoRepeat:
            return NoRepeatFill;
        case CSSValueRound:
            return RoundFill;
        case CSSValueSpace:
            return SpaceFill;
        default:
            ASSERT_NOT_REACHED();
            return RepeatFill;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EBoxAlignment e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case BSTRETCH:
            m_value.ident = CSSValueStretch;
            break;
        case BSTART:
            m_value.ident = CSSValueStart;
            break;
        case BCENTER:
            m_value.ident = CSSValueCenter;
            break;
        case BEND:
            m_value.ident = CSSValueEnd;
            break;
        case BBASELINE:
            m_value.ident = CSSValueBaseline;
            break;
        case BJUSTIFY:
            m_value.ident = CSSValueJustify;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EBoxAlignment() const
{
    switch (m_value.ident) {
        case CSSValueStretch:
            return BSTRETCH;
        case CSSValueStart:
            return BSTART;
        case CSSValueEnd:
            return BEND;
        case CSSValueCenter:
            return BCENTER;
        case CSSValueBaseline:
            return BBASELINE;
        case CSSValueJustify:
            return BJUSTIFY;
        default:
            ASSERT_NOT_REACHED();
            return BSTRETCH;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EBoxSizing e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case BORDER_BOX:
        m_value.ident = CSSValueBorderBox;
        break;
    case CONTENT_BOX:
        m_value.ident = CSSValueContentBox;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EBoxSizing() const
{
    switch (m_value.ident) {
    case CSSValueBorderBox:
        return BORDER_BOX;
    case CSSValueContentBox:
        return CONTENT_BOX;
    default:
        ASSERT_NOT_REACHED();
        return BORDER_BOX;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EBoxDirection e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case BNORMAL:
            m_value.ident = CSSValueNormal;
            break;
        case BREVERSE:
            m_value.ident = CSSValueReverse;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EBoxDirection() const
{
    switch (m_value.ident) {
        case CSSValueNormal:
            return BNORMAL;
        case CSSValueReverse:
            return BREVERSE;
        default:
            ASSERT_NOT_REACHED();
            return BNORMAL;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EBoxLines e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case SINGLE:
            m_value.ident = CSSValueSingle;
            break;
        case MULTIPLE:
            m_value.ident = CSSValueMultiple;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EBoxLines() const
{
    switch (m_value.ident) {
        case CSSValueSingle:
            return SINGLE;
        case CSSValueMultiple:
            return MULTIPLE;
        default:
            ASSERT_NOT_REACHED();
            return SINGLE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EBoxOrient e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case HORIZONTAL:
            m_value.ident = CSSValueHorizontal;
            break;
        case VERTICAL:
            m_value.ident = CSSValueVertical;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EBoxOrient() const
{
    switch (m_value.ident) {
        case CSSValueHorizontal:
        case CSSValueInlineAxis:
            return HORIZONTAL;
        case CSSValueVertical:
        case CSSValueBlockAxis:
            return VERTICAL;
        default:
            ASSERT_NOT_REACHED();
            return HORIZONTAL;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ECaptionSide e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case CAPLEFT:
            m_value.ident = CSSValueLeft;
            break;
        case CAPRIGHT:
            m_value.ident = CSSValueRight;
            break;
        case CAPTOP:
            m_value.ident = CSSValueTop;
            break;
        case CAPBOTTOM:
            m_value.ident = CSSValueBottom;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator ECaptionSide() const
{
    switch (m_value.ident) {
        case CSSValueLeft:
            return CAPLEFT;
        case CSSValueRight:
            return CAPRIGHT;
        case CSSValueTop:
            return CAPTOP;
        case CSSValueBottom:
            return CAPBOTTOM;
        default:
            ASSERT_NOT_REACHED();
            return CAPTOP;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EClear e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case CNONE:
            m_value.ident = CSSValueNone;
            break;
        case CLEFT:
            m_value.ident = CSSValueLeft;
            break;
        case CRIGHT:
            m_value.ident = CSSValueRight;
            break;
        case CBOTH:
            m_value.ident = CSSValueBoth;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EClear() const
{
    switch (m_value.ident) {
        case CSSValueNone:
            return CNONE;
        case CSSValueLeft:
            return CLEFT;
        case CSSValueRight:
            return CRIGHT;
        case CSSValueBoth:
            return CBOTH;
        default:
            ASSERT_NOT_REACHED();
            return CNONE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ECursor e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case CURSOR_AUTO:
            m_value.ident = CSSValueAuto;
            break;
        case CURSOR_CROSS:
            m_value.ident = CSSValueCrosshair;
            break;
        case CURSOR_DEFAULT:
            m_value.ident = CSSValueDefault;
            break;
        case CURSOR_POINTER:
            m_value.ident = CSSValuePointer;
            break;
        case CURSOR_MOVE:
            m_value.ident = CSSValueMove;
            break;
        case CURSOR_CELL:
            m_value.ident = CSSValueCell;
            break;
        case CURSOR_VERTICAL_TEXT:
            m_value.ident = CSSValueVerticalText;
            break;
        case CURSOR_CONTEXT_MENU:
            m_value.ident = CSSValueContextMenu;
            break;
        case CURSOR_ALIAS:
            m_value.ident = CSSValueAlias;
            break;
        case CURSOR_COPY:
            m_value.ident = CSSValueCopy;
            break;
        case CURSOR_NONE:
            m_value.ident = CSSValueNone;
            break;
        case CURSOR_PROGRESS:
            m_value.ident = CSSValueProgress;
            break;
        case CURSOR_NO_DROP:
            m_value.ident = CSSValueNoDrop;
            break;
        case CURSOR_NOT_ALLOWED:
            m_value.ident = CSSValueNotAllowed;
            break;
        case CURSOR_WEBKIT_ZOOM_IN:
            m_value.ident = CSSValueWebkitZoomIn;
            break;
        case CURSOR_WEBKIT_ZOOM_OUT:
            m_value.ident = CSSValueWebkitZoomOut;
            break;
        case CURSOR_E_RESIZE:
            m_value.ident = CSSValueEResize;
            break;
        case CURSOR_NE_RESIZE:
            m_value.ident = CSSValueNeResize;
            break;
        case CURSOR_NW_RESIZE:
            m_value.ident = CSSValueNwResize;
            break;
        case CURSOR_N_RESIZE:
            m_value.ident = CSSValueNResize;
            break;
        case CURSOR_SE_RESIZE:
            m_value.ident = CSSValueSeResize;
            break;
        case CURSOR_SW_RESIZE:
            m_value.ident = CSSValueSwResize;
            break;
        case CURSOR_S_RESIZE:
            m_value.ident = CSSValueSResize;
            break;
        case CURSOR_W_RESIZE:
            m_value.ident = CSSValueWResize;
            break;
        case CURSOR_EW_RESIZE:
            m_value.ident = CSSValueEwResize;
            break;
        case CURSOR_NS_RESIZE:
            m_value.ident = CSSValueNsResize;
            break;
        case CURSOR_NESW_RESIZE:
            m_value.ident = CSSValueNeswResize;
            break;
        case CURSOR_NWSE_RESIZE:
            m_value.ident = CSSValueNwseResize;
            break;
        case CURSOR_COL_RESIZE:
            m_value.ident = CSSValueColResize;
            break;
        case CURSOR_ROW_RESIZE:
            m_value.ident = CSSValueRowResize;
            break;
        case CURSOR_TEXT:
            m_value.ident = CSSValueText;
            break;
        case CURSOR_WAIT:
            m_value.ident = CSSValueWait;
            break;
        case CURSOR_HELP:
            m_value.ident = CSSValueHelp;
            break;
        case CURSOR_ALL_SCROLL:
            m_value.ident = CSSValueAllScroll;
            break;
        case CURSOR_WEBKIT_GRAB:
            m_value.ident = CSSValueWebkitGrab;
            break;
        case CURSOR_WEBKIT_GRABBING:
            m_value.ident = CSSValueWebkitGrabbing;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator ECursor() const
{
    if (m_value.ident == CSSValueCopy)
        return CURSOR_COPY;
    if (m_value.ident == CSSValueNone)
        return CURSOR_NONE;
    return static_cast<ECursor>(m_value.ident - CSSValueAuto);
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EDisplay e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case INLINE:
            m_value.ident = CSSValueInline;
            break;
        case BLOCK:
            m_value.ident = CSSValueBlock;
            break;
        case LIST_ITEM:
            m_value.ident = CSSValueListItem;
            break;
        case RUN_IN:
            m_value.ident = CSSValueRunIn;
            break;
        case COMPACT:
            m_value.ident = CSSValueCompact;
            break;
        case INLINE_BLOCK:
            m_value.ident = CSSValueInlineBlock;
            break;
        case TABLE:
            m_value.ident = CSSValueTable;
            break;
        case INLINE_TABLE:
            m_value.ident = CSSValueInlineTable;
            break;
        case TABLE_ROW_GROUP:
            m_value.ident = CSSValueTableRowGroup;
            break;
        case TABLE_HEADER_GROUP:
            m_value.ident = CSSValueTableHeaderGroup;
            break;
        case TABLE_FOOTER_GROUP:
            m_value.ident = CSSValueTableFooterGroup;
            break;
        case TABLE_ROW:
            m_value.ident = CSSValueTableRow;
            break;
        case TABLE_COLUMN_GROUP:
            m_value.ident = CSSValueTableColumnGroup;
            break;
        case TABLE_COLUMN:
            m_value.ident = CSSValueTableColumn;
            break;
        case TABLE_CELL:
            m_value.ident = CSSValueTableCell;
            break;
        case TABLE_CAPTION:
            m_value.ident = CSSValueTableCaption;
            break;
        case BOX:
            m_value.ident = CSSValueWebkitBox;
            break;
        case INLINE_BOX:
            m_value.ident = CSSValueWebkitInlineBox;
            break;
        case FLEXBOX:
            m_value.ident = CSSValueWebkitFlexbox;
            break;
        case INLINE_FLEXBOX:
            m_value.ident = CSSValueWebkitInlineFlexbox;
            break;
#if ENABLE(CSS_GRID_LAYOUT)
        case GRID:
            m_value.ident = CSSValueWebkitGrid;
            break;
        case INLINE_GRID:
            m_value.ident = CSSValueWebkitInlineGrid;
            break;
#endif
        case NONE:
            m_value.ident = CSSValueNone;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EDisplay() const
{
    if (m_value.ident == CSSValueNone)
        return NONE;

    EDisplay display = static_cast<EDisplay>(m_value.ident - CSSValueInline);
    ASSERT(display >= INLINE && display <= NONE);
    return display;
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EEmptyCell e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case SHOW:
            m_value.ident = CSSValueShow;
            break;
        case HIDE:
            m_value.ident = CSSValueHide;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EEmptyCell() const
{
    switch (m_value.ident) {
        case CSSValueShow:
            return SHOW;
        case CSSValueHide:
            return HIDE;
        default:
            ASSERT_NOT_REACHED();
            return SHOW;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EFlexAlign e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case AlignStart:
        m_value.ident = CSSValueStart;
        break;
    case AlignEnd:
        m_value.ident = CSSValueEnd;
        break;
    case AlignCenter:
        m_value.ident = CSSValueCenter;
        break;
    case AlignStretch:
        m_value.ident = CSSValueStretch;
        break;
    case AlignBaseline:
        m_value.ident = CSSValueBaseline;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EFlexAlign() const
{
    switch (m_value.ident) {
    case CSSValueStart:
        return AlignStart;
    case CSSValueEnd:
        return AlignEnd;
    case CSSValueCenter:
        return AlignCenter;
    case CSSValueStretch:
        return AlignStretch;
    case CSSValueBaseline:
        return AlignBaseline;
    default:
        ASSERT_NOT_REACHED();
        return AlignStart;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EFlexPack e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case PackStart:
        m_value.ident = CSSValueStart;
        break;
    case PackEnd:
        m_value.ident = CSSValueEnd;
        break;
    case PackCenter:
        m_value.ident = CSSValueCenter;
        break;
    case PackJustify:
        m_value.ident = CSSValueJustify;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EFlexPack() const
{
    switch (m_value.ident) {
    case CSSValueStart:
        return PackStart;
    case CSSValueEnd:
        return PackEnd;
    case CSSValueCenter:
        return PackCenter;
    case CSSValueJustify:
        return PackJustify;
    default:
        ASSERT_NOT_REACHED();
        return PackStart;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EFlexFlow e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case FlowRow:
        m_value.ident = CSSValueRow;
        break;
    case FlowRowReverse:
        m_value.ident = CSSValueRowReverse;
        break;
    case FlowColumn:
        m_value.ident = CSSValueColumn;
        break;
    case FlowColumnReverse:
        m_value.ident = CSSValueColumnReverse;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EFlexFlow() const
{
    switch (m_value.ident) {
    case CSSValueRow:
        return FlowRow;
    case CSSValueRowReverse:
        return FlowRowReverse;
    case CSSValueColumn:
        return FlowColumn;
    case CSSValueColumnReverse:
        return FlowColumnReverse;
    default:
        ASSERT_NOT_REACHED();
        return FlowRow;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EFloat e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case NoFloat:
            m_value.ident = CSSValueNone;
            break;
        case LeftFloat:
            m_value.ident = CSSValueLeft;
            break;
        case RightFloat:
            m_value.ident = CSSValueRight;
            break;
        case PositionedFloat:
            m_value.ident = CSSValueWebkitPositioned;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EFloat() const
{
    switch (m_value.ident) {
        case CSSValueLeft:
            return LeftFloat;
        case CSSValueRight:
            return RightFloat;
        case CSSValueNone:
        case CSSValueCenter:  // Non-standard CSS value
            return NoFloat;
        case CSSValueWebkitPositioned:
            return PositionedFloat;
        default:
            ASSERT_NOT_REACHED();
            return NoFloat;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EKHTMLLineBreak e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case LBNORMAL:
            m_value.ident = CSSValueNormal;
            break;
        case AFTER_WHITE_SPACE:
            m_value.ident = CSSValueAfterWhiteSpace;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EKHTMLLineBreak() const
{
    switch (m_value.ident) {
        case CSSValueAfterWhiteSpace:
            return AFTER_WHITE_SPACE;
        case CSSValueNormal:
            return LBNORMAL;
        default:
            ASSERT_NOT_REACHED();
            return LBNORMAL;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EListStylePosition e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case OUTSIDE:
            m_value.ident = CSSValueOutside;
            break;
        case INSIDE:
            m_value.ident = CSSValueInside;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EListStylePosition() const
{
    return (EListStylePosition)(m_value.ident - CSSValueOutside);
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EListStyleType e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case Afar:
        m_value.ident = CSSValueAfar;
        break;
    case Amharic:
        m_value.ident = CSSValueAmharic;
        break;
    case AmharicAbegede:
        m_value.ident = CSSValueAmharicAbegede;
        break;
    case ArabicIndic:
        m_value.ident = CSSValueArabicIndic;
        break;
    case Armenian:
        m_value.ident = CSSValueArmenian;
        break;
    case Asterisks:
        m_value.ident = CSSValueAsterisks;
        break;
    case BinaryListStyle:
        m_value.ident = CSSValueBinary;
        break;
    case Bengali:
        m_value.ident = CSSValueBengali;
        break;
    case Cambodian:
        m_value.ident = CSSValueCambodian;
        break;
    case Circle:
        m_value.ident = CSSValueCircle;
        break;
    case CjkEarthlyBranch:
        m_value.ident = CSSValueCjkEarthlyBranch;
        break;
    case CjkHeavenlyStem:
        m_value.ident = CSSValueCjkHeavenlyStem;
        break;
    case CJKIdeographic:
        m_value.ident = CSSValueCjkIdeographic;
        break;
    case DecimalLeadingZero:
        m_value.ident = CSSValueDecimalLeadingZero;
        break;
    case DecimalListStyle:
        m_value.ident = CSSValueDecimal;
        break;
    case Devanagari:
        m_value.ident = CSSValueDevanagari;
        break;
    case Disc:
        m_value.ident = CSSValueDisc;
        break;
    case Ethiopic:
        m_value.ident = CSSValueEthiopic;
        break;
    case EthiopicAbegede:
        m_value.ident = CSSValueEthiopicAbegede;
        break;
    case EthiopicAbegedeAmEt:
        m_value.ident = CSSValueEthiopicAbegedeAmEt;
        break;
    case EthiopicAbegedeGez:
        m_value.ident = CSSValueEthiopicAbegedeGez;
        break;
    case EthiopicAbegedeTiEr:
        m_value.ident = CSSValueEthiopicAbegedeTiEr;
        break;
    case EthiopicAbegedeTiEt:
        m_value.ident = CSSValueEthiopicAbegedeTiEt;
        break;
    case EthiopicHalehameAaEr:
        m_value.ident = CSSValueEthiopicHalehameAaEr;
        break;
    case EthiopicHalehameAaEt:
        m_value.ident = CSSValueEthiopicHalehameAaEt;
        break;
    case EthiopicHalehameAmEt:
        m_value.ident = CSSValueEthiopicHalehameAmEt;
        break;
    case EthiopicHalehameGez:
        m_value.ident = CSSValueEthiopicHalehameGez;
        break;
    case EthiopicHalehameOmEt:
        m_value.ident = CSSValueEthiopicHalehameOmEt;
        break;
    case EthiopicHalehameSidEt:
        m_value.ident = CSSValueEthiopicHalehameSidEt;
        break;
    case EthiopicHalehameSoEt:
        m_value.ident = CSSValueEthiopicHalehameSoEt;
        break;
    case EthiopicHalehameTiEr:
        m_value.ident = CSSValueEthiopicHalehameTiEr;
        break;
    case EthiopicHalehameTiEt:
        m_value.ident = CSSValueEthiopicHalehameTiEt;
        break;
    case EthiopicHalehameTig:
        m_value.ident = CSSValueEthiopicHalehameTig;
        break;
    case Footnotes:
        m_value.ident = CSSValueFootnotes;
        break;
    case Georgian:
        m_value.ident = CSSValueGeorgian;
        break;
    case Gujarati:
        m_value.ident = CSSValueGujarati;
        break;
    case Gurmukhi:
        m_value.ident = CSSValueGurmukhi;
        break;
    case Hangul:
        m_value.ident = CSSValueHangul;
        break;
    case HangulConsonant:
        m_value.ident = CSSValueHangulConsonant;
        break;
    case Hebrew:
        m_value.ident = CSSValueHebrew;
        break;
    case Hiragana:
        m_value.ident = CSSValueHiragana;
        break;
    case HiraganaIroha:
        m_value.ident = CSSValueHiraganaIroha;
        break;
    case Kannada:
        m_value.ident = CSSValueKannada;
        break;
    case Katakana:
        m_value.ident = CSSValueKatakana;
        break;
    case KatakanaIroha:
        m_value.ident = CSSValueKatakanaIroha;
        break;
    case Khmer:
        m_value.ident = CSSValueKhmer;
        break;
    case Lao:
        m_value.ident = CSSValueLao;
        break;
    case LowerAlpha:
        m_value.ident = CSSValueLowerAlpha;
        break;
    case LowerArmenian:
        m_value.ident = CSSValueLowerArmenian;
        break;
    case LowerGreek:
        m_value.ident = CSSValueLowerGreek;
        break;
    case LowerHexadecimal:
        m_value.ident = CSSValueLowerHexadecimal;
        break;
    case LowerLatin:
        m_value.ident = CSSValueLowerLatin;
        break;
    case LowerNorwegian:
        m_value.ident = CSSValueLowerNorwegian;
        break;
    case LowerRoman:
        m_value.ident = CSSValueLowerRoman;
        break;
    case Malayalam:
        m_value.ident = CSSValueMalayalam;
        break;
    case Mongolian:
        m_value.ident = CSSValueMongolian;
        break;
    case Myanmar:
        m_value.ident = CSSValueMyanmar;
        break;
    case NoneListStyle:
        m_value.ident = CSSValueNone;
        break;
    case Octal:
        m_value.ident = CSSValueOctal;
        break;
    case Oriya:
        m_value.ident = CSSValueOriya;
        break;
    case Oromo:
        m_value.ident = CSSValueOromo;
        break;
    case Persian:
        m_value.ident = CSSValuePersian;
        break;
    case Sidama:
        m_value.ident = CSSValueSidama;
        break;
    case Somali:
        m_value.ident = CSSValueSomali;
        break;
    case Square:
        m_value.ident = CSSValueSquare;
        break;
    case Telugu:
        m_value.ident = CSSValueTelugu;
        break;
    case Thai:
        m_value.ident = CSSValueThai;
        break;
    case Tibetan:
        m_value.ident = CSSValueTibetan;
        break;
    case Tigre:
        m_value.ident = CSSValueTigre;
        break;
    case TigrinyaEr:
        m_value.ident = CSSValueTigrinyaEr;
        break;
    case TigrinyaErAbegede:
        m_value.ident = CSSValueTigrinyaErAbegede;
        break;
    case TigrinyaEt:
        m_value.ident = CSSValueTigrinyaEt;
        break;
    case TigrinyaEtAbegede:
        m_value.ident = CSSValueTigrinyaEtAbegede;
        break;
    case UpperAlpha:
        m_value.ident = CSSValueUpperAlpha;
        break;
    case UpperArmenian:
        m_value.ident = CSSValueUpperArmenian;
        break;
    case UpperGreek:
        m_value.ident = CSSValueUpperGreek;
        break;
    case UpperHexadecimal:
        m_value.ident = CSSValueUpperHexadecimal;
        break;
    case UpperLatin:
        m_value.ident = CSSValueUpperLatin;
        break;
    case UpperNorwegian:
        m_value.ident = CSSValueUpperNorwegian;
        break;
    case UpperRoman:
        m_value.ident = CSSValueUpperRoman;
        break;
    case Urdu:
        m_value.ident = CSSValueUrdu;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EListStyleType() const
{
    switch (m_value.ident) {
        case CSSValueNone:
            return NoneListStyle;
        default:
            return static_cast<EListStyleType>(m_value.ident - CSSValueDisc);
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EMarginCollapse e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case MCOLLAPSE:
            m_value.ident = CSSValueCollapse;
            break;
        case MSEPARATE:
            m_value.ident = CSSValueSeparate;
            break;
        case MDISCARD:
            m_value.ident = CSSValueDiscard;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EMarginCollapse() const
{
    switch (m_value.ident) {
        case CSSValueCollapse:
            return MCOLLAPSE;
        case CSSValueSeparate:
            return MSEPARATE;
        case CSSValueDiscard:
            return MDISCARD;
        default:
            ASSERT_NOT_REACHED();
            return MCOLLAPSE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EMarqueeBehavior e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case MNONE:
            m_value.ident = CSSValueNone;
            break;
        case MSCROLL:
            m_value.ident = CSSValueScroll;
            break;
        case MSLIDE:
            m_value.ident = CSSValueSlide;
            break;
        case MALTERNATE:
            m_value.ident = CSSValueAlternate;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EMarqueeBehavior() const
{
    switch (m_value.ident) {
        case CSSValueNone:
            return MNONE;
        case CSSValueScroll:
            return MSCROLL;
        case CSSValueSlide:
            return MSLIDE;
        case CSSValueAlternate:
            return MALTERNATE;
        default:
            ASSERT_NOT_REACHED();
            return MNONE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(RegionOverflow e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case AutoRegionOverflow:
        m_value.ident = CSSValueAuto;
        break;
    case BreakRegionOverflow:
        m_value.ident = CSSValueBreak;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator RegionOverflow() const
{
    switch (m_value.ident) {
    case CSSValueAuto:
        return AutoRegionOverflow;
    case CSSValueBreak:
        return BreakRegionOverflow;
    default:
        ASSERT_NOT_REACHED();
        return AutoRegionOverflow;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EMarqueeDirection e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case MFORWARD:
            m_value.ident = CSSValueForwards;
            break;
        case MBACKWARD:
            m_value.ident = CSSValueBackwards;
            break;
        case MAUTO:
            m_value.ident = CSSValueAuto;
            break;
        case MUP:
            m_value.ident = CSSValueUp;
            break;
        case MDOWN:
            m_value.ident = CSSValueDown;
            break;
        case MLEFT:
            m_value.ident = CSSValueLeft;
            break;
        case MRIGHT:
            m_value.ident = CSSValueRight;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EMarqueeDirection() const
{
    switch (m_value.ident) {
        case CSSValueForwards:
            return MFORWARD;
        case CSSValueBackwards:
            return MBACKWARD;
        case CSSValueAuto:
            return MAUTO;
        case CSSValueAhead:
        case CSSValueUp: // We don't support vertical languages, so AHEAD just maps to UP.
            return MUP;
        case CSSValueReverse:
        case CSSValueDown: // REVERSE just maps to DOWN, since we don't do vertical text.
            return MDOWN;
        case CSSValueLeft:
            return MLEFT;
        case CSSValueRight:
            return MRIGHT;
        default:
            ASSERT_NOT_REACHED();
            return MAUTO;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EMatchNearestMailBlockquoteColor e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case BCNORMAL:
            m_value.ident = CSSValueNormal;
            break;
        case MATCH:
            m_value.ident = CSSValueMatch;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EMatchNearestMailBlockquoteColor() const
{
    switch (m_value.ident) {
        case CSSValueNormal:
            return BCNORMAL;
        case CSSValueMatch:
            return MATCH;
        default:
            ASSERT_NOT_REACHED();
            return BCNORMAL;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ENBSPMode e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case NBNORMAL:
            m_value.ident = CSSValueNormal;
            break;
        case SPACE:
            m_value.ident = CSSValueSpace;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator ENBSPMode() const
{
    switch (m_value.ident) {
        case CSSValueSpace:
            return SPACE;
        case CSSValueNormal:
            return NBNORMAL;
        default:
            ASSERT_NOT_REACHED();
            return NBNORMAL;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EOverflow e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case OVISIBLE:
            m_value.ident = CSSValueVisible;
            break;
        case OHIDDEN:
            m_value.ident = CSSValueHidden;
            break;
        case OSCROLL:
            m_value.ident = CSSValueScroll;
            break;
        case OAUTO:
            m_value.ident = CSSValueAuto;
            break;
        case OMARQUEE:
            m_value.ident = CSSValueWebkitMarquee;
            break;
        case OOVERLAY:
            m_value.ident = CSSValueOverlay;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EOverflow() const
{
    switch (m_value.ident) {
        case CSSValueVisible:
            return OVISIBLE;
        case CSSValueHidden:
            return OHIDDEN;
        case CSSValueScroll:
            return OSCROLL;
        case CSSValueAuto:
            return OAUTO;
        case CSSValueWebkitMarquee:
            return OMARQUEE;
        case CSSValueOverlay:
            return OOVERLAY;
        default:
            ASSERT_NOT_REACHED();
            return OVISIBLE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EPageBreak e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case PBAUTO:
            m_value.ident = CSSValueAuto;
            break;
        case PBALWAYS:
            m_value.ident = CSSValueAlways;
            break;
        case PBAVOID:
            m_value.ident = CSSValueAvoid;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EPageBreak() const
{
    switch (m_value.ident) {
        case CSSValueAuto:
            return PBAUTO;
        case CSSValueLeft:
        case CSSValueRight:
        case CSSValueAlways:
            return PBALWAYS; // CSS2.1: "Conforming user agents may map left/right to always."
        case CSSValueAvoid:
            return PBAVOID;
        default:
            ASSERT_NOT_REACHED();
            return PBAUTO;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EPosition e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case StaticPosition:
            m_value.ident = CSSValueStatic;
            break;
        case RelativePosition:
            m_value.ident = CSSValueRelative;
            break;
        case AbsolutePosition:
            m_value.ident = CSSValueAbsolute;
            break;
        case FixedPosition:
            m_value.ident = CSSValueFixed;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EPosition() const
{
    switch (m_value.ident) {
        case CSSValueStatic:
            return StaticPosition;
        case CSSValueRelative:
            return RelativePosition;
        case CSSValueAbsolute:
            return AbsolutePosition;
        case CSSValueFixed:
            return FixedPosition;
        default:
            ASSERT_NOT_REACHED();
            return StaticPosition;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EResize e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case RESIZE_BOTH:
            m_value.ident = CSSValueBoth;
            break;
        case RESIZE_HORIZONTAL:
            m_value.ident = CSSValueHorizontal;
            break;
        case RESIZE_VERTICAL:
            m_value.ident = CSSValueVertical;
            break;
        case RESIZE_NONE:
            m_value.ident = CSSValueNone;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EResize() const
{
    switch (m_value.ident) {
        case CSSValueBoth:
            return RESIZE_BOTH;
        case CSSValueHorizontal:
            return RESIZE_HORIZONTAL;
        case CSSValueVertical:
            return RESIZE_VERTICAL;
        case CSSValueAuto:
            ASSERT_NOT_REACHED(); // Depends on settings, thus should be handled by the caller.
            return RESIZE_NONE;
        case CSSValueNone:
            return RESIZE_NONE;
        default:
            ASSERT_NOT_REACHED();
            return RESIZE_NONE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ETableLayout e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case TAUTO:
            m_value.ident = CSSValueAuto;
            break;
        case TFIXED:
            m_value.ident = CSSValueFixed;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator ETableLayout() const
{
    switch (m_value.ident) {
        case CSSValueFixed:
            return TFIXED;
        case CSSValueAuto:
            return TAUTO;
        default:
            ASSERT_NOT_REACHED();
            return TAUTO;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ETextAlign e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case TAAUTO:
        m_value.ident = CSSValueWebkitAuto;
        break;
    case TASTART:
        m_value.ident = CSSValueStart;
        break;
    case TAEND:
        m_value.ident = CSSValueEnd;
        break;
    case LEFT:
        m_value.ident = CSSValueLeft;
        break;
    case RIGHT:
        m_value.ident = CSSValueRight;
        break;
    case CENTER:
        m_value.ident = CSSValueCenter;
        break;
    case JUSTIFY:
        m_value.ident = CSSValueJustify;
        break;
    case WEBKIT_LEFT:
        m_value.ident = CSSValueWebkitLeft;
        break;
    case WEBKIT_RIGHT:
        m_value.ident = CSSValueWebkitRight;
        break;
    case WEBKIT_CENTER:
        m_value.ident = CSSValueWebkitCenter;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator ETextAlign() const
{
    switch (m_value.ident) {
        case CSSValueStart:
            return TASTART;
        case CSSValueEnd:
            return TAEND;
        default:
            return static_cast<ETextAlign>(m_value.ident - CSSValueWebkitAuto);
    }
}

template<> inline CSSPrimitiveValue::operator ETextDecoration() const
{
    switch (m_value.ident) {
    case CSSValueNone:
        return TDNONE;
    case CSSValueUnderline:
        return UNDERLINE;
    case CSSValueOverline:
        return OVERLINE;
    case CSSValueLineThrough:
        return LINE_THROUGH;
    case CSSValueBlink:
        return BLINK;
    default:
        ASSERT_NOT_REACHED();
        return TDNONE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ETextSecurity e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case TSNONE:
            m_value.ident = CSSValueNone;
            break;
        case TSDISC:
            m_value.ident = CSSValueDisc;
            break;
        case TSCIRCLE:
            m_value.ident = CSSValueCircle;
            break;
        case TSSQUARE:
            m_value.ident = CSSValueSquare;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator ETextSecurity() const
{
    switch (m_value.ident) {
        case CSSValueNone:
            return TSNONE;
        case CSSValueDisc:
            return TSDISC;
        case CSSValueCircle:
            return TSCIRCLE;
        case CSSValueSquare:
            return TSSQUARE;
        default:
            ASSERT_NOT_REACHED();
            return TSNONE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ETextTransform e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case CAPITALIZE:
            m_value.ident = CSSValueCapitalize;
            break;
        case UPPERCASE:
            m_value.ident = CSSValueUppercase;
            break;
        case LOWERCASE:
            m_value.ident = CSSValueLowercase;
            break;
        case TTNONE:
            m_value.ident = CSSValueNone;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator ETextTransform() const
{
    switch (m_value.ident) {
        case CSSValueCapitalize:
            return CAPITALIZE;
        case CSSValueUppercase:
            return UPPERCASE;
        case CSSValueLowercase:
            return LOWERCASE;
        case CSSValueNone:
            return TTNONE;
        default:
            ASSERT_NOT_REACHED();
            return TTNONE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EUnicodeBidi e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case UBNormal:
        m_value.ident = CSSValueNormal;
        break;
    case Embed:
        m_value.ident = CSSValueEmbed;
        break;
    case Override:
        m_value.ident = CSSValueBidiOverride;
        break;
    case Isolate:
        m_value.ident = CSSValueWebkitIsolate;
        break;
    case Plaintext:
        m_value.ident = CSSValueWebkitPlaintext;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EUnicodeBidi() const
{
    switch (m_value.ident) {
    case CSSValueNormal:
        return UBNormal;
    case CSSValueEmbed:
        return Embed;
    case CSSValueBidiOverride:
        return Override;
    case CSSValueWebkitIsolate:
        return Isolate;
    case CSSValueWebkitPlaintext:
        return Plaintext;
    default:
        ASSERT_NOT_REACHED();
        return UBNormal;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EUserDrag e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case DRAG_AUTO:
            m_value.ident = CSSValueAuto;
            break;
        case DRAG_NONE:
            m_value.ident = CSSValueNone;
            break;
        case DRAG_ELEMENT:
            m_value.ident = CSSValueElement;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EUserDrag() const
{
    switch (m_value.ident) {
        case CSSValueAuto:
            return DRAG_AUTO;
        case CSSValueNone:
            return DRAG_NONE;
        case CSSValueElement:
            return DRAG_ELEMENT;
        default:
            ASSERT_NOT_REACHED();
            return DRAG_AUTO;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EUserModify e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case READ_ONLY:
            m_value.ident = CSSValueReadOnly;
            break;
        case READ_WRITE:
            m_value.ident = CSSValueReadWrite;
            break;
        case READ_WRITE_PLAINTEXT_ONLY:
            m_value.ident = CSSValueReadWritePlaintextOnly;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EUserModify() const
{
    return static_cast<EUserModify>(m_value.ident - CSSValueReadOnly);
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EUserSelect e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case SELECT_NONE:
            m_value.ident = CSSValueNone;
            break;
        case SELECT_TEXT:
            m_value.ident = CSSValueText;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EUserSelect() const
{
    switch (m_value.ident) {
        case CSSValueAuto:
            return SELECT_TEXT;
        case CSSValueNone:
            return SELECT_NONE;
        case CSSValueText:
            return SELECT_TEXT;
        default:
            ASSERT_NOT_REACHED();
            return SELECT_TEXT;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EVerticalAlign a)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (a) {
    case TOP:
        m_value.ident = CSSValueTop;
        break;
    case BOTTOM:
        m_value.ident = CSSValueBottom;
        break;
    case MIDDLE:
        m_value.ident = CSSValueMiddle;
        break;
    case BASELINE:
        m_value.ident = CSSValueBaseline;
        break;
    case TEXT_BOTTOM:
        m_value.ident = CSSValueTextBottom;
        break;
    case TEXT_TOP:
        m_value.ident = CSSValueTextTop;
        break;
    case SUB:
        m_value.ident = CSSValueSub;
        break;
    case SUPER:
        m_value.ident = CSSValueSuper;
        break;
    case BASELINE_MIDDLE:
        m_value.ident = CSSValueWebkitBaselineMiddle;
        break;
    case LENGTH:
        m_value.ident = CSSValueInvalid;
    }
}

template<> inline CSSPrimitiveValue::operator EVerticalAlign() const
{
    switch (m_value.ident) {
    case CSSValueTop:
        return TOP;
    case CSSValueBottom:
        return BOTTOM;
    case CSSValueMiddle:
        return MIDDLE;
    case CSSValueBaseline:
        return BASELINE;
    case CSSValueTextBottom:
        return TEXT_BOTTOM;
    case CSSValueTextTop:
        return TEXT_TOP;
    case CSSValueSub:
        return SUB;
    case CSSValueSuper:
        return SUPER;
    case CSSValueWebkitBaselineMiddle:
        return BASELINE_MIDDLE;
    default:
        ASSERT_NOT_REACHED();
        return TOP;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EVisibility e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case VISIBLE:
            m_value.ident = CSSValueVisible;
            break;
        case HIDDEN:
            m_value.ident = CSSValueHidden;
            break;
        case COLLAPSE:
            m_value.ident = CSSValueCollapse;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EVisibility() const
{
    switch (m_value.ident) {
        case CSSValueHidden:
            return HIDDEN;
        case CSSValueVisible:
            return VISIBLE;
        case CSSValueCollapse:
            return COLLAPSE;
        default:
            ASSERT_NOT_REACHED();
            return VISIBLE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EWhiteSpace e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case NORMAL:
            m_value.ident = CSSValueNormal;
            break;
        case PRE:
            m_value.ident = CSSValuePre;
            break;
        case PRE_WRAP:
            m_value.ident = CSSValuePreWrap;
            break;
        case PRE_LINE:
            m_value.ident = CSSValuePreLine;
            break;
        case NOWRAP:
            m_value.ident = CSSValueNowrap;
            break;
        case KHTML_NOWRAP:
            m_value.ident = CSSValueWebkitNowrap;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EWhiteSpace() const
{
    switch (m_value.ident) {
        case CSSValueWebkitNowrap:
            return KHTML_NOWRAP;
        case CSSValueNowrap:
            return NOWRAP;
        case CSSValuePre:
            return PRE;
        case CSSValuePreWrap:
            return PRE_WRAP;
        case CSSValuePreLine:
            return PRE_LINE;
        case CSSValueNormal:
            return NORMAL;
        default:
            ASSERT_NOT_REACHED();
            return NORMAL;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EWordBreak e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case NormalWordBreak:
            m_value.ident = CSSValueNormal;
            break;
        case BreakAllWordBreak:
            m_value.ident = CSSValueBreakAll;
            break;
        case BreakWordBreak:
            m_value.ident = CSSValueBreakWord;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EWordBreak() const
{
    switch (m_value.ident) {
        case CSSValueBreakAll:
            return BreakAllWordBreak;
        case CSSValueBreakWord:
            return BreakWordBreak;
        case CSSValueNormal:
            return NormalWordBreak;
        default:
        ASSERT_NOT_REACHED();
        return NormalWordBreak;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EWordWrap e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case NormalWordWrap:
            m_value.ident = CSSValueNormal;
            break;
        case BreakWordWrap:
            m_value.ident = CSSValueBreakWord;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EWordWrap() const
{
    switch (m_value.ident) {
        case CSSValueBreakWord:
            return BreakWordWrap;
        case CSSValueNormal:
            return NormalWordWrap;
        default:
            ASSERT_NOT_REACHED();
            return NormalWordWrap;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(TextDirection e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case LTR:
            m_value.ident = CSSValueLtr;
            break;
        case RTL:
            m_value.ident = CSSValueRtl;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator TextDirection() const
{
    switch (m_value.ident) {
        case CSSValueLtr:
            return LTR;
        case CSSValueRtl:
            return RTL;
        default:
            ASSERT_NOT_REACHED();
            return LTR;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(WritingMode e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case TopToBottomWritingMode:
        m_value.ident = CSSValueHorizontalTb;
        break;
    case RightToLeftWritingMode:
        m_value.ident = CSSValueVerticalRl;
        break;
    case LeftToRightWritingMode:
        m_value.ident = CSSValueVerticalLr;
        break;
    case BottomToTopWritingMode:
        m_value.ident = CSSValueHorizontalBt;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator WritingMode() const
{
    switch (m_value.ident) {
    case CSSValueHorizontalTb:
        return TopToBottomWritingMode;
    case CSSValueVerticalRl:
        return RightToLeftWritingMode;
    case CSSValueVerticalLr:
        return LeftToRightWritingMode;
    case CSSValueHorizontalBt:
        return BottomToTopWritingMode;
    default:
        ASSERT_NOT_REACHED();
        return TopToBottomWritingMode;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(TextCombine e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case TextCombineNone:
        m_value.ident = CSSValueNone;
        break;
    case TextCombineHorizontal:
        m_value.ident = CSSValueHorizontal;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator TextCombine() const
{
    switch (m_value.ident) {
    case CSSValueNone:
        return TextCombineNone;
    case CSSValueHorizontal:
        return TextCombineHorizontal;
    default:
        ASSERT_NOT_REACHED();
        return TextCombineNone;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(TextEmphasisPosition position)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (position) {
    case TextEmphasisPositionOver:
        m_value.ident = CSSValueOver;
        break;
    case TextEmphasisPositionUnder:
        m_value.ident = CSSValueUnder;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator TextEmphasisPosition() const
{
    switch (m_value.ident) {
    case CSSValueOver:
        return TextEmphasisPositionOver;
    case CSSValueUnder:
        return TextEmphasisPositionUnder;
    default:
        ASSERT_NOT_REACHED();
        return TextEmphasisPositionOver;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(TextOverflow overflow)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (overflow) {
    case TextOverflowClip:
        m_value.ident = CSSValueClip;
        break;
    case TextOverflowEllipsis:
        m_value.ident = CSSValueEllipsis;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator TextOverflow() const
{
    switch (m_value.ident) {
    case CSSValueClip:
        return TextOverflowClip;
    case CSSValueEllipsis:
        return TextOverflowEllipsis;
    default:
        ASSERT_NOT_REACHED();
        return TextOverflowClip;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(TextEmphasisFill fill)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (fill) {
    case TextEmphasisFillFilled:
        m_value.ident = CSSValueFilled;
        break;
    case TextEmphasisFillOpen:
        m_value.ident = CSSValueOpen;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator TextEmphasisFill() const
{
    switch (m_value.ident) {
    case CSSValueFilled:
        return TextEmphasisFillFilled;
    case CSSValueOpen:
        return TextEmphasisFillOpen;
    default:
        ASSERT_NOT_REACHED();
        return TextEmphasisFillFilled;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(TextEmphasisMark mark)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (mark) {
    case TextEmphasisMarkDot:
        m_value.ident = CSSValueDot;
        break;
    case TextEmphasisMarkCircle:
        m_value.ident = CSSValueCircle;
        break;
    case TextEmphasisMarkDoubleCircle:
        m_value.ident = CSSValueDoubleCircle;
        break;
    case TextEmphasisMarkTriangle:
        m_value.ident = CSSValueTriangle;
        break;
    case TextEmphasisMarkSesame:
        m_value.ident = CSSValueSesame;
        break;
    case TextEmphasisMarkNone:
    case TextEmphasisMarkAuto:
    case TextEmphasisMarkCustom:
        ASSERT_NOT_REACHED();
        m_value.ident = CSSValueNone;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator TextEmphasisMark() const
{
    switch (m_value.ident) {
    case CSSValueNone:
        return TextEmphasisMarkNone;
    case CSSValueDot:
        return TextEmphasisMarkDot;
    case CSSValueCircle:
        return TextEmphasisMarkCircle;
    case CSSValueDoubleCircle:
        return TextEmphasisMarkDoubleCircle;
    case CSSValueTriangle:
        return TextEmphasisMarkTriangle;
    case CSSValueSesame:
        return TextEmphasisMarkSesame;
    default:
        ASSERT_NOT_REACHED();
        return TextEmphasisMarkNone;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(TextOrientation e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case TextOrientationVerticalRight:
        m_value.ident = CSSValueVerticalRight;
        break;
    case TextOrientationUpright:
        m_value.ident = CSSValueUpright;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator TextOrientation() const
{
    switch (m_value.ident) {
    case CSSValueVerticalRight:
        return TextOrientationVerticalRight;
    case CSSValueUpright:
        return TextOrientationUpright;
    default:
        ASSERT_NOT_REACHED();
        return TextOrientationVerticalRight;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EPointerEvents e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case PE_NONE:
            m_value.ident = CSSValueNone;
            break;
        case PE_STROKE:
            m_value.ident = CSSValueStroke;
            break;
        case PE_FILL:
            m_value.ident = CSSValueFill;
            break;
        case PE_PAINTED:
            m_value.ident = CSSValuePainted;
            break;
        case PE_VISIBLE:
            m_value.ident = CSSValueVisible;
            break;
        case PE_VISIBLE_STROKE:
            m_value.ident = CSSValueVisiblestroke;
            break;
        case PE_VISIBLE_FILL:
            m_value.ident = CSSValueVisiblefill;
            break;
        case PE_VISIBLE_PAINTED:
            m_value.ident = CSSValueVisiblepainted;
            break;
        case PE_AUTO:
            m_value.ident = CSSValueAuto;
            break;
        case PE_ALL:
            m_value.ident = CSSValueAll;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EPointerEvents() const
{
    switch (m_value.ident) {
        case CSSValueAll:
            return PE_ALL;
        case CSSValueAuto:
            return PE_AUTO;
        case CSSValueNone:
            return PE_NONE;
        case CSSValueVisiblepainted:
            return PE_VISIBLE_PAINTED;
        case CSSValueVisiblefill:
            return PE_VISIBLE_FILL;
        case CSSValueVisiblestroke:
            return PE_VISIBLE_STROKE;
        case CSSValueVisible:
            return PE_VISIBLE;
        case CSSValuePainted:
            return PE_PAINTED;
        case CSSValueFill:
            return PE_FILL;
        case CSSValueStroke:
            return PE_STROKE;
        default:
            ASSERT_NOT_REACHED();
            return PE_ALL;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(FontSmoothingMode smoothing)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (smoothing) {
    case AutoSmoothing:
        m_value.ident = CSSValueAuto;
        return;
    case NoSmoothing:
        m_value.ident = CSSValueNone;
        return;
    case Antialiased:
        m_value.ident = CSSValueAntialiased;
        return;
    case SubpixelAntialiased:
        m_value.ident = CSSValueSubpixelAntialiased;
        return;
    }

    ASSERT_NOT_REACHED();
    m_value.ident = CSSValueAuto;
}

template<> inline CSSPrimitiveValue::operator FontSmoothingMode() const
{
    switch (m_value.ident) {
    case CSSValueAuto:
        return AutoSmoothing;
    case CSSValueNone:
        return NoSmoothing;
    case CSSValueAntialiased:
        return Antialiased;
    case CSSValueSubpixelAntialiased:
        return SubpixelAntialiased;
    }

    ASSERT_NOT_REACHED();
    return AutoSmoothing;
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(FontWeight weight)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (weight) {
    case FontWeight900:
        m_value.ident = CSSValue900;
        return;
    case FontWeight800:
        m_value.ident = CSSValue800;
        return;
    case FontWeight700:
        m_value.ident = CSSValue700;
        return;
    case FontWeight600:
        m_value.ident = CSSValue600;
        return;
    case FontWeight500:
        m_value.ident = CSSValue500;
        return;
    case FontWeight400:
        m_value.ident = CSSValue400;
        return;
    case FontWeight300:
        m_value.ident = CSSValue300;
        return;
    case FontWeight200:
        m_value.ident = CSSValue200;
        return;
    case FontWeight100:
        m_value.ident = CSSValue100;
        return;
    }

    ASSERT_NOT_REACHED();
    m_value.ident = CSSValueNormal;
}

template<> inline CSSPrimitiveValue::operator FontWeight() const
{
    switch (m_value.ident) {
    case CSSValueBold:
        return FontWeightBold;
    case CSSValueNormal:
        return FontWeightNormal;
    case CSSValue900:
        return FontWeight900;
    case CSSValue800:
        return FontWeight800;
    case CSSValue700:
        return FontWeight700;
    case CSSValue600:
        return FontWeight600;
    case CSSValue500:
        return FontWeight500;
    case CSSValue400:
        return FontWeight400;
    case CSSValue300:
        return FontWeight300;
    case CSSValue200:
        return FontWeight200;
    case CSSValue100:
        return FontWeight100;
    }

    ASSERT_NOT_REACHED();
    return FontWeightNormal;
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(FontItalic italic)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (italic) {
    case FontItalicOff:
        m_value.ident = CSSValueNormal;
        return;
    case FontItalicOn:
        m_value.ident = CSSValueItalic;
        return;
    }

    ASSERT_NOT_REACHED();
    m_value.ident = CSSValueNormal;
}

template<> inline CSSPrimitiveValue::operator FontItalic() const
{
    switch (m_value.ident) {
    case CSSValueOblique:
    // FIXME: oblique is the same as italic for the moment...
    case CSSValueItalic:
        return FontItalicOn;
    case CSSValueNormal:
        return FontItalicOff;
    }
    ASSERT_NOT_REACHED();
    return FontItalicOff;
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(FontSmallCaps smallCaps)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (smallCaps) {
    case FontSmallCapsOff:
        m_value.ident = CSSValueNormal;
        return;
    case FontSmallCapsOn:
        m_value.ident = CSSValueSmallCaps;
        return;
    }

    ASSERT_NOT_REACHED();
    m_value.ident = CSSValueNormal;
}

template<> inline CSSPrimitiveValue::operator FontSmallCaps() const
{
    switch (m_value.ident) {
    case CSSValueSmallCaps:
        return FontSmallCapsOn;
    case CSSValueNormal:
        return FontSmallCapsOff;
    }
    ASSERT_NOT_REACHED();
    return FontSmallCapsOff;
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(TextRenderingMode e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case AutoTextRendering:
            m_value.ident = CSSValueAuto;
            break;
        case OptimizeSpeed:
            m_value.ident = CSSValueOptimizespeed;
            break;
        case OptimizeLegibility:
            m_value.ident = CSSValueOptimizelegibility;
            break;
        case GeometricPrecision:
            m_value.ident = CSSValueGeometricprecision;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator TextRenderingMode() const
{
    switch (m_value.ident) {
        case CSSValueAuto:
            return AutoTextRendering;
        case CSSValueOptimizespeed:
            return OptimizeSpeed;
        case CSSValueOptimizelegibility:
            return OptimizeLegibility;
        case CSSValueGeometricprecision:
            return GeometricPrecision;
        default:
            ASSERT_NOT_REACHED();
            return AutoTextRendering;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ColorSpace space)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (space) {
    case ColorSpaceDeviceRGB:
        m_value.ident = CSSValueDefault;
        break;
    case ColorSpaceSRGB:
        m_value.ident = CSSValueSrgb;
        break;
    case ColorSpaceLinearRGB:
        // CSS color correction does not support linearRGB yet.
        ASSERT_NOT_REACHED();
        m_value.ident = CSSValueDefault;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator ColorSpace() const
{
    switch (m_value.ident) {
    case CSSValueDefault:
        return ColorSpaceDeviceRGB;
    case CSSValueSrgb:
        return ColorSpaceSRGB;
    default:
        ASSERT_NOT_REACHED();
        return ColorSpaceDeviceRGB;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(Hyphens hyphens)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (hyphens) {
    case HyphensNone:
        m_value.ident = CSSValueNone;
        break;
    case HyphensManual:
        m_value.ident = CSSValueManual;
        break;
    case HyphensAuto:
        m_value.ident = CSSValueAuto;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator Hyphens() const
{
    switch (m_value.ident) {
    case CSSValueNone:
        return HyphensNone;
    case CSSValueManual:
        return HyphensManual;
    case CSSValueAuto:
        return HyphensAuto;
    default:
        ASSERT_NOT_REACHED();
        return HyphensAuto;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(LineGridSnap gridSnap)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (gridSnap) {
    case LineGridSnapNone:
        m_value.ident = CSSValueNone;
        break;
    case LineGridSnapBaseline:
        m_value.ident = CSSValueBaseline;
        break;
    case LineGridSnapBounds:
        m_value.ident = CSSValueBounds;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator LineGridSnap() const
{
    switch (m_value.ident) {
    case CSSValueNone:
        return LineGridSnapNone;
    case CSSValueBaseline:
        return LineGridSnapBaseline;
    case CSSValueBounds:
        return LineGridSnapBounds;
    default:
        ASSERT_NOT_REACHED();
        return LineGridSnapNone;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ESpeak e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case SpeakNone:
        m_value.ident = CSSValueNone;
        break;
    case SpeakNormal:
        m_value.ident = CSSValueNormal;
        break;
    case SpeakSpellOut:
        m_value.ident = CSSValueSpellOut;
        break;
    case SpeakDigits:
        m_value.ident = CSSValueDigits;
        break;
    case SpeakLiteralPunctuation:
        m_value.ident = CSSValueLiteralPunctuation;
        break;
    case SpeakNoPunctuation:
        m_value.ident = CSSValueNoPunctuation;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator Order() const
{
    switch (m_value.ident) {
    case CSSValueLogical:
        return LogicalOrder;
    case CSSValueVisual:
        return VisualOrder;
    default:
        ASSERT_NOT_REACHED();
        return LogicalOrder;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(Order e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case LogicalOrder:
        m_value.ident = CSSValueLogical;
        break;
    case VisualOrder:
        m_value.ident = CSSValueVisual;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator ESpeak() const
{
    switch (m_value.ident) {
    case CSSValueNone:
        return SpeakNone;
    case CSSValueNormal:
        return SpeakNormal;
    case CSSValueSpellOut:
        return SpeakSpellOut;
    case CSSValueDigits:
        return SpeakDigits;
    case CSSValueLiteralPunctuation:
        return SpeakLiteralPunctuation;
    case CSSValueNoPunctuation:
        return SpeakNoPunctuation;
    default:
        ASSERT_NOT_REACHED();
        return SpeakNormal;
    }
}

#if ENABLE(CSS_SHADERS)
template<> inline CSSPrimitiveValue::CSSPrimitiveValue(CustomFilterOperation::MeshBoxType meshBoxType)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (meshBoxType) {
    case CustomFilterOperation::FILTER_BOX:
        m_value.ident = CSSValueFilterBox;
        break;
    case CustomFilterOperation::BORDER_BOX:
        m_value.ident = CSSValueBorderBox;
        break;
    case CustomFilterOperation::PADDING_BOX:
        m_value.ident = CSSValuePaddingBox;
        break;
    case CustomFilterOperation::CONTENT_BOX:
        m_value.ident = CSSValueContentBox;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator CustomFilterOperation::MeshBoxType() const
{
    switch (m_value.ident) {
    case CSSValueFilterBox:
        return CustomFilterOperation::FILTER_BOX;
    case CSSValueBorderBox:
        return CustomFilterOperation::BORDER_BOX;
    case CSSValuePaddingBox:
        return CustomFilterOperation::PADDING_BOX;
    case CSSValueContentBox:
        return CustomFilterOperation::CONTENT_BOX;
    default:
        ASSERT_NOT_REACHED();
        return CustomFilterOperation::FILTER_BOX;
    }
}
#endif // ENABLE(CSS_SHADERS)

#if ENABLE(SVG)

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(LineCap e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case ButtCap:
            m_value.ident = CSSValueButt;
            break;
        case RoundCap:
            m_value.ident = CSSValueRound;
            break;
        case SquareCap:
            m_value.ident = CSSValueSquare;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator LineCap() const
{
    switch (m_value.ident) {
        case CSSValueButt:
            return ButtCap;
        case CSSValueRound:
            return RoundCap;
        case CSSValueSquare:
            return SquareCap;
        default:
            ASSERT_NOT_REACHED();
            return ButtCap;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(LineJoin e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case MiterJoin:
            m_value.ident = CSSValueMiter;
            break;
        case RoundJoin:
            m_value.ident = CSSValueRound;
            break;
        case BevelJoin:
            m_value.ident = CSSValueBevel;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator LineJoin() const
{
    switch (m_value.ident) {
        case CSSValueMiter:
            return MiterJoin;
        case CSSValueRound:
            return RoundJoin;
        case CSSValueBevel:
            return BevelJoin;
        default:
            ASSERT_NOT_REACHED();
            return MiterJoin;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(WindRule e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case RULE_NONZERO:
            m_value.ident = CSSValueNonzero;
            break;
        case RULE_EVENODD:
            m_value.ident = CSSValueEvenodd;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator WindRule() const
{
    switch (m_value.ident) {
        case CSSValueNonzero:
            return RULE_NONZERO;
        case CSSValueEvenodd:
            return RULE_EVENODD;
        default:
            ASSERT_NOT_REACHED();
            return RULE_NONZERO;
    }
}


template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EAlignmentBaseline e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case AB_AUTO:
            m_value.ident = CSSValueAuto;
            break;
        case AB_BASELINE:
            m_value.ident = CSSValueBaseline;
            break;
        case AB_BEFORE_EDGE:
            m_value.ident = CSSValueBeforeEdge;
            break;
        case AB_TEXT_BEFORE_EDGE:
            m_value.ident = CSSValueTextBeforeEdge;
            break;
        case AB_MIDDLE:
            m_value.ident = CSSValueMiddle;
            break;
        case AB_CENTRAL:
            m_value.ident = CSSValueCentral;
            break;
        case AB_AFTER_EDGE:
            m_value.ident = CSSValueAfterEdge;
            break;
        case AB_TEXT_AFTER_EDGE:
            m_value.ident = CSSValueTextAfterEdge;
            break;
        case AB_IDEOGRAPHIC:
            m_value.ident = CSSValueIdeographic;
            break;
        case AB_ALPHABETIC:
            m_value.ident = CSSValueAlphabetic;
            break;
        case AB_HANGING:
            m_value.ident = CSSValueHanging;
            break;
        case AB_MATHEMATICAL:
            m_value.ident = CSSValueMathematical;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EAlignmentBaseline() const
{
    switch (m_value.ident) {
        case CSSValueAuto:
            return AB_AUTO;
        case CSSValueBaseline:
            return AB_BASELINE;
        case CSSValueBeforeEdge:
            return AB_BEFORE_EDGE;
        case CSSValueTextBeforeEdge:
            return AB_TEXT_BEFORE_EDGE;
        case CSSValueMiddle:
            return AB_MIDDLE;
        case CSSValueCentral:
            return AB_CENTRAL;
        case CSSValueAfterEdge:
            return AB_AFTER_EDGE;
        case CSSValueTextAfterEdge:
            return AB_TEXT_AFTER_EDGE;
        case CSSValueIdeographic:
            return AB_IDEOGRAPHIC;
        case CSSValueAlphabetic:
            return AB_ALPHABETIC;
        case CSSValueHanging:
            return AB_HANGING;
        case CSSValueMathematical:
            return AB_MATHEMATICAL;
        default:
            ASSERT_NOT_REACHED();
            return AB_AUTO;
    }
}

#endif

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EBorderCollapse e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case BSEPARATE:
        m_value.ident = CSSValueSeparate;
        break;
    case BCOLLAPSE:
        m_value.ident = CSSValueCollapse;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EBorderCollapse() const
{
    switch (m_value.ident) {
    case CSSValueSeparate:
        return BSEPARATE;
    case CSSValueCollapse:
        return BCOLLAPSE;
    default:
        ASSERT_NOT_REACHED();
        return BSEPARATE;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EBorderFit e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case BorderFitBorder:
        m_value.ident = CSSValueBorder;
        break;
    case BorderFitLines:
        m_value.ident = CSSValueLines;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EBorderFit() const
{
    switch (m_value.ident) {
    case CSSValueBorder:
        return BorderFitBorder;
    case CSSValueLines:
        return BorderFitLines;
    default:
        ASSERT_NOT_REACHED();
        return BorderFitLines;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EImageRendering e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case ImageRenderingAuto:
        m_value.ident = CSSValueAuto;
        break;
    case ImageRenderingOptimizeSpeed:
        m_value.ident = CSSValueOptimizespeed;
        break;
    case ImageRenderingOptimizeQuality:
        m_value.ident = CSSValueOptimizequality;
        break;
    case ImageRenderingOptimizeContrast:
        m_value.ident = CSSValueWebkitOptimizeContrast;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EImageRendering() const
{
    switch (m_value.ident) {
    case CSSValueAuto:
        return ImageRenderingAuto;
    case CSSValueOptimizespeed:
        return ImageRenderingOptimizeSpeed;
    case CSSValueOptimizequality:
        return ImageRenderingOptimizeQuality;
    case CSSValueWebkitOptimizeContrast:
        return ImageRenderingOptimizeContrast;
    default:
        ASSERT_NOT_REACHED();
        return ImageRenderingAuto;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ETransformStyle3D e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case TransformStyle3DFlat:
        m_value.ident = CSSValueFlat;
        break;
    case TransformStyle3DPreserve3D:
        m_value.ident = CSSValuePreserve3d;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator ETransformStyle3D() const
{
    switch (m_value.ident) {
    case CSSValueFlat:
        return TransformStyle3DFlat;
    case CSSValuePreserve3d:
        return TransformStyle3DPreserve3D;
    default:
        ASSERT_NOT_REACHED();
        return TransformStyle3DFlat;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ColumnAxis e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case HorizontalColumnAxis:
        m_value.ident = CSSValueHorizontal;
        break;
    case VerticalColumnAxis:
        m_value.ident = CSSValueVertical;
        break;
    case AutoColumnAxis:
        m_value.ident = CSSValueAuto;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator ColumnAxis() const
{
    switch (m_value.ident) {
    case CSSValueHorizontal:
        return HorizontalColumnAxis;
    case CSSValueVertical:
        return VerticalColumnAxis;
    case CSSValueAuto:
        return AutoColumnAxis;
    default:
        ASSERT_NOT_REACHED();
        return AutoColumnAxis;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(WrapFlow wrapFlow)
: CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (wrapFlow) {
    case WrapFlowAuto:
        m_value.ident = CSSValueAuto;
        break;
    case WrapFlowBoth:
        m_value.ident = CSSValueBoth;
        break;
    case WrapFlowLeft:
        m_value.ident = CSSValueLeft;
        break;
    case WrapFlowRight:
        m_value.ident = CSSValueRight;
        break;
    case WrapFlowMaximum:
        m_value.ident = CSSValueMaximum;
        break;
    case WrapFlowClear:
        m_value.ident = CSSValueClear;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator WrapFlow() const
{
    switch (m_value.ident) {
    case CSSValueAuto:
        return WrapFlowAuto;
    case CSSValueBoth:
        return WrapFlowBoth;
    case CSSValueLeft:
        return WrapFlowLeft;
    case CSSValueRight:
        return WrapFlowRight;
    case CSSValueMaximum:
        return WrapFlowMaximum;
    case CSSValueClear:
        return WrapFlowClear;
    default:
        ASSERT_NOT_REACHED();
        return WrapFlowAuto;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(WrapThrough wrapThrough)
: CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (wrapThrough) {
    case WrapThroughWrap:
        m_value.ident = CSSValueWrap;
        break;
    case WrapThroughNone:
        m_value.ident = CSSValueNone;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator WrapThrough() const
{
    switch (m_value.ident) {
    case CSSValueWrap:
        return WrapThroughWrap;
    case CSSValueNone:
        return WrapThroughNone;
    default:
        ASSERT_NOT_REACHED();
        return WrapThroughWrap;
    }
}

#if ENABLE(SVG)

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EColorInterpolation e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case CI_AUTO:
            m_value.ident = CSSValueAuto;
            break;
        case CI_SRGB:
            m_value.ident = CSSValueSrgb;
            break;
        case CI_LINEARRGB:
            m_value.ident = CSSValueLinearrgb;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EColorInterpolation() const
{
    switch (m_value.ident) {
        case CSSValueSrgb:
            return CI_SRGB;
        case CSSValueLinearrgb:
            return CI_LINEARRGB;
        case CSSValueAuto:
            return CI_AUTO;
        default:
            ASSERT_NOT_REACHED();
            return CI_AUTO;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EColorRendering e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case CR_AUTO:
            m_value.ident = CSSValueAuto;
            break;
        case CR_OPTIMIZESPEED:
            m_value.ident = CSSValueOptimizespeed;
            break;
        case CR_OPTIMIZEQUALITY:
            m_value.ident = CSSValueOptimizequality;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EColorRendering() const
{
    switch (m_value.ident) {
        case CSSValueOptimizespeed:
            return CR_OPTIMIZESPEED;
        case CSSValueOptimizequality:
            return CR_OPTIMIZEQUALITY;
        case CSSValueAuto:
            return CR_AUTO;
        default:
            ASSERT_NOT_REACHED();
            return CR_AUTO;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EDominantBaseline e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case DB_AUTO:
            m_value.ident = CSSValueAuto;
            break;
        case DB_USE_SCRIPT:
            m_value.ident = CSSValueUseScript;
            break;
        case DB_NO_CHANGE:
            m_value.ident = CSSValueNoChange;
            break;
        case DB_RESET_SIZE:
            m_value.ident = CSSValueResetSize;
            break;
        case DB_CENTRAL:
            m_value.ident = CSSValueCentral;
            break;
        case DB_MIDDLE:
            m_value.ident = CSSValueMiddle;
            break;
        case DB_TEXT_BEFORE_EDGE:
            m_value.ident = CSSValueTextBeforeEdge;
            break;
        case DB_TEXT_AFTER_EDGE:
            m_value.ident = CSSValueTextAfterEdge;
            break;
        case DB_IDEOGRAPHIC:
            m_value.ident = CSSValueIdeographic;
            break;
        case DB_ALPHABETIC:
            m_value.ident = CSSValueAlphabetic;
            break;
        case DB_HANGING:
            m_value.ident = CSSValueHanging;
            break;
        case DB_MATHEMATICAL:
            m_value.ident = CSSValueMathematical;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator EDominantBaseline() const
{
    switch (m_value.ident) {
        case CSSValueAuto:
            return DB_AUTO;
        case CSSValueUseScript:
            return DB_USE_SCRIPT;
        case CSSValueNoChange:
            return DB_NO_CHANGE;
        case CSSValueResetSize:
            return DB_RESET_SIZE;
        case CSSValueIdeographic:
            return DB_IDEOGRAPHIC;
        case CSSValueAlphabetic:
            return DB_ALPHABETIC;
        case CSSValueHanging:
            return DB_HANGING;
        case CSSValueMathematical:
            return DB_MATHEMATICAL;
        case CSSValueCentral:
            return DB_CENTRAL;
        case CSSValueMiddle:
            return DB_MIDDLE;
        case CSSValueTextAfterEdge:
            return DB_TEXT_AFTER_EDGE;
        case CSSValueTextBeforeEdge:
            return DB_TEXT_BEFORE_EDGE;
        default:
            ASSERT_NOT_REACHED();
            return DB_AUTO;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EShapeRendering e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case SR_AUTO:
        m_value.ident = CSSValueAuto;
        break;
    case SR_OPTIMIZESPEED:
        m_value.ident = CSSValueOptimizespeed;
        break;
    case SR_CRISPEDGES:
        m_value.ident = CSSValueCrispedges;
        break;
    case SR_GEOMETRICPRECISION:
        m_value.ident = CSSValueGeometricprecision;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EShapeRendering() const
{
    switch (m_value.ident) {
    case CSSValueAuto:
        return SR_AUTO;
    case CSSValueOptimizespeed:
        return SR_OPTIMIZESPEED;
    case CSSValueCrispedges:
        return SR_CRISPEDGES;
    case CSSValueGeometricprecision:
        return SR_GEOMETRICPRECISION;
    default:
        ASSERT_NOT_REACHED();
        return SR_AUTO;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(ETextAnchor e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case TA_START:
            m_value.ident = CSSValueStart;
            break;
        case TA_MIDDLE:
            m_value.ident = CSSValueMiddle;
            break;
        case TA_END:
            m_value.ident = CSSValueEnd;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator ETextAnchor() const
{
    switch (m_value.ident) {
        case CSSValueStart:
            return TA_START;
        case CSSValueMiddle:
            return TA_MIDDLE;
        case CSSValueEnd:
            return TA_END;
        default:
            ASSERT_NOT_REACHED();
            return TA_START;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(SVGWritingMode e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
        case WM_LRTB:
            m_value.ident = CSSValueLrTb;
            break;
        case WM_LR:
            m_value.ident = CSSValueLr;
            break;
        case WM_RLTB:
            m_value.ident = CSSValueRlTb;
            break;
        case WM_RL:
            m_value.ident = CSSValueRl;
            break;
        case WM_TBRL:
            m_value.ident = CSSValueTbRl;
            break;
        case WM_TB:
            m_value.ident = CSSValueTb;
            break;
    }
}

template<> inline CSSPrimitiveValue::operator SVGWritingMode() const
{
    switch (m_value.ident) {
    case CSSValueLrTb:
        return WM_LRTB;
    case CSSValueLr:
        return WM_LR;
    case CSSValueRlTb:
        return WM_RLTB;
    case CSSValueRl:
        return WM_RL;
    case CSSValueTbRl:
        return WM_TBRL;
    case CSSValueTb:
        return WM_TB;
    default:
        ASSERT_NOT_REACHED();
        return WM_LRTB;
    }
}

template<> inline CSSPrimitiveValue::CSSPrimitiveValue(EVectorEffect e)
    : CSSValue(PrimitiveClass)
{
    m_primitiveUnitType = CSS_IDENT;
    switch (e) {
    case VE_NONE:
        m_value.ident = CSSValueNone;
        break;
    case VE_NON_SCALING_STROKE:
        m_value.ident = CSSValueNonScalingStroke;
        break;
    }
}

template<> inline CSSPrimitiveValue::operator EVectorEffect() const
{
    switch (m_value.ident) {
    case CSSValueNone:
        return VE_NONE;
    case CSSValueNonScalingStroke:
        return VE_NON_SCALING_STROKE;
    default:
        ASSERT_NOT_REACHED();
        return VE_NONE;
    }
}

#endif // ENABLE(SVG)

}

#endif
