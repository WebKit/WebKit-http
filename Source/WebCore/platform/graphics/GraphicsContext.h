/*
 * Copyright (C) 2003, 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2008-2009 Torch Mobile, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GraphicsContext_h
#define GraphicsContext_h

#include "ColorSpace.h"
#include "DashArray.h"
#include "FloatRect.h"
#include "Font.h"
#include "Gradient.h"
#include "Image.h"
#include "ImageOrientation.h"
#include "Path.h"
#include "Pattern.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

#if USE(CG)
typedef struct CGContext PlatformGraphicsContext;
#elif USE(CAIRO)
namespace WebCore {
class PlatformContextCairo;
}
typedef WebCore::PlatformContextCairo PlatformGraphicsContext;
#elif USE(WINGDI)
typedef struct HDC__ PlatformGraphicsContext;
#else
typedef void PlatformGraphicsContext;
#endif

#if PLATFORM(WIN)
#include "DIBPixelData.h"
typedef struct HDC__* HDC;
#if !USE(CG)
// UInt8 is defined in CoreFoundation/CFBase.h
typedef unsigned char UInt8;
#endif
#endif

namespace WebCore {

#if USE(WINGDI)
    class SharedBitmap;
    class SimpleFontData;
    class GlyphBuffer;
#endif

    const int cMisspellingLineThickness = 3;
    const int cMisspellingLinePatternWidth = 4;
    const int cMisspellingLinePatternGapWidth = 1;

    class AffineTransform;
    class DrawingBuffer;
    class FloatRoundedRect;
    class Gradient;
    class GraphicsContextPlatformPrivate;
    class ImageBuffer;
    class IntRect;
    class RoundedRect;
    class URL;
    class GraphicsContext3D;
    class TextRun;
    class TransformationMatrix;
#if PLATFORM(IOS)
    struct BidiStatus;
#endif

    enum TextDrawingMode {
        TextModeFill = 1 << 0,
        TextModeStroke = 1 << 1,
#if ENABLE(LETTERPRESS)
        TextModeLetterpress = 1 << 2,
#endif
    };
    typedef unsigned TextDrawingModeFlags;

    enum StrokeStyle {
        NoStroke,
        SolidStroke,
        DottedStroke,
        DashedStroke,
        DoubleStroke,
        WavyStroke,
    };

    enum InterpolationQuality {
        InterpolationDefault,
        InterpolationNone,
        InterpolationLow,
        InterpolationMedium,
        InterpolationHigh
    };

    struct GraphicsContextState {
        GraphicsContextState()
            : strokeThickness(0)
            , shadowBlur(0)
            , textDrawingMode(TextModeFill)
            , strokeColor(Color::black)
            , fillColor(Color::black)
            , strokeStyle(SolidStroke)
            , fillRule(RULE_NONZERO)
            , strokeColorSpace(ColorSpaceDeviceRGB)
            , fillColorSpace(ColorSpaceDeviceRGB)
            , shadowColorSpace(ColorSpaceDeviceRGB)
            , compositeOperator(CompositeSourceOver)
            , blendMode(BlendModeNormal)
#if PLATFORM(IOS)
            , emojiDrawingEnabled(true)
            , shouldUseContextColors(true)
#endif
            , shouldAntialias(true)
            , shouldSmoothFonts(true)
            , shouldSubpixelQuantizeFonts(true)
            , paintingDisabled(false)
            , shadowsIgnoreTransforms(false)
#if USE(CG)
            // Core Graphics incorrectly renders shadows with radius > 8px (<rdar://problem/8103442>),
            // but we need to preserve this buggy behavior for canvas and -webkit-box-shadow.
            , shadowsUseLegacyRadius(false)
#endif
            , drawLuminanceMask(false)
        {
        }

        RefPtr<Gradient> strokeGradient;
        RefPtr<Pattern> strokePattern;
        
        RefPtr<Gradient> fillGradient;
        RefPtr<Pattern> fillPattern;

        FloatSize shadowOffset;

        float strokeThickness;
        float shadowBlur;

        TextDrawingModeFlags textDrawingMode;

        Color strokeColor;
        Color fillColor;
        Color shadowColor;

        StrokeStyle strokeStyle;
        WindRule fillRule;

        ColorSpace strokeColorSpace;
        ColorSpace fillColorSpace;
        ColorSpace shadowColorSpace;

        CompositeOperator compositeOperator;
        BlendMode blendMode;

#if PLATFORM(IOS)
        bool emojiDrawingEnabled : 1;
        bool shouldUseContextColors : 1;
#endif
        bool shouldAntialias : 1;
        bool shouldSmoothFonts : 1;
        bool shouldSubpixelQuantizeFonts : 1;
        bool paintingDisabled : 1;
        bool shadowsIgnoreTransforms : 1;
#if USE(CG)
        bool shadowsUseLegacyRadius : 1;
#endif
        bool drawLuminanceMask : 1;
    };

#if PLATFORM(IOS)
    void setStrokeAndFillColor(PlatformGraphicsContext*, CGColorRef);
#endif

    struct ImagePaintingOptions {
        ImagePaintingOptions(CompositeOperator compositeOperator = CompositeSourceOver, BlendMode blendMode = BlendModeNormal, ImageOrientationDescription orientationDescription = ImageOrientationDescription(), bool useLowQualityScale = false)
            : m_compositeOperator(compositeOperator)
            , m_blendMode(blendMode)
            , m_orientationDescription(orientationDescription)
            , m_useLowQualityScale(useLowQualityScale)
        {
        }

        ImagePaintingOptions(ImageOrientationDescription orientationDescription, bool useLowQualityScale = false, CompositeOperator compositeOperator = CompositeSourceOver, BlendMode blendMode = BlendModeNormal)
            : m_compositeOperator(compositeOperator)
            , m_blendMode(blendMode)
            , m_orientationDescription(orientationDescription)
            , m_useLowQualityScale(useLowQualityScale)
        {
        }

        ImagePaintingOptions(bool useLowQualityScale, ImageOrientationDescription orientationDescription = ImageOrientationDescription(), CompositeOperator compositeOperator = CompositeSourceOver, BlendMode blendMode = BlendModeNormal)
            : m_compositeOperator(compositeOperator)
            , m_blendMode(blendMode)
            , m_orientationDescription(orientationDescription)
            , m_useLowQualityScale(useLowQualityScale)
        {
        }

        CompositeOperator m_compositeOperator;
        BlendMode m_blendMode;
        ImageOrientationDescription m_orientationDescription;
        bool m_useLowQualityScale;
    };

    class GraphicsContext {
        WTF_MAKE_NONCOPYABLE(GraphicsContext); WTF_MAKE_FAST_ALLOCATED;
    public:
#if !PLATFORM(IOS)
        GraphicsContext(PlatformGraphicsContext*);
#else
        GraphicsContext(PlatformGraphicsContext*, bool shouldUseContextColors = true);
#endif
        ~GraphicsContext();

        PlatformGraphicsContext* platformContext() const;

        float strokeThickness() const;
        void setStrokeThickness(float);
        StrokeStyle strokeStyle() const;
        void setStrokeStyle(StrokeStyle);
        Color strokeColor() const;
        ColorSpace strokeColorSpace() const;
        void setStrokeColor(const Color&, ColorSpace);

        void setStrokePattern(PassRefPtr<Pattern>);
        Pattern* strokePattern() const;

        void setStrokeGradient(PassRefPtr<Gradient>);
        Gradient* strokeGradient() const;

        WindRule fillRule() const;
        void setFillRule(WindRule);
        Color fillColor() const;
        ColorSpace fillColorSpace() const;
        void setFillColor(const Color&, ColorSpace);

        void setFillPattern(PassRefPtr<Pattern>);
        Pattern* fillPattern() const;

        void setFillGradient(PassRefPtr<Gradient>);
        Gradient* fillGradient() const;

        void setShadowsIgnoreTransforms(bool);
        bool shadowsIgnoreTransforms() const;

        void setShouldAntialias(bool);
        bool shouldAntialias() const;

        void setShouldSmoothFonts(bool);
        bool shouldSmoothFonts() const;

        // Normally CG enables subpixel-quantization because it improves the performance of aligning glyphs.
        // In some cases we have to disable to to ensure a high-quality output of the glyphs.
        void setShouldSubpixelQuantizeFonts(bool);
        bool shouldSubpixelQuantizeFonts() const;

        const GraphicsContextState& state() const;

#if USE(CG)
        void applyStrokePattern();
        void applyFillPattern();
        void drawPath(const Path&);

        void drawNativeImage(PassNativeImagePtr, const FloatSize& selfSize, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect, float scale = 1, CompositeOperator = CompositeSourceOver, BlendMode = BlendModeNormal, ImageOrientation = DefaultImageOrientation);

        // Allow font smoothing (LCD antialiasing). Not part of the graphics state.
        void setAllowsFontSmoothing(bool);
        
        void setIsCALayerContext(bool);
        bool isCALayerContext() const;

        void setIsAcceleratedContext(bool);
#endif
        bool isAcceleratedContext() const;

        void save();
        void restore();

        // These draw methods will do both stroking and filling.
        // FIXME: ...except drawRect(), which fills properly but always strokes
        // using a 1-pixel stroke inset from the rect borders (of the correct
        // stroke color).
        void drawRect(const FloatRect&, float borderThickness = 1);
        void drawLine(const FloatPoint&, const FloatPoint&);

#if PLATFORM(IOS)
        void drawJoinedLines(CGPoint points[], unsigned count, bool antialias, CGLineCap = kCGLineCapButt);
#endif

        void drawEllipse(const IntRect&);
#if PLATFORM(IOS)
        void drawEllipse(const FloatRect&);
        void drawRaisedEllipse(const FloatRect&, const Color& ellipseColor, ColorSpace ellipseColorSpace, const Color& shadowColor, ColorSpace shadowColorSpace);
#endif
        void drawConvexPolygon(size_t numPoints, const FloatPoint*, bool shouldAntialias = false);

        void fillPath(const Path&);
        void strokePath(const Path&);

        void fillEllipse(const FloatRect&);
        void strokeEllipse(const FloatRect&);

        void fillRect(const FloatRect&);
        void fillRect(const FloatRect&, const Color&, ColorSpace);
        void fillRect(const FloatRect&, Gradient&);
        void fillRect(const FloatRect&, const Color&, ColorSpace, CompositeOperator, BlendMode = BlendModeNormal);
        void fillRoundedRect(const FloatRoundedRect&, const Color&, ColorSpace, BlendMode = BlendModeNormal);
        void fillRectWithRoundedHole(const FloatRect&, const FloatRoundedRect& roundedHoleRect, const Color&, ColorSpace);

        void clearRect(const FloatRect&);

        void strokeRect(const FloatRect&, float lineWidth);

        void drawImage(Image*, ColorSpace, const FloatPoint& destination, const ImagePaintingOptions& = ImagePaintingOptions());
        void drawImage(Image*, ColorSpace, const FloatRect& destination, const ImagePaintingOptions& = ImagePaintingOptions());
        void drawImage(Image*, ColorSpace, const FloatRect& destination, const FloatRect& source, const ImagePaintingOptions& = ImagePaintingOptions());

        void drawTiledImage(Image*, ColorSpace, const FloatRect& destination, const FloatPoint& source, const FloatSize& tileSize,
            const ImagePaintingOptions& = ImagePaintingOptions());
        void drawTiledImage(Image*, ColorSpace, const FloatRect& destination, const FloatRect& source, const FloatSize& tileScaleFactor,
            Image::TileRule, Image::TileRule, const ImagePaintingOptions& = ImagePaintingOptions());

        void drawImageBuffer(ImageBuffer*, ColorSpace, const FloatPoint& destination, const ImagePaintingOptions& = ImagePaintingOptions());
        void drawImageBuffer(ImageBuffer*, ColorSpace, const FloatRect& destination, const ImagePaintingOptions& = ImagePaintingOptions());
        void drawImageBuffer(ImageBuffer*, ColorSpace, const FloatRect& destination, const FloatRect& source, const ImagePaintingOptions& = ImagePaintingOptions());

        void setImageInterpolationQuality(InterpolationQuality);
        InterpolationQuality imageInterpolationQuality() const;

        void clip(const IntRect&);
        void clip(const FloatRect&);
        void clipRoundedRect(const FloatRoundedRect&);

        void clipOut(const FloatRect&);
        void clipOutRoundedRect(const FloatRoundedRect&);
        void clipPath(const Path&, WindRule);
        void clipConvexPolygon(size_t numPoints, const FloatPoint*, bool antialias = true);
        void clipToImageBuffer(ImageBuffer*, const FloatRect&);
        
        IntRect clipBounds() const;

        TextDrawingModeFlags textDrawingMode() const;
        void setTextDrawingMode(TextDrawingModeFlags);

#if PLATFORM(IOS)
        bool emojiDrawingEnabled();
        void setEmojiDrawingEnabled(bool);
#endif
        
#if !PLATFORM(IOS)
        void drawText(const Font&, const TextRun&, const FloatPoint&, int from = 0, int to = -1);
#else
        float drawText(const Font&, const TextRun&, const FloatPoint&, int from = 0, int to = -1);
#endif
        void drawGlyphs(const Font&, const SimpleFontData&, const GlyphBuffer&, int from, int numGlyphs, const FloatPoint&);
        void drawEmphasisMarks(const Font&, const TextRun& , const AtomicString& mark, const FloatPoint&, int from = 0, int to = -1);
#if !PLATFORM(IOS)
        void drawBidiText(const Font&, const TextRun&, const FloatPoint&, Font::CustomFontNotReadyAction = Font::DoNotPaintIfFontNotReady);
#else
        float drawBidiText(const Font&, const TextRun&, const FloatPoint&, Font::CustomFontNotReadyAction = Font::DoNotPaintIfFontNotReady, BidiStatus* = 0, int length = -1);
#endif
        enum RoundingMode {
            RoundAllSides,
            RoundOriginAndDimensions
        };
        FloatRect roundToDevicePixels(const FloatRect&, RoundingMode = RoundAllSides);

        FloatRect computeLineBoundsForText(const FloatPoint&, float width, bool printing);
        void drawLineForText(const FloatPoint&, float width, bool printing, bool doubleLines = false);
        void drawLinesForText(const FloatPoint&, const DashArray& widths, bool printing, bool doubleLines = false);
        enum DocumentMarkerLineStyle {
#if PLATFORM(IOS)
            TextCheckingDictationPhraseWithAlternativesLineStyle,
#endif
            DocumentMarkerSpellingLineStyle,
            DocumentMarkerGrammarLineStyle,
            DocumentMarkerAutocorrectionReplacementLineStyle,
            DocumentMarkerDictationAlternativesLineStyle
        };
        static void updateDocumentMarkerResources();
        void drawLineForDocumentMarker(const FloatPoint&, float width, DocumentMarkerLineStyle);

        bool paintingDisabled() const;
        void setPaintingDisabled(bool);

        bool updatingControlTints() const;
        void setUpdatingControlTints(bool);

        void beginTransparencyLayer(float opacity);
        void endTransparencyLayer();
        bool isInTransparencyLayer() const;

        bool hasShadow() const;
        void setShadow(const FloatSize&, float blur, const Color&, ColorSpace);
        // Legacy shadow blur radius is used for canvas, and -webkit-box-shadow.
        // It has different treatment of radii > 8px.
        void setLegacyShadow(const FloatSize&, float blur, const Color&, ColorSpace);

        bool getShadow(FloatSize&, float&, Color&, ColorSpace&) const;
        void clearShadow();

        bool hasBlurredShadow() const;
#if USE(CAIRO)
        bool mustUseShadowBlur() const;
#endif

        void drawFocusRing(const Vector<IntRect>&, int width, int offset, const Color&);
        void drawFocusRing(const Path&, int width, int offset, const Color&);
#if PLATFORM(MAC)
        void drawFocusRing(const Vector<IntRect>&, int width, int offset, double timeOffset, bool& needsRedraw);
#endif

        void setLineCap(LineCap);
        void setLineDash(const DashArray&, float dashOffset);
        void setLineJoin(LineJoin);
        void setMiterLimit(float);

        void setAlpha(float);

        void setCompositeOperation(CompositeOperator, BlendMode = BlendModeNormal);
        CompositeOperator compositeOperation() const;
        BlendMode blendModeOperation() const;

        void setDrawLuminanceMask(bool);
        bool drawLuminanceMask() const;

        void clip(const Path&, WindRule = RULE_EVENODD);

        // This clip function is used only by <canvas> code. It allows
        // implementations to handle clipping on the canvas differently since
        // the discipline is different.
        void canvasClip(const Path&, WindRule = RULE_EVENODD);
        void clipOut(const Path&);

        void scale(const FloatSize&);
        void rotate(float angleInRadians);
        void translate(const FloatSize& size) { translate(size.width(), size.height()); }
        void translate(float x, float y);

        void setURLForRect(const URL&, const IntRect&);

        void concatCTM(const AffineTransform&);
        void setCTM(const AffineTransform&);

        enum IncludeDeviceScale { DefinitelyIncludeDeviceScale, PossiblyIncludeDeviceScale };
        AffineTransform getCTM(IncludeDeviceScale includeScale = PossiblyIncludeDeviceScale) const;

#if ENABLE(3D_RENDERING) && USE(TEXTURE_MAPPER)
        // This is needed when using accelerated-compositing in software mode, like in TextureMapper.
        void concat3DTransform(const TransformationMatrix&);
        void set3DTransform(const TransformationMatrix&);
        TransformationMatrix get3DTransform() const;
#endif
        // Create an image buffer compatible with this context, with suitable resolution
        // for drawing into the buffer and then into this context.
        std::unique_ptr<ImageBuffer> createCompatibleBuffer(const FloatSize&, bool hasAlpha = true) const;
        bool isCompatibleWithBuffer(ImageBuffer*) const;

        // This function applies the device scale factor to the context, making the context capable of
        // acting as a base-level context for a HiDPI environment.
        void applyDeviceScaleFactor(float);
        void platformApplyDeviceScaleFactor(float);

#if OS(WINDOWS)
        HDC getWindowsContext(const IntRect&, bool supportAlphaBlend, bool mayCreateBitmap); // The passed in rect is used to create a bitmap for compositing inside transparency layers.
        void releaseWindowsContext(HDC, const IntRect&, bool supportAlphaBlend, bool mayCreateBitmap); // The passed in HDC should be the one handed back by getWindowsContext.
#if PLATFORM(WIN)
#if USE(WINGDI)
        void setBitmap(PassRefPtr<SharedBitmap>);
        const AffineTransform& affineTransform() const;
        AffineTransform& affineTransform();
        void resetAffineTransform();
        void fillRect(const FloatRect&, const Gradient*);
        void drawText(const SimpleFontData* fontData, const GlyphBuffer& glyphBuffer, int from, int numGlyphs, const FloatPoint& point);
        void drawFrameControl(const IntRect& rect, unsigned type, unsigned state);
        void drawFocusRect(const IntRect& rect);
        void paintTextField(const IntRect& rect, unsigned state);
        void drawBitmap(SharedBitmap*, const IntRect& dstRect, const IntRect& srcRect, ColorSpace styleColorSpace, CompositeOperator compositeOp, BlendMode blendMode);
        void drawBitmapPattern(SharedBitmap*, const FloatRect& tileRectIn, const AffineTransform& patternTransform, const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect, const IntSize& origSourceSize);
        void drawIcon(HICON icon, const IntRect& dstRect, UINT flags);
        void drawRoundCorner(bool newClip, RECT clipRect, RECT rectWin, HDC dc, int width, int height);
#else
        GraphicsContext(HDC, bool hasAlpha = false); // FIXME: To be removed.

        // When set to true, child windows should be rendered into this context
        // rather than allowing them just to render to the screen. Defaults to
        // false.
        // FIXME: This is a layering violation. GraphicsContext shouldn't know
        // what a "window" is. It would be much more appropriate for this flag
        // to be passed as a parameter alongside the GraphicsContext, but doing
        // that would require lots of changes in cross-platform code that we
        // aren't sure we want to make.
        void setShouldIncludeChildWindows(bool);
        bool shouldIncludeChildWindows() const;

        class WindowsBitmap {
            WTF_MAKE_NONCOPYABLE(WindowsBitmap);
        public:
            WindowsBitmap(HDC, const IntSize&);
            ~WindowsBitmap();

            HDC hdc() const { return m_hdc; }
            UInt8* buffer() const { return m_pixelData.buffer(); }
            unsigned bufferLength() const { return m_pixelData.bufferLength(); }
            const IntSize& size() const { return m_pixelData.size(); }
            unsigned bytesPerRow() const { return m_pixelData.bytesPerRow(); }
            unsigned short bitsPerPixel() const { return m_pixelData.bitsPerPixel(); }
            const DIBPixelData& windowsDIB() const { return m_pixelData; }

        private:
            HDC m_hdc;
            HBITMAP m_bitmap;
            DIBPixelData m_pixelData;
        };

        PassOwnPtr<WindowsBitmap> createWindowsBitmap(const IntSize&);
        // The bitmap should be non-premultiplied.
        void drawWindowsBitmap(WindowsBitmap*, const IntPoint&);
#endif
#else // PLATFORM(WIN)
        bool shouldIncludeChildWindows() const { return false; }
#endif // PLATFORM(WIN)
#endif // OS(WINDOWS)

#if USE(CAIRO)
        GraphicsContext(cairo_t*);
#endif

#if PLATFORM(GTK)
        void setGdkExposeEvent(GdkEventExpose*);
        GdkWindow* gdkWindow() const;
        GdkEventExpose* gdkExposeEvent() const;
#endif

        static void adjustLineToPixelBoundaries(FloatPoint& p1, FloatPoint& p2, float strokeWidth, StrokeStyle);

    private:
        void platformInit(PlatformGraphicsContext*, bool shouldUseContextColors = false);
        void platformDestroy();

#if PLATFORM(WIN) && !USE(WINGDI)
        void platformInit(HDC, bool hasAlpha = false);
#endif

        void savePlatformState();
        void restorePlatformState();

        void setPlatformTextDrawingMode(TextDrawingModeFlags);

        void setPlatformStrokeColor(const Color&, ColorSpace);
        void setPlatformStrokeStyle(StrokeStyle);
        void setPlatformStrokeThickness(float);

        void setPlatformFillColor(const Color&, ColorSpace);

        void setPlatformShouldAntialias(bool);
        void setPlatformShouldSmoothFonts(bool);

        void setPlatformShadow(const FloatSize&, float blur, const Color&, ColorSpace);
        void clearPlatformShadow();

        void setPlatformCompositeOperation(CompositeOperator, BlendMode = BlendModeNormal);

        void beginPlatformTransparencyLayer(float opacity);
        void endPlatformTransparencyLayer();
        static bool supportsTransparencyLayers();

        void fillEllipseAsPath(const FloatRect&);
        void strokeEllipseAsPath(const FloatRect&);

        void platformFillEllipse(const FloatRect&);
        void platformStrokeEllipse(const FloatRect&);

        void platformFillRoundedRect(const FloatRoundedRect&, const Color&, ColorSpace);

        FloatRect computeLineBoundsAndAntialiasingModeForText(const FloatPoint&, float width, bool printing, bool& shouldAntialias, Color&);

        GraphicsContextPlatformPrivate* m_data;

        GraphicsContextState m_state;
        Vector<GraphicsContextState> m_stack;
        bool m_updatingControlTints;
        unsigned m_transparencyCount;
    };

    class GraphicsContextStateSaver {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        GraphicsContextStateSaver(GraphicsContext& context, bool saveAndRestore = true)
        : m_context(context)
        , m_saveAndRestore(saveAndRestore)
        {
            if (m_saveAndRestore)
                m_context.save();
        }
        
        ~GraphicsContextStateSaver()
        {
            if (m_saveAndRestore)
                m_context.restore();
        }
        
        void save()
        {
            ASSERT(!m_saveAndRestore);
            m_context.save();
            m_saveAndRestore = true;
        }

        void restore()
        {
            ASSERT(m_saveAndRestore);
            m_context.restore();
            m_saveAndRestore = false;
        }
        
        GraphicsContext* context() const { return &m_context; }

    private:
        GraphicsContext& m_context;
        bool m_saveAndRestore;
    };

} // namespace WebCore

#endif // GraphicsContext_h

