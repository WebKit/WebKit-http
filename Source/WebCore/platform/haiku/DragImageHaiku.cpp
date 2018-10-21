/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DragImage.h"

#include "CachedImage.h"
#include "FontCascade.h"
#include "Image.h"

#include "NotImplemented.h"


namespace WebCore {

const float DragLabelBorderX = 4;
// Keep border_y in synch with DragController::LinkDragBorderInset.
const float DragLabelBorderY = 2;
const float DragLabelRadius = 5;
const float LabelBorderYOffset = 2;

const float MinDragLabelWidthBeforeClip = 120;
const float MaxDragLabelWidth = 200;
const float MaxDragLabelStringWidth = (MaxDragLabelWidth - 2 * DragLabelBorderX);

const float DragLinkLabelFontsize = 11;
const float DragLinkUrlFontSize = 10;

IntSize dragImageSize(DragImageRef)
{
    notImplemented();
    return IntSize(0, 0);
}

void deleteDragImage(DragImageRef)
{
    notImplemented();
}

DragImageRef scaleDragImage(DragImageRef image, FloatSize)
{
    notImplemented();
    return image;
}

DragImageRef dissolveDragImageToFraction(DragImageRef image, float)
{
    notImplemented();
    return image;
}

DragImageRef createDragImageFromImage(Image*, ImageOrientationDescription)
{
    notImplemented();
    return 0;
}

DragImageRef createDragImageIconForCachedImageFilename(const String&)
{
    notImplemented();
    return 0;
}

static FontCascade dragLabelFont(int size, bool bold, FontRenderingMode renderingMode)
{
    FontCascade result;

    FontCascadeDescription description;
    description.setWeight(bold ? boldWeightValue() : normalWeightValue());
    //description.setOneFamily(metrics.lfSmCaptionFont.lfFaceName);
    description.setSpecifiedSize((float)size);
    description.setComputedSize((float)size);
    description.setRenderingMode(renderingMode);
    result = FontCascade(WTFMove(description), 0, 0);
    result.update(0);
    return result;
}


DragImageRef createDragImageForLink(Element&, URL& url, const String& inLabel, TextIndicatorData&, FontRenderingMode fontRenderingMode, float)
{
#if 0
    // This is more or less an exact match for the Mac OS X code.

    const FontCascade* labelFont;
    const FontCascade* urlFont;

    if (fontRenderingMode == FontRenderingMode::Alternate) {
        static const FontCascade alternateRenderingModeLabelFont = dragLabelFont(DragLinkLabelFontsize, true, FontRenderingMode::Alternate);
        static const FontCascade alternateRenderingModeURLFont = dragLabelFont(DragLinkUrlFontSize, false, FontRenderingMode::Alternate);
        labelFont = &alternateRenderingModeLabelFont;
        urlFont = &alternateRenderingModeURLFont;
    } else {
        static const FontCascade normalRenderingModeLabelFont = dragLabelFont(DragLinkLabelFontsize, true, FontRenderingMode::Normal);
        static const FontCascade normalRenderingModeURLFont = dragLabelFont(DragLinkUrlFontSize, false, FontRenderingMode::Normal);
        labelFont = &normalRenderingModeLabelFont;
        urlFont = &normalRenderingModeURLFont;
    }

    bool drawURLString = true;
    bool clipURLString = false;
    bool clipLabelString = false;

    String urlString = url.string(); 
    String label = inLabel;
    if (label.isEmpty()) {
        drawURLString = false;
        label = urlString;
    }

    // First step in drawing the link drag image width.
    TextRun labelRun(label);
    TextRun urlRun(urlString);
    IntSize labelSize(labelFont->width(labelRun), labelFont->fontMetrics().ascent() + labelFont->fontMetrics().descent());

    if (labelSize.width() > MaxDragLabelStringWidth) {
        labelSize.setWidth(MaxDragLabelStringWidth);
        clipLabelString = true;
    }
    
    IntSize urlStringSize;
    IntSize imageSize(labelSize.width() + DragLabelBorderX * 2, labelSize.height() + DragLabelBorderY * 2);

    if (drawURLString) {
        urlStringSize.setWidth(urlFont->width(urlRun));
        urlStringSize.setHeight(urlFont->fontMetrics().ascent() + urlFont->fontMetrics().descent()); 
        imageSize.setHeight(imageSize.height() + urlStringSize.height());
        if (urlStringSize.width() > MaxDragLabelStringWidth) {
            imageSize.setWidth(MaxDragLabelWidth);
            clipURLString = true;
        } else
            imageSize.setWidth(std::max(labelSize.width(), urlStringSize.width()) + DragLabelBorderX * 2);
    }

    PlatformGraphicsContext* contextRef = NULL;
    // FIXME attach a BBitmap to the context and then draw
    GraphicsContext context(contextRef);
    // On Mac alpha is {0.7, 0.7, 0.7, 0.8}, however we can't control alpha
    // for drag images on win, so we use 1
    static const Color backgroundColor(140, 140, 140);
    static const IntSize radii(DragLabelRadius, DragLabelRadius);
    IntRect rect(0, 0, imageSize.width(), imageSize.height());
    context.fillRoundedRect(FloatRoundedRect(rect, radii, radii, radii, radii), backgroundColor);
 
    // Draw the text
    static const Color topColor(0, 0, 0, 255); // original alpha = 0.75
    static const Color bottomColor(255, 255, 255, 127); // original alpha = 0.5
    if (drawURLString) {
        if (clipURLString)
            urlString = StringTruncator::rightTruncate(urlString, imageSize.width() - (DragLabelBorderX * 2.0f), *urlFont);
        IntPoint textPos(DragLabelBorderX, imageSize.height() - (LabelBorderYOffset + urlFont->fontMetrics().descent()));
        WebCoreDrawDoubledTextAtPoint(context, urlString, textPos, *urlFont, topColor, bottomColor);
    }
    
    if (clipLabelString)
        label = StringTruncator::rightTruncate(label, imageSize.width() - (DragLabelBorderX * 2.0f), *labelFont);

    IntPoint textPos(DragLabelBorderX, DragLabelBorderY + labelFont->pixelSize());
    WebCoreDrawDoubledTextAtPoint(context, label, textPos, *labelFont, topColor, bottomColor);

    deallocContext(contextRef);
    return image.leak();
#endif
    return 0;
}

DragImageRef createDragImageForColor(const Color&, const FloatRect&, float, Path&)
{
    return nullptr;
}

} // namespace WebCore

