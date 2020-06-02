/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "VideoTextureCopierCV.h"

#include "FourCC.h"
#include "Logging.h"
#include "TextureCacheCV.h"
#include <pal/spi/cocoa/IOSurfaceSPI.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/StdMap.h>
#include <wtf/text/StringBuilder.h>

#if USE(OPENGL_ES)
#include <OpenGLES/ES3/glext.h>
#endif

#if USE(ANGLE)
#define EGL_EGL_PROTOTYPES 0
#include <ANGLE/egl.h>
#include <ANGLE/eglext.h>
#include <ANGLE/eglext_angle.h>
#include <ANGLE/entry_points_egl.h>
#include <ANGLE/entry_points_gles_2_0_autogen.h>
// Skip the inclusion of ANGLE's explicit context entry points for now.
#define GL_ANGLE_explicit_context
#include <ANGLE/gl2ext.h>
#include <ANGLE/gl2ext_angle.h>
#endif

#include "CoreVideoSoftLink.h"

namespace WebCore {

enum class PixelRange {
    Unknown,
    Video,
    Full,
};

enum class TransferFunctionCV {
    Unknown,
    kITU_R_709_2,
    kITU_R_601_4,
    kSMPTE_240M_1995,
    kDCI_P3,
    kP3_D65,
    kITU_R_2020,
};

static PixelRange pixelRangeFromPixelFormat(OSType pixelFormat)
{
    switch (pixelFormat) {
    case kCVPixelFormatType_4444AYpCbCr8:
    case kCVPixelFormatType_4444AYpCbCr16:
    case kCVPixelFormatType_422YpCbCr_4A_8BiPlanar:
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
    case kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange:
    case kCVPixelFormatType_422YpCbCr10BiPlanarVideoRange:
    case kCVPixelFormatType_444YpCbCr10BiPlanarVideoRange:
        return PixelRange::Video;
    case kCVPixelFormatType_420YpCbCr8PlanarFullRange:
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
    case kCVPixelFormatType_422YpCbCr8FullRange:
    case kCVPixelFormatType_ARGB2101010LEPacked:
    case kCVPixelFormatType_420YpCbCr10BiPlanarFullRange:
    case kCVPixelFormatType_422YpCbCr10BiPlanarFullRange:
    case kCVPixelFormatType_444YpCbCr10BiPlanarFullRange:
        return PixelRange::Full;
    default:
        return PixelRange::Unknown;
    }
}

static TransferFunctionCV transferFunctionFromString(CFStringRef string)
{
    if (!string || CFGetTypeID(string) != CFStringGetTypeID())
        return TransferFunctionCV::Unknown;
    if (CFEqual(string, kCVImageBufferYCbCrMatrix_ITU_R_709_2))
        return TransferFunctionCV::kITU_R_709_2;
    if (CFEqual(string, kCVImageBufferYCbCrMatrix_ITU_R_601_4))
        return TransferFunctionCV::kITU_R_601_4;
    if (CFEqual(string, kCVImageBufferYCbCrMatrix_SMPTE_240M_1995))
        return TransferFunctionCV::kSMPTE_240M_1995;
    if (canLoad_CoreVideo_kCVImageBufferYCbCrMatrix_DCI_P3() && CFEqual(string, kCVImageBufferYCbCrMatrix_DCI_P3))
        return TransferFunctionCV::kDCI_P3;
    if (canLoad_CoreVideo_kCVImageBufferYCbCrMatrix_P3_D65() && CFEqual(string, kCVImageBufferYCbCrMatrix_P3_D65))
        return TransferFunctionCV::kP3_D65;
    if (canLoad_CoreVideo_kCVImageBufferYCbCrMatrix_ITU_R_2020() && CFEqual(string, kCVImageBufferYCbCrMatrix_ITU_R_2020))
        return TransferFunctionCV::kITU_R_2020;
    return TransferFunctionCV::Unknown;
}

struct GLfloatColor {
    union {
        struct {
            GLfloat r;
            GLfloat g;
            GLfloat b;
        } rgb;
        struct {
            GLfloat y;
            GLfloat cb;
            GLfloat cr;
        } ycbcr;
    };

    constexpr GLfloatColor(GLfloat r, GLfloat g, GLfloat b)
        : rgb { r, g, b }
    {
    }

    constexpr GLfloatColor(int r, int g, int b, GLfloat scale)
        : rgb { r / scale, g / scale, b / scale}
    {
    }

    static constexpr GLfloat abs(GLfloat value)
    {
        return value >= 0 ? value : -value;
    }

    constexpr bool isApproximatelyEqualTo(const GLfloatColor& color, GLfloat maxDelta) const
    {
        return abs(rgb.r - color.rgb.r) < abs(maxDelta)
            && abs(rgb.g - color.rgb.g) < abs(maxDelta)
            && abs(rgb.b - color.rgb.b) < abs(maxDelta);
    }
};

struct GLfloatColors {
    static constexpr GLfloatColor black   {0, 0, 0};
    static constexpr GLfloatColor white   {1, 1, 1};
    static constexpr GLfloatColor red     {1, 0, 0};
    static constexpr GLfloatColor green   {0, 1, 0};
    static constexpr GLfloatColor blue    {0, 0, 1};
    static constexpr GLfloatColor cyan    {0, 1, 1};
    static constexpr GLfloatColor magenta {1, 0, 1};
    static constexpr GLfloatColor yellow  {1, 1, 0};
};

struct YCbCrMatrix {
    union {
        GLfloat rows[4][4];
        GLfloat data[16];
    };

    constexpr YCbCrMatrix(PixelRange, GLfloat cbCoefficient, GLfloat crCoefficient);

    operator Vector<GLfloat>() const
    {
        Vector<GLfloat> vector;
        vector.append(data, 16);
        return vector;
    }

    constexpr GLfloatColor operator*(const GLfloatColor&) const;
};

constexpr YCbCrMatrix::YCbCrMatrix(PixelRange range, GLfloat cbCoefficient, GLfloat crCoefficient)
    : rows { }
{
    // The conversion from YCbCr -> RGB generally takes the form:
    // Y = Kr * R + Kg * G + Kb * B
    // Cb = (B - Y) / (2 * (1 - Kb))
    // Cr = (R - Y) / (2 * (1 - Kr))
    // Where the values of Kb and Kr are defined in a specification and Kg is derived from: Kr + Kg + Kb = 1
    //
    // Solving the above equations for R, B, and G derives the following:
    // R = Y + (2 * (1 - Kr)) * Cr
    // B = Y + (2 * (1 - Kb)) * Cb
    // G = Y - (2 * (1 - Kb)) * (Kb / Kg) * Cb - ((1 - Kr) * 2) * (Kr / Kg) * Cr
    //
    // When the color values are Video range, Y has a range of [16, 235] with a width of 219, and Cb & Cr have
    // a range of [16, 240] with a width of 224. When the color values are Full range, Y, Cb, and Cr all have
    // a range of [0, 255] with a width of 256.

    GLfloat cgCoefficient = 1 - cbCoefficient - crCoefficient;
    GLfloat yScalingFactor = range == PixelRange::Full ? 1.f : 255.f / 219.f;
    GLfloat cbcrScalingFactor = range == PixelRange::Full ? 1.f : 255.f / 224.f;

    rows[0][0] = yScalingFactor;
    rows[0][1] = 0;
    rows[0][2] = cbcrScalingFactor * 2 * (1 - crCoefficient);
    rows[0][3] = 0;

    rows[1][0] = yScalingFactor;
    rows[1][1] = -cbcrScalingFactor * 2 * (1 - cbCoefficient) * (cbCoefficient / cgCoefficient);
    rows[1][2] = -cbcrScalingFactor * 2 * (1 - crCoefficient) * (crCoefficient / cgCoefficient);
    rows[1][3] = 0;

    rows[2][0] = yScalingFactor;
    rows[2][1] = cbcrScalingFactor * 2 * (1 - cbCoefficient);
    rows[2][2] = 0;
    rows[2][3] = 0;

    rows[3][0] = 0;
    rows[3][1] = 0;
    rows[3][2] = 0;
    rows[3][3] = 1;

    // Configure the final column of the matrix to convert Cb and Cr to [-128, 128]
    // and, in the case of video-range, to convert Y to [16, 240]:
    for (auto rowNumber = 0; rowNumber < 3; ++rowNumber) {
        auto& row = rows[rowNumber];
        auto& x = row[0];
        auto& y = row[1];
        auto& z = row[2];
        auto& w = row[3];

        w -= (y + z) * 128 / 255;
        if (range == PixelRange::Video)
            w -= x * 16 / 255;
    }
}

constexpr GLfloatColor YCbCrMatrix::operator*(const GLfloatColor& color) const
{
    return GLfloatColor(
        rows[0][0] * color.rgb.r + rows[0][1] * color.rgb.g + rows[0][2] * color.rgb.b + rows[0][3],
        rows[1][0] * color.rgb.r + rows[1][1] * color.rgb.g + rows[1][2] * color.rgb.b + rows[1][3],
        rows[2][0] * color.rgb.r + rows[2][1] * color.rgb.g + rows[2][2] * color.rgb.b + rows[2][3]
    );
}

static const Vector<GLfloat> YCbCrToRGBMatrixForRangeAndTransferFunction(PixelRange range, TransferFunctionCV transferFunction)
{
    using MapKey = std::pair<PixelRange, TransferFunctionCV>;
    using MatrixMap = StdMap<MapKey, Vector<GLfloat>>;

    static NeverDestroyed<MatrixMap> matrices;
    static dispatch_once_t onceToken;

    // Matrices are derived from the components in the ITU R.601 rev 4 specification
    // https://www.itu.int/rec/R-REC-BT.601
    constexpr static YCbCrMatrix r601VideoMatrix { PixelRange::Video, 0.114f, 0.299f };
    constexpr static YCbCrMatrix r601FullMatrix { PixelRange::Full, 0.114f, 0.299f };

    static_assert((r601VideoMatrix * GLfloatColor(16,  128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::black,   1.5f / 255.f), "r.610 video matrix does not produce black color");
    static_assert((r601VideoMatrix * GLfloatColor(235, 128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::white,   1.5f / 255.f), "r.610 video matrix does not produce white color");
    static_assert((r601VideoMatrix * GLfloatColor(81,  90,  240, 255)).isApproximatelyEqualTo(GLfloatColors::red,     1.5f / 255.f), "r.610 video matrix does not produce red color");
    static_assert((r601VideoMatrix * GLfloatColor(145, 54,  34,  255)).isApproximatelyEqualTo(GLfloatColors::green,   1.5f / 255.f), "r.610 video matrix does not produce green color");
    static_assert((r601VideoMatrix * GLfloatColor(41,  240, 110, 255)).isApproximatelyEqualTo(GLfloatColors::blue,    1.5f / 255.f), "r.610 video matrix does not produce blue color");
    static_assert((r601VideoMatrix * GLfloatColor(210, 16,  146, 255)).isApproximatelyEqualTo(GLfloatColors::yellow,  1.5f / 255.f), "r.610 video matrix does not produce yellow color");
    static_assert((r601VideoMatrix * GLfloatColor(106, 202, 222, 255)).isApproximatelyEqualTo(GLfloatColors::magenta, 1.5f / 255.f), "r.610 video matrix does not produce magenta color");
    static_assert((r601VideoMatrix * GLfloatColor(170, 166, 16,  255)).isApproximatelyEqualTo(GLfloatColors::cyan,    1.5f / 255.f), "r.610 video matrix does not produce cyan color");

    static_assert((r601FullMatrix * GLfloatColor(0,   128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::black,   1.5f / 255.f), "r.610 full matrix does not produce black color");
    static_assert((r601FullMatrix * GLfloatColor(255, 128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::white,   1.5f / 255.f), "r.610 full matrix does not produce white color");
    static_assert((r601FullMatrix * GLfloatColor(76,  85,  255, 255)).isApproximatelyEqualTo(GLfloatColors::red,     1.5f / 255.f), "r.610 full matrix does not produce red color");
    static_assert((r601FullMatrix * GLfloatColor(150, 44,  21,  255)).isApproximatelyEqualTo(GLfloatColors::green,   1.5f / 255.f), "r.610 full matrix does not produce green color");
    static_assert((r601FullMatrix * GLfloatColor(29,  255, 107, 255)).isApproximatelyEqualTo(GLfloatColors::blue,    1.5f / 255.f), "r.610 full matrix does not produce blue color");
    static_assert((r601FullMatrix * GLfloatColor(226, 0,   149, 255)).isApproximatelyEqualTo(GLfloatColors::yellow,  1.5f / 255.f), "r.610 full matrix does not produce yellow color");
    static_assert((r601FullMatrix * GLfloatColor(105, 212, 235, 255)).isApproximatelyEqualTo(GLfloatColors::magenta, 1.5f / 255.f), "r.610 full matrix does not produce magenta color");
    static_assert((r601FullMatrix * GLfloatColor(179, 171, 1,   255)).isApproximatelyEqualTo(GLfloatColors::cyan,    1.5f / 255.f), "r.610 full matrix does not produce cyan color");

    // Matrices are derived from the components in the ITU R.709 rev 2 specification
    // https://www.itu.int/rec/R-REC-BT.709-2-199510-S
    constexpr static YCbCrMatrix r709VideoMatrix { PixelRange::Video, 0.0722, 0.2126 };
    constexpr static YCbCrMatrix r709FullMatrix { PixelRange::Full, 0.0722, 0.2126 };

    static_assert((r709VideoMatrix * GLfloatColor(16,  128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::black,   1.5f / 255.f), "r.709 video matrix does not produce black color");
    static_assert((r709VideoMatrix * GLfloatColor(235, 128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::white,   1.5f / 255.f), "r.709 video matrix does not produce white color");
    static_assert((r709VideoMatrix * GLfloatColor(63,  102, 240, 255)).isApproximatelyEqualTo(GLfloatColors::red,     1.5f / 255.f), "r.709 video matrix does not produce red color");
    static_assert((r709VideoMatrix * GLfloatColor(173, 42,  26,  255)).isApproximatelyEqualTo(GLfloatColors::green,   1.5f / 255.f), "r.709 video matrix does not produce green color");
    static_assert((r709VideoMatrix * GLfloatColor(32,  240, 118, 255)).isApproximatelyEqualTo(GLfloatColors::blue,    1.5f / 255.f), "r.709 video matrix does not produce blue color");
    static_assert((r709VideoMatrix * GLfloatColor(219, 16,  138, 255)).isApproximatelyEqualTo(GLfloatColors::yellow,  1.5f / 255.f), "r.709 video matrix does not produce yellow color");
    static_assert((r709VideoMatrix * GLfloatColor(78,  214, 230, 255)).isApproximatelyEqualTo(GLfloatColors::magenta, 1.5f / 255.f), "r.709 video matrix does not produce magenta color");
    static_assert((r709VideoMatrix * GLfloatColor(188, 154, 16,  255)).isApproximatelyEqualTo(GLfloatColors::cyan,    1.5f / 255.f), "r.709 video matrix does not produce cyan color");

    static_assert((r709FullMatrix * GLfloatColor(0,   128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::black,   1.5f / 255.f), "r.709 full matrix does not produce black color");
    static_assert((r709FullMatrix * GLfloatColor(255, 128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::white,   1.5f / 255.f), "r.709 full matrix does not produce white color");
    static_assert((r709FullMatrix * GLfloatColor(54,  99,  256, 255)).isApproximatelyEqualTo(GLfloatColors::red,     1.5f / 255.f), "r.709 full matrix does not produce red color");
    static_assert((r709FullMatrix * GLfloatColor(182, 30,  12,  255)).isApproximatelyEqualTo(GLfloatColors::green,   1.5f / 255.f), "r.709 full matrix does not produce green color");
    static_assert((r709FullMatrix * GLfloatColor(18,  256, 116, 255)).isApproximatelyEqualTo(GLfloatColors::blue,    1.5f / 255.f), "r.709 full matrix does not produce blue color");
    static_assert((r709FullMatrix * GLfloatColor(237, 1,   140, 255)).isApproximatelyEqualTo(GLfloatColors::yellow,  1.5f / 255.f), "r.709 full matrix does not produce yellow color");
    static_assert((r709FullMatrix * GLfloatColor(73,  226, 244, 255)).isApproximatelyEqualTo(GLfloatColors::magenta, 1.5f / 255.f), "r.709 full matrix does not produce magenta color");
    static_assert((r709FullMatrix * GLfloatColor(201, 157, 1,   255)).isApproximatelyEqualTo(GLfloatColors::cyan,    1.5f / 255.f), "r.709 full matrix does not produce cyan color");

    // Matrices are derived from the components in the ITU-R BT.2020-2 specification
    // https://www.itu.int/rec/R-REC-BT.2020
    constexpr static YCbCrMatrix bt2020VideoMatrix { PixelRange::Video, 0.0593, 0.2627 };
    constexpr static YCbCrMatrix bt2020FullMatrix { PixelRange::Full, 0.0593, 0.2627 };

    static_assert((bt2020VideoMatrix * GLfloatColor(16,  128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::black,   1.5f / 255.f), "bt.2020 video matrix does not produce black color");
    static_assert((bt2020VideoMatrix * GLfloatColor(235, 128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::white,   1.5f / 255.f), "bt.2020 video matrix does not produce white color");
    static_assert((bt2020VideoMatrix * GLfloatColor(74,  97,  240, 255)).isApproximatelyEqualTo(GLfloatColors::red,     1.5f / 255.f), "bt.2020 video matrix does not produce red color");
    static_assert((bt2020VideoMatrix * GLfloatColor(164, 47,  25,  255)).isApproximatelyEqualTo(GLfloatColors::green,   1.5f / 255.f), "bt.2020 video matrix does not produce green color");
    static_assert((bt2020VideoMatrix * GLfloatColor(29,  240, 119, 255)).isApproximatelyEqualTo(GLfloatColors::blue,    1.5f / 255.f), "bt.2020 video matrix does not produce blue color");
    static_assert((bt2020VideoMatrix * GLfloatColor(222, 16,  137, 255)).isApproximatelyEqualTo(GLfloatColors::yellow,  1.5f / 255.f), "bt.2020 video matrix does not produce yellow color");
    static_assert((bt2020VideoMatrix * GLfloatColor(87,  209, 231, 255)).isApproximatelyEqualTo(GLfloatColors::magenta, 1.5f / 255.f), "bt.2020 video matrix does not produce magenta color");
    static_assert((bt2020VideoMatrix * GLfloatColor(177, 159, 16,  255)).isApproximatelyEqualTo(GLfloatColors::cyan,    1.5f / 255.f), "bt.2020 video matrix does not produce cyan color");

    static_assert((bt2020FullMatrix * GLfloatColor(0,   128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::black,   1.5f / 255.f), "bt.2020 full matrix does not produce black color");
    static_assert((bt2020FullMatrix * GLfloatColor(255, 128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::white,   1.5f / 255.f), "bt.2020 full matrix does not produce white color");
    static_assert((bt2020FullMatrix * GLfloatColor(67,  92,  256, 255)).isApproximatelyEqualTo(GLfloatColors::red,     1.5f / 255.f), "bt.2020 full matrix does not produce red color");
    static_assert((bt2020FullMatrix * GLfloatColor(173, 36,  11,  255)).isApproximatelyEqualTo(GLfloatColors::green,   1.5f / 255.f), "bt.2020 full matrix does not produce green color");
    static_assert((bt2020FullMatrix * GLfloatColor(15,  256, 118, 255)).isApproximatelyEqualTo(GLfloatColors::blue,    1.5f / 255.f), "bt.2020 full matrix does not produce blue color");
    static_assert((bt2020FullMatrix * GLfloatColor(240, 0,   138, 255)).isApproximatelyEqualTo(GLfloatColors::yellow,  1.5f / 255.f), "bt.2020 full matrix does not produce yellow color");
    static_assert((bt2020FullMatrix * GLfloatColor(82,  220, 245, 255)).isApproximatelyEqualTo(GLfloatColors::magenta, 1.5f / 255.f), "bt.2020 full matrix does not produce magenta color");
    static_assert((bt2020FullMatrix * GLfloatColor(188, 164, 1,   255)).isApproximatelyEqualTo(GLfloatColors::cyan,    1.5f / 255.f), "bt.2020 full matrix does not produce cyan color");

    // Matrices are derived from the components in the SMPTE 240M-1999 specification
    // http://ieeexplore.ieee.org/document/7291461/
    constexpr static YCbCrMatrix smpte240MVideoMatrix { PixelRange::Video, 0.087, 0.212 };
    constexpr static YCbCrMatrix smpte240MFullMatrix { PixelRange::Full, 0.087, 0.212 };

    static_assert((smpte240MVideoMatrix * GLfloatColor(16,  128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::black,   1.5f / 255.f), "SMPTE 240M video matrix does not produce black color");
    static_assert((smpte240MVideoMatrix * GLfloatColor(235, 128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::white,   1.5f / 255.f), "SMPTE 240M video matrix does not produce white color");
    static_assert((smpte240MVideoMatrix * GLfloatColor(62,  102, 240, 255)).isApproximatelyEqualTo(GLfloatColors::red,     1.5f / 255.f), "SMPTE 240M video matrix does not produce red color");
    static_assert((smpte240MVideoMatrix * GLfloatColor(170, 42,  28,  255)).isApproximatelyEqualTo(GLfloatColors::green,   1.5f / 255.f), "SMPTE 240M video matrix does not produce green color");
    static_assert((smpte240MVideoMatrix * GLfloatColor(35,  240, 116, 255)).isApproximatelyEqualTo(GLfloatColors::blue,    1.5f / 255.f), "SMPTE 240M video matrix does not produce blue color");
    static_assert((smpte240MVideoMatrix * GLfloatColor(216, 16,  140, 255)).isApproximatelyEqualTo(GLfloatColors::yellow,  1.5f / 255.f), "SMPTE 240M video matrix does not produce yellow color");
    static_assert((smpte240MVideoMatrix * GLfloatColor(81,  214, 228, 255)).isApproximatelyEqualTo(GLfloatColors::magenta, 1.5f / 255.f), "SMPTE 240M video matrix does not produce magenta color");
    static_assert((smpte240MVideoMatrix * GLfloatColor(189, 154, 16,  255)).isApproximatelyEqualTo(GLfloatColors::cyan,    1.5f / 255.f), "SMPTE 240M video matrix does not produce cyan color");

    static_assert((smpte240MFullMatrix * GLfloatColor(0,   128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::black,   1.5f / 255.f), "SMPTE 240M full matrix does not produce black color");
    static_assert((smpte240MFullMatrix * GLfloatColor(255, 128, 128, 255)).isApproximatelyEqualTo(GLfloatColors::white,   1.5f / 255.f), "SMPTE 240M full matrix does not produce white color");
    static_assert((smpte240MFullMatrix * GLfloatColor(54,  98,  256, 255)).isApproximatelyEqualTo(GLfloatColors::red,     1.5f / 255.f), "SMPTE 240M full matrix does not produce red color");
    static_assert((smpte240MFullMatrix * GLfloatColor(179, 30,  15,  255)).isApproximatelyEqualTo(GLfloatColors::green,   1.5f / 255.f), "SMPTE 240M full matrix does not produce green color");
    static_assert((smpte240MFullMatrix * GLfloatColor(22,  256, 114, 255)).isApproximatelyEqualTo(GLfloatColors::blue,    1.5f / 255.f), "SMPTE 240M full matrix does not produce blue color");
    static_assert((smpte240MFullMatrix * GLfloatColor(233, 1,   142, 255)).isApproximatelyEqualTo(GLfloatColors::yellow,  1.5f / 255.f), "SMPTE 240M full matrix does not produce yellow color");
    static_assert((smpte240MFullMatrix * GLfloatColor(76,  226, 241, 255)).isApproximatelyEqualTo(GLfloatColors::magenta, 1.5f / 255.f), "SMPTE 240M full matrix does not produce magenta color");
    static_assert((smpte240MFullMatrix * GLfloatColor(201, 158, 1,   255)).isApproximatelyEqualTo(GLfloatColors::cyan,    1.5f / 255.f), "SMPTE 240M full matrix does not produce cyan color");

    dispatch_once(&onceToken, ^{
        matrices.get().emplace(MapKey(PixelRange::Video, TransferFunctionCV::kITU_R_601_4), r601VideoMatrix);
        matrices.get().emplace(MapKey(PixelRange::Full, TransferFunctionCV::kITU_R_601_4), r601FullMatrix);
        matrices.get().emplace(MapKey(PixelRange::Video, TransferFunctionCV::kITU_R_709_2), r709VideoMatrix);
        matrices.get().emplace(MapKey(PixelRange::Full, TransferFunctionCV::kITU_R_709_2), r709FullMatrix);
        matrices.get().emplace(MapKey(PixelRange::Video, TransferFunctionCV::kITU_R_2020), bt2020VideoMatrix);
        matrices.get().emplace(MapKey(PixelRange::Full, TransferFunctionCV::kITU_R_2020), bt2020FullMatrix);
        matrices.get().emplace(MapKey(PixelRange::Video, TransferFunctionCV::kSMPTE_240M_1995), smpte240MVideoMatrix);
        matrices.get().emplace(MapKey(PixelRange::Full, TransferFunctionCV::kSMPTE_240M_1995), smpte240MFullMatrix);
    });

    // We should never be asked to handle a Pixel Format whose range value is unknown.
    ASSERT(range != PixelRange::Unknown);
    if (range == PixelRange::Unknown)
        range = PixelRange::Full;

    auto iterator = matrices.get().find({range, transferFunction});

    // Assume unknown transfer functions are r.601:
    if (iterator == matrices.get().end())
        iterator = matrices.get().find({range, TransferFunctionCV::kITU_R_601_4});

    ASSERT(iterator != matrices.get().end());
    return iterator->second;
}

VideoTextureCopierCV::VideoTextureCopierCV(GraphicsContextGLOpenGL& context)
    : m_sharedContext(context)
    , m_context(GraphicsContextGLOpenGL::createShared(context))
    , m_framebuffer(m_context->createFramebuffer())
{
}

VideoTextureCopierCV::~VideoTextureCopierCV()
{
    if (m_vertexBuffer)
        m_context->deleteBuffer(m_vertexBuffer);
    if (m_program)
        m_context->deleteProgram(m_program);
    if (m_yuvVertexBuffer)
        m_context->deleteBuffer(m_yuvVertexBuffer);
    if (m_yuvProgram)
        m_context->deleteProgram(m_yuvProgram);
    m_context->deleteFramebuffer(m_framebuffer);
}

#if !LOG_DISABLED
using StringMap = StdMap<uint32_t, const char*>;
#define STRINGIFY_PAIR(e) e, #e
static StringMap& enumToStringMap()
{
    static NeverDestroyed<StringMap> map;
    if (map.get().empty()) {
        StringMap stringMap;
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RGB));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RGBA));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::LUMINANCE_ALPHA));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::LUMINANCE));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::ALPHA));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::R8));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::R16F));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::R32F));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::R8UI));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::R8I));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::R16UI));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::R16I));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::R32UI));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::R32I));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RG8));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RG16F));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RG32F));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RG8UI));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RG8I));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RG16UI));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RG16I));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RG32UI));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RG32I));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RGB8));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::SRGB8));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RGBA8));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::SRGB8_ALPHA8));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RGBA4));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RGB10_A2));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::DEPTH_COMPONENT16));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::DEPTH_COMPONENT24));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::DEPTH_COMPONENT32F));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::DEPTH24_STENCIL8));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::DEPTH32F_STENCIL8));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RGB));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RGBA));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::LUMINANCE_ALPHA));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::LUMINANCE));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::ALPHA));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RED));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::RG_INTEGER));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::DEPTH_STENCIL));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::UNSIGNED_BYTE));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::UNSIGNED_SHORT_5_6_5));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::UNSIGNED_SHORT_4_4_4_4));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::UNSIGNED_SHORT_5_5_5_1));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::BYTE));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::HALF_FLOAT));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::FLOAT));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::UNSIGNED_SHORT));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::SHORT));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::UNSIGNED_INT));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::INT));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::UNSIGNED_INT_2_10_10_10_REV));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::UNSIGNED_INT_24_8));
        map.get().emplace(STRINGIFY_PAIR(GraphicsContextGL::FLOAT_32_UNSIGNED_INT_24_8_REV));

#if USE(OPENGL_ES)
        map.get().emplace(STRINGIFY_PAIR(GL_RED_INTEGER));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB_INTEGER));
        map.get().emplace(STRINGIFY_PAIR(GL_RG8_SNORM));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB565));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB8_SNORM));
        map.get().emplace(STRINGIFY_PAIR(GL_R11F_G11F_B10F));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB9_E5));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB16F));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB32F));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB8UI));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB8I));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB16UI));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB16I));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB32UI));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB32I));
        map.get().emplace(STRINGIFY_PAIR(GL_RGBA8_SNORM));
        map.get().emplace(STRINGIFY_PAIR(GL_RGBA16F));
        map.get().emplace(STRINGIFY_PAIR(GL_RGBA32F));
        map.get().emplace(STRINGIFY_PAIR(GL_RGBA8UI));
        map.get().emplace(STRINGIFY_PAIR(GL_RGBA8I));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB10_A2UI));
        map.get().emplace(STRINGIFY_PAIR(GL_RGBA16UI));
        map.get().emplace(STRINGIFY_PAIR(GL_RGBA16I));
        map.get().emplace(STRINGIFY_PAIR(GL_RGBA32I));
        map.get().emplace(STRINGIFY_PAIR(GL_RGBA32UI));
        map.get().emplace(STRINGIFY_PAIR(GL_RGB5_A1));
        map.get().emplace(STRINGIFY_PAIR(GL_RG));
        map.get().emplace(STRINGIFY_PAIR(GL_RGBA_INTEGER));
        map.get().emplace(STRINGIFY_PAIR(GL_DEPTH_COMPONENT));
        map.get().emplace(STRINGIFY_PAIR(GL_UNSIGNED_INT_10F_11F_11F_REV));
        map.get().emplace(STRINGIFY_PAIR(GL_UNSIGNED_INT_5_9_9_9_REV));
#endif
    }
    return map.get();
}

#endif

bool VideoTextureCopierCV::initializeContextObjects()
{
    StringBuilder vertexShaderSource;
    vertexShaderSource.appendLiteral("attribute vec4 a_position;\n");
    vertexShaderSource.appendLiteral("uniform int u_flipY;\n");
    vertexShaderSource.appendLiteral("varying vec2 v_texturePosition;\n");
    vertexShaderSource.appendLiteral("void main() {\n");
    vertexShaderSource.appendLiteral("    v_texturePosition = vec2((a_position.x + 1.0) / 2.0, (a_position.y + 1.0) / 2.0);\n");
    vertexShaderSource.appendLiteral("    if (u_flipY == 1) {\n");
    vertexShaderSource.appendLiteral("        v_texturePosition.y = 1.0 - v_texturePosition.y;\n");
    vertexShaderSource.appendLiteral("    }\n");
    vertexShaderSource.appendLiteral("    gl_Position = a_position;\n");
    vertexShaderSource.appendLiteral("}\n");

    PlatformGLObject vertexShader = m_context->createShader(GraphicsContextGL::VERTEX_SHADER);
    m_context->shaderSource(vertexShader, vertexShaderSource.toString());
    m_context->compileShaderDirect(vertexShader);

    GCGLint value = 0;
    m_context->getShaderiv(vertexShader, GraphicsContextGL::COMPILE_STATUS, &value);
    if (!value) {
        LOG(WebGL, "VideoTextureCopierCV::copyVideoTextureToPlatformTexture(%p) - Vertex shader failed to compile.", this);
        m_context->deleteShader(vertexShader);
        return false;
    }

    StringBuilder fragmentShaderSource;

#if USE(OPENGL_ES) || USE(ANGLE)
    fragmentShaderSource.appendLiteral("precision mediump float;\n");
#endif
#if USE(OPENGL_ES) || (USE(ANGLE) && PLATFORM(IOS_FAMILY))
    fragmentShaderSource.appendLiteral("uniform sampler2D u_texture;\n");
#elif USE(OPENGL) || (USE(ANGLE) && !PLATFORM(IOS_FAMILY))
    fragmentShaderSource.appendLiteral("uniform sampler2DRect u_texture;\n");
#else
#error Unsupported configuration
#endif
    fragmentShaderSource.appendLiteral("varying vec2 v_texturePosition;\n");
    fragmentShaderSource.appendLiteral("uniform int u_premultiply;\n");
    fragmentShaderSource.appendLiteral("uniform vec2 u_textureDimensions;\n");
    fragmentShaderSource.appendLiteral("uniform int u_swapColorChannels;\n");
    fragmentShaderSource.appendLiteral("void main() {\n");
    fragmentShaderSource.appendLiteral("    vec2 texPos = vec2(v_texturePosition.x * u_textureDimensions.x, v_texturePosition.y * u_textureDimensions.y);\n");
#if USE(OPENGL_ES) || (USE(ANGLE) && PLATFORM(IOS_FAMILY))
    fragmentShaderSource.appendLiteral("    vec4 color = texture2D(u_texture, texPos);\n");
#elif USE(OPENGL) || (USE(ANGLE) && !PLATFORM(IOS_FAMILY))
    fragmentShaderSource.appendLiteral("    vec4 color = texture2DRect(u_texture, texPos);\n");
#else
#error Unsupported configuration
#endif
    fragmentShaderSource.appendLiteral("    if (u_swapColorChannels == 1) {\n");
    fragmentShaderSource.appendLiteral("        color.rgba = color.bgra;\n");
    fragmentShaderSource.appendLiteral("    }\n");
    fragmentShaderSource.appendLiteral("    if (u_premultiply == 1) {\n");
    fragmentShaderSource.appendLiteral("        gl_FragColor = vec4(color.r * color.a, color.g * color.a, color.b * color.a, color.a);\n");
    fragmentShaderSource.appendLiteral("    } else {\n");
    fragmentShaderSource.appendLiteral("        gl_FragColor = color;\n");
    fragmentShaderSource.appendLiteral("    }\n");
    fragmentShaderSource.appendLiteral("}\n");

    PlatformGLObject fragmentShader = m_context->createShader(GraphicsContextGL::FRAGMENT_SHADER);
    m_context->shaderSource(fragmentShader, fragmentShaderSource.toString());
    m_context->compileShaderDirect(fragmentShader);

    m_context->getShaderiv(fragmentShader, GraphicsContextGL::COMPILE_STATUS, &value);
    if (!value) {
        LOG(WebGL, "VideoTextureCopierCV::copyVideoTextureToPlatformTexture(%p) - Fragment shader failed to compile.", this);
        m_context->deleteShader(vertexShader);
        m_context->deleteShader(fragmentShader);
        return false;
    }

    m_program = m_context->createProgram();
    m_context->attachShader(m_program, vertexShader);
    m_context->attachShader(m_program, fragmentShader);
    m_context->linkProgram(m_program);

    m_context->getProgramiv(m_program, GraphicsContextGL::LINK_STATUS, &value);
    if (!value) {
        LOG(WebGL, "VideoTextureCopierCV::copyVideoTextureToPlatformTexture(%p) - Program failed to link.", this);
        m_context->deleteShader(vertexShader);
        m_context->deleteShader(fragmentShader);
        m_context->deleteProgram(m_program);
        m_program = 0;
        return false;
    }

    m_textureUniformLocation = m_context->getUniformLocation(m_program, "u_texture"_s);
    m_textureDimensionsUniformLocation = m_context->getUniformLocation(m_program, "u_textureDimensions"_s);
    m_flipYUniformLocation = m_context->getUniformLocation(m_program, "u_flipY"_s);
    m_swapColorChannelsUniformLocation = m_context->getUniformLocation(m_program, "u_swapColorChannels"_s);
    m_premultiplyUniformLocation = m_context->getUniformLocation(m_program, "u_premultiply"_s);
    m_positionAttributeLocation = m_context->getAttribLocationDirect(m_program, "a_position"_s);

    m_context->detachShader(m_program, vertexShader);
    m_context->detachShader(m_program, fragmentShader);
    m_context->deleteShader(vertexShader);
    m_context->deleteShader(fragmentShader);

    LOG(WebGL, "Uniform and Attribute locations: u_texture = %d, u_textureDimensions = %d, u_flipY = %d, u_premultiply = %d, a_position = %d", m_textureUniformLocation, m_textureDimensionsUniformLocation, m_flipYUniformLocation, m_premultiplyUniformLocation, m_positionAttributeLocation);
    m_context->enableVertexAttribArray(m_positionAttributeLocation);

    m_vertexBuffer = m_context->createBuffer();
    float vertices[12] = { -1, -1, 1, -1, 1, 1, 1, 1, -1, 1, -1, -1 };

    m_context->bindBuffer(GraphicsContextGL::ARRAY_BUFFER, m_vertexBuffer);
    m_context->bufferData(GraphicsContextGL::ARRAY_BUFFER, sizeof(float) * 12, vertices, GraphicsContextGL::STATIC_DRAW);

    return true;
}

bool VideoTextureCopierCV::initializeUVContextObjects()
{
    String vertexShaderSource {
        "attribute vec2 a_position;\n"
        "uniform vec2 u_yTextureSize;\n"
        "uniform vec2 u_uvTextureSize;\n"
        "uniform int u_flipY;\n"
        "varying vec2 v_yTextureCoordinate;\n"
        "varying vec2 v_uvTextureCoordinate;\n"
        "void main() {\n"
        "   gl_Position = vec4(a_position, 0, 1.0);\n"
        "   vec2 normalizedPosition = a_position * .5 + .5;\n"
        "   if (u_flipY == 1) {\n"
        "       normalizedPosition.y = 1.0 - normalizedPosition.y;\n"
        "   }\n"
#if USE(OPENGL_ES) || (USE(ANGLE) && PLATFORM(IOS_FAMILY))
        "   v_yTextureCoordinate = normalizedPosition;\n"
        "   v_uvTextureCoordinate = normalizedPosition;\n"
#elif USE(OPENGL) || (USE(ANGLE) && !PLATFORM(IOS_FAMILY))
        "   v_yTextureCoordinate = normalizedPosition * u_yTextureSize;\n"
        "   v_uvTextureCoordinate = normalizedPosition * u_uvTextureSize;\n"
#else
#error Unsupported configuration
#endif
        "}\n"_s
    };

    PlatformGLObject vertexShader = m_context->createShader(GraphicsContextGL::VERTEX_SHADER);
    m_context->shaderSource(vertexShader, vertexShaderSource);
    m_context->compileShaderDirect(vertexShader);

    GCGLint status = 0;
    m_context->getShaderiv(vertexShader, GraphicsContextGL::COMPILE_STATUS, &status);
    if (!status) {
        LOG(WebGL, "VideoTextureCopierCV::initializeUVContextObjects(%p) - Vertex shader failed to compile.", this);
        m_context->deleteShader(vertexShader);
        return false;
    }

    String fragmentShaderSource {
#if USE(OPENGL_ES) || USE(ANGLE)
        "precision mediump float;\n"
#endif
#if USE(OPENGL_ES) || (USE(ANGLE) && PLATFORM(IOS_FAMILY))
        "#define SAMPLERTYPE sampler2D\n"
        "#define TEXTUREFUNC texture2D\n"
#elif USE(OPENGL) || (USE(ANGLE) && !PLATFORM(IOS_FAMILY))
        "#define SAMPLERTYPE sampler2DRect\n"
        "#define TEXTUREFUNC texture2DRect\n"
#else
#error Unsupported configuration
#endif
        "uniform SAMPLERTYPE u_yTexture;\n"
        "uniform SAMPLERTYPE u_uvTexture;\n"
        "uniform mat4 u_colorMatrix;\n"
        "varying vec2 v_yTextureCoordinate;\n"
        "varying vec2 v_uvTextureCoordinate;\n"
        "void main() {\n"
        "    vec4 yuv;\n"
        "    yuv.r = TEXTUREFUNC(u_yTexture, v_yTextureCoordinate).r;\n"
        "    yuv.gb = TEXTUREFUNC(u_uvTexture, v_uvTextureCoordinate).rg;\n"
        "    yuv.a = 1.0;\n"
        "    gl_FragColor = yuv * u_colorMatrix;\n"
        "}\n"_s
    };

    PlatformGLObject fragmentShader = m_context->createShader(GraphicsContextGL::FRAGMENT_SHADER);
    m_context->shaderSource(fragmentShader, fragmentShaderSource);
    m_context->compileShaderDirect(fragmentShader);

    m_context->getShaderiv(fragmentShader, GraphicsContextGL::COMPILE_STATUS, &status);
    if (!status) {
        LOG(WebGL, "VideoTextureCopierCV::initializeUVContextObjects(%p) - Fragment shader failed to compile.", this);
        m_context->deleteShader(vertexShader);
        m_context->deleteShader(fragmentShader);
        return false;
    }

    m_yuvProgram = m_context->createProgram();
    m_context->attachShader(m_yuvProgram, vertexShader);
    m_context->attachShader(m_yuvProgram, fragmentShader);
    m_context->linkProgram(m_yuvProgram);

    m_context->getProgramiv(m_yuvProgram, GraphicsContextGL::LINK_STATUS, &status);
    if (!status) {
        LOG(WebGL, "VideoTextureCopierCV::initializeUVContextObjects(%p) - Program failed to link.", this);
        m_context->deleteShader(vertexShader);
        m_context->deleteShader(fragmentShader);
        m_context->deleteProgram(m_yuvProgram);
        m_yuvProgram = 0;
        return false;
    }

    m_yTextureUniformLocation = m_context->getUniformLocation(m_yuvProgram, "u_yTexture"_s);
    m_uvTextureUniformLocation = m_context->getUniformLocation(m_yuvProgram, "u_uvTexture"_s);
    m_colorMatrixUniformLocation = m_context->getUniformLocation(m_yuvProgram, "u_colorMatrix"_s);
    m_yuvFlipYUniformLocation = m_context->getUniformLocation(m_yuvProgram, "u_flipY"_s);
    m_yTextureSizeUniformLocation = m_context->getUniformLocation(m_yuvProgram, "u_yTextureSize"_s);
    m_uvTextureSizeUniformLocation = m_context->getUniformLocation(m_yuvProgram, "u_uvTextureSize"_s);
    m_yuvPositionAttributeLocation = m_context->getAttribLocationDirect(m_yuvProgram, "a_position"_s);

    m_context->detachShader(m_yuvProgram, vertexShader);
    m_context->detachShader(m_yuvProgram, fragmentShader);
    m_context->deleteShader(vertexShader);
    m_context->deleteShader(fragmentShader);

    m_yuvVertexBuffer = m_context->createBuffer();
    float vertices[12] = { -1, -1, 1, -1, 1, 1, 1, 1, -1, 1, -1, -1 };

    m_context->bindBuffer(GraphicsContextGL::ARRAY_BUFFER, m_yuvVertexBuffer);
    m_context->bufferData(GraphicsContextGL::ARRAY_BUFFER, sizeof(vertices), vertices, GraphicsContextGL::STATIC_DRAW);
    m_context->enableVertexAttribArray(m_yuvPositionAttributeLocation);
    m_context->vertexAttribPointer(m_yuvPositionAttributeLocation, 2, GraphicsContextGL::FLOAT, false, 0, 0);

    return true;
}

#if USE(ANGLE)
void* VideoTextureCopierCV::attachIOSurfaceToTexture(GCGLenum target, GCGLenum internalFormat, GCGLsizei width, GCGLsizei height, GCGLenum type, IOSurfaceRef surface, GCGLuint plane)
{
    auto display = m_context->platformDisplay();
    EGLint eglTextureTarget = 0;

    if (target == GraphicsContextGL::TEXTURE_RECTANGLE_ARB)
        eglTextureTarget = EGL_TEXTURE_RECTANGLE_ANGLE;
    else if (target == GraphicsContextGL::TEXTURE_2D)
        eglTextureTarget = EGL_TEXTURE_2D;
    else {
        LOG(WebGL, "Unknown texture target %d.", static_cast<int>(target));
        return nullptr;
    }
    if (eglTextureTarget != GraphicsContextGL::EGLIOSurfaceTextureTarget) {
        LOG(WebGL, "Mismatch in EGL texture target %d.", static_cast<int>(target));
        return nullptr;
    }

    const EGLint surfaceAttributes[] = {
        EGL_WIDTH, width,
        EGL_HEIGHT, height,
        EGL_IOSURFACE_PLANE_ANGLE, static_cast<EGLint>(plane),
        EGL_TEXTURE_TARGET, static_cast<EGLint>(eglTextureTarget),
        EGL_TEXTURE_INTERNAL_FORMAT_ANGLE, static_cast<EGLint>(internalFormat),
        EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
        EGL_TEXTURE_TYPE_ANGLE, static_cast<EGLint>(type),
        // Only has an effect on the iOS Simulator.
        EGL_IOSURFACE_USAGE_HINT_ANGLE, EGL_IOSURFACE_READ_HINT_ANGLE,
        EGL_NONE, EGL_NONE
    };
    EGLSurface pbuffer = EGL_CreatePbufferFromClientBuffer(display, EGL_IOSURFACE_ANGLE, surface, m_context->platformConfig(), surfaceAttributes);
    if (!pbuffer)
        return nullptr;
    if (!EGL_BindTexImage(display, pbuffer, EGL_BACK_BUFFER)) {
        EGL_DestroySurface(display, pbuffer);
        return nullptr;
    }
    return pbuffer;
}

void VideoTextureCopierCV::detachIOSurfaceFromTexture(void* handle)
{
    auto display = m_context->platformDisplay();
    EGL_ReleaseTexImage(display, handle, EGL_BACK_BUFFER);
    EGL_DestroySurface(display, handle);
}
#endif

bool VideoTextureCopierCV::copyImageToPlatformTexture(CVPixelBufferRef image, size_t width, size_t height, PlatformGLObject outputTexture, GCGLenum outputTarget, GCGLint level, GCGLenum internalFormat, GCGLenum format, GCGLenum type, bool premultiplyAlpha, bool flipY)
{
    // CVOpenGLTextureCache seems to be disabled since the deprecation of
    // OpenGL. To avoid porting unused code to the ANGLE code paths, remove it.
#if USE(ANGLE)
    UNUSED_PARAM(outputTarget);
    UNUSED_PARAM(premultiplyAlpha);
#else
    if (!m_textureCache) {
        m_textureCache = TextureCacheCV::create(m_context);
        if (!m_textureCache)
            return false;
    }

    if (auto texture = m_textureCache->textureFromImage(image, outputTarget, level, internalFormat, format, type)) {
        bool swapColorChannels = false;
#if USE(OPENGL_ES)
        // FIXME: Remove this workaround once rdar://problem/35834388 is fixed.
        swapColorChannels = CVPixelBufferGetPixelFormatType(image) == kCVPixelFormatType_32BGRA;
#endif
        return copyVideoTextureToPlatformTexture(texture.get(), width, height, outputTexture, outputTarget, level, internalFormat, format, type, premultiplyAlpha, flipY, swapColorChannels);
    }
#endif // USE(ANGLE)

    // FIXME: This currently only supports '420v' and '420f' pixel formats. Investigate supporting more pixel formats.
    OSType pixelFormat = CVPixelBufferGetPixelFormatType(image);
    if (pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange && pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarFullRange) {
        LOG(WebGL, "VideoTextureCopierCV::copyVideoTextureToPlatformTexture(%p) - Asked to copy an unsupported pixel format ('%s').", this, FourCC(pixelFormat).toString().utf8().data());
        return false;
    }

    IOSurfaceRef surface = CVPixelBufferGetIOSurface(image);
    if (!surface)
        return false;

    auto newSurfaceSeed = IOSurfaceGetSeed(surface);
    if (flipY == m_lastFlipY
        && surface == m_lastSurface 
        && newSurfaceSeed == m_lastSurfaceSeed
        && lastTextureSeed(outputTexture) == m_context->textureSeed(outputTexture)) {
        // If the texture hasn't been modified since the last time we copied to it, and the
        // image hasn't been modified since the last time it was copied, this is a no-op.
        return true;
    }

    if (!m_yuvProgram) {
        if (!initializeUVContextObjects()) {
            LOG(WebGL, "VideoTextureCopierCV::copyVideoTextureToPlatformTexture(%p) - Unable to initialize OpenGL context objects.", this);
            return false;
        }
    }

    m_context->bindFramebuffer(GraphicsContextGL::FRAMEBUFFER, m_framebuffer);

    // Allocate memory for the output texture.
    m_context->bindTexture(GraphicsContextGL::TEXTURE_2D, outputTexture);
    m_context->texParameteri(GraphicsContextGL::TEXTURE_2D, GraphicsContextGL::TEXTURE_MAG_FILTER, GraphicsContextGL::LINEAR);
    m_context->texParameteri(GraphicsContextGL::TEXTURE_2D, GraphicsContextGL::TEXTURE_MIN_FILTER, GraphicsContextGL::LINEAR);
    m_context->texParameteri(GraphicsContextGL::TEXTURE_2D, GraphicsContextGL::TEXTURE_WRAP_S, GraphicsContextGL::CLAMP_TO_EDGE);
    m_context->texParameteri(GraphicsContextGL::TEXTURE_2D, GraphicsContextGL::TEXTURE_WRAP_T, GraphicsContextGL::CLAMP_TO_EDGE);
    m_context->texImage2DDirect(GraphicsContextGL::TEXTURE_2D, level, internalFormat, width, height, 0, format, type, nullptr);

    m_context->framebufferTexture2D(GraphicsContextGL::FRAMEBUFFER, GraphicsContextGL::COLOR_ATTACHMENT0, GraphicsContextGL::TEXTURE_2D, outputTexture, level);
    GCGLenum status = m_context->checkFramebufferStatus(GraphicsContextGL::FRAMEBUFFER);
    if (status != GraphicsContextGL::FRAMEBUFFER_COMPLETE) {
        LOG(WebGL, "VideoTextureCopierCV::copyVideoTextureToPlatformTexture(%p) - Unable to create framebuffer for outputTexture.", this);
        return false;
    }
    m_context->bindTexture(GraphicsContextGL::TEXTURE_2D, 0);

    m_context->useProgram(m_yuvProgram);
    m_context->viewport(0, 0, width, height);

    // Bind and set up the textures for the video source.
    auto yPlaneWidth = IOSurfaceGetWidthOfPlane(surface, 0);
    auto yPlaneHeight = IOSurfaceGetHeightOfPlane(surface, 0);
    auto uvPlaneWidth = IOSurfaceGetWidthOfPlane(surface, 1);
    auto uvPlaneHeight = IOSurfaceGetHeightOfPlane(surface, 1);

#if USE(OPENGL_ES)
    GCGLenum videoTextureTarget = GraphicsContextGL::TEXTURE_2D;
#elif USE(OPENGL)
    GCGLenum videoTextureTarget = GraphicsContextGL::TEXTURE_RECTANGLE_ARB;
#elif USE(ANGLE)
    GCGLenum videoTextureTarget = GraphicsContextGL::IOSurfaceTextureTarget;
#else
#error Unsupported configuration
#endif
    auto uvTexture = m_context->createTexture();
    m_context->activeTexture(GraphicsContextGL::TEXTURE1);
    m_context->bindTexture(videoTextureTarget, uvTexture);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_MAG_FILTER, GraphicsContextGL::LINEAR);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_MIN_FILTER, GraphicsContextGL::LINEAR);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_WRAP_S, GraphicsContextGL::CLAMP_TO_EDGE);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_WRAP_T, GraphicsContextGL::CLAMP_TO_EDGE);
#if USE(ANGLE)
    auto uvHandle = attachIOSurfaceToTexture(videoTextureTarget, GraphicsContextGL::RG, uvPlaneWidth, uvPlaneHeight, GraphicsContextGL::UNSIGNED_BYTE, surface, 1);
    if (!uvHandle) {
        m_context->deleteTexture(uvTexture);
        return false;
    }
#else
    if (!m_context->texImageIOSurface2D(videoTextureTarget, GraphicsContextGL::RG, uvPlaneWidth, uvPlaneHeight, GraphicsContextGL::RG, GraphicsContextGL::UNSIGNED_BYTE, surface, 1)) {
        m_context->deleteTexture(uvTexture);
        return false;
    }
#endif // USE(ANGLE)

    auto yTexture = m_context->createTexture();
    m_context->activeTexture(GraphicsContextGL::TEXTURE0);
    m_context->bindTexture(videoTextureTarget, yTexture);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_MAG_FILTER, GraphicsContextGL::LINEAR);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_MIN_FILTER, GraphicsContextGL::LINEAR);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_WRAP_S, GraphicsContextGL::CLAMP_TO_EDGE);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_WRAP_T, GraphicsContextGL::CLAMP_TO_EDGE);
#if USE(ANGLE)
    auto yHandle = attachIOSurfaceToTexture(videoTextureTarget, GraphicsContextGL::RED, yPlaneWidth, yPlaneHeight, GraphicsContextGL::UNSIGNED_BYTE, surface, 0);
    if (!yHandle) {
        m_context->deleteTexture(yTexture);
        m_context->deleteTexture(uvTexture);
        return false;
    }
#else
    if (!m_context->texImageIOSurface2D(videoTextureTarget, GraphicsContextGL::LUMINANCE, yPlaneWidth, yPlaneHeight, GraphicsContextGL::LUMINANCE, GraphicsContextGL::UNSIGNED_BYTE, surface, 0)) {
        m_context->deleteTexture(yTexture);
        m_context->deleteTexture(uvTexture);
        return false;
    }
#endif // USE(ANGLE)

    // Configure the drawing parameters.
    m_context->uniform1i(m_yTextureUniformLocation, 0);
    m_context->uniform1i(m_uvTextureUniformLocation, 1);
    m_context->uniform1i(m_yuvFlipYUniformLocation, flipY);
    m_context->uniform2f(m_yTextureSizeUniformLocation, yPlaneWidth, yPlaneHeight);
    m_context->uniform2f(m_uvTextureSizeUniformLocation, uvPlaneWidth, uvPlaneHeight);

    auto range = pixelRangeFromPixelFormat(pixelFormat);
    auto transferFunction = transferFunctionFromString((CFStringRef)CVBufferGetAttachment(image, kCVImageBufferYCbCrMatrixKey, nil));
    auto& colorMatrix = YCbCrToRGBMatrixForRangeAndTransferFunction(range, transferFunction);
    m_context->uniformMatrix4fv(m_colorMatrixUniformLocation, 1, GL_FALSE, colorMatrix.data());

    // Do the actual drawing.
    m_context->drawArrays(GraphicsContextGL::TRIANGLES, 0, 6);

#if USE(OPENGL_ES) || (USE(ANGLE) && PLATFORM(IOS_FAMILY))
    // flush() must be called here in order to re-synchronize the output texture's contents across the
    // two EAGL contexts.
    m_context->flush();
#endif

    // Clean-up.
    m_context->deleteTexture(yTexture);
    m_context->deleteTexture(uvTexture);
#if USE(ANGLE)
    detachIOSurfaceFromTexture(yHandle);
    detachIOSurfaceFromTexture(uvHandle);
#endif

    m_lastSurface = surface;
    m_lastSurfaceSeed = newSurfaceSeed;
    m_lastTextureSeed.set(outputTexture, m_context->textureSeed(outputTexture));
    m_lastFlipY = flipY;

    return true;
}

bool VideoTextureCopierCV::copyVideoTextureToPlatformTexture(TextureType inputVideoTexture, size_t width, size_t height, PlatformGLObject outputTexture, GCGLenum outputTarget, GCGLint level, GCGLenum internalFormat, GCGLenum format, GCGLenum type, bool premultiplyAlpha, bool flipY, bool swapColorChannels)
{
    if (!inputVideoTexture)
        return false;

    GLfloat lowerLeft[2] = { 0, 0 };
    GLfloat lowerRight[2] = { 0, 0 };
    GLfloat upperRight[2] = { 0, 0 };
    GLfloat upperLeft[2] = { 0, 0 };
    PlatformGLObject videoTextureName;
    GCGLenum videoTextureTarget;

#if USE(OPENGL_ES)
    videoTextureName = CVOpenGLESTextureGetName(inputVideoTexture);
    videoTextureTarget = CVOpenGLESTextureGetTarget(inputVideoTexture);
    CVOpenGLESTextureGetCleanTexCoords(inputVideoTexture, lowerLeft, lowerRight, upperRight, upperLeft);
#elif USE(OPENGL)
    videoTextureName = CVOpenGLTextureGetName(inputVideoTexture);
    videoTextureTarget = CVOpenGLTextureGetTarget(inputVideoTexture);
    CVOpenGLTextureGetCleanTexCoords(inputVideoTexture, lowerLeft, lowerRight, upperRight, upperLeft);
#elif USE(ANGLE)
    // CVOpenGLTextureCacheCreateTextureFromImage seems to always return
    // kCVReturnPixelBufferNotOpenGLCompatible on desktop macOS now, so this
    // entire code path seems to be unused. Assume the IOSurface path will be
    // taken when using ANGLE.
    UNUSED_PARAM(lowerLeft);
    UNUSED_PARAM(lowerRight);
    UNUSED_PARAM(upperLeft);
    UNUSED_PARAM(upperRight);
    UNUSED_PARAM(width);
    UNUSED_PARAM(height);
    UNUSED_PARAM(outputTexture);
    UNUSED_PARAM(outputTarget);
    UNUSED_PARAM(level);
    UNUSED_PARAM(internalFormat);
    UNUSED_PARAM(format);
    UNUSED_PARAM(type);
    UNUSED_PARAM(premultiplyAlpha);
    UNUSED_PARAM(flipY);
    UNUSED_PARAM(swapColorChannels);
    // FIXME: determine how to access rectangular textures via ANGLE.
    UNIMPLEMENTED();
    return false;
#endif

    if (lowerLeft[1] < upperRight[1])
        flipY = !flipY;

    return copyVideoTextureToPlatformTexture(videoTextureName, videoTextureTarget, width, height, outputTexture, outputTarget, level, internalFormat, format, type, premultiplyAlpha, flipY, swapColorChannels);
}

bool VideoTextureCopierCV::copyVideoTextureToPlatformTexture(PlatformGLObject videoTextureName, GCGLenum videoTextureTarget, size_t width, size_t height, PlatformGLObject outputTexture, GCGLenum outputTarget, GCGLint level, GCGLenum internalFormat, GCGLenum format, GCGLenum type, bool premultiplyAlpha, bool flipY, bool swapColorChannels)
{
    LOG(WebGL, "VideoTextureCopierCV::copyVideoTextureToPlatformTexture(%p) - internalFormat: %s, format: %s, type: %s flipY: %s, premultiplyAlpha: %s", this, enumToStringMap()[internalFormat], enumToStringMap()[format], enumToStringMap()[type], flipY ? "true" : "false", premultiplyAlpha ? "true" : "false");

    if (!m_program) {
        if (!initializeContextObjects()) {
            LOG(WebGL, "VideoTextureCopierCV::copyVideoTextureToPlatformTexture(%p) - Unable to initialize OpenGL context objects.", this);
            return false;
        }
    }

    m_context->bindFramebuffer(GraphicsContextGL::FRAMEBUFFER, m_framebuffer);
    
    // Allocate memory for the output texture.
    m_context->bindTexture(GraphicsContextGL::TEXTURE_2D, outputTexture);
    m_context->texParameteri(GraphicsContextGL::TEXTURE_2D, GraphicsContextGL::TEXTURE_MAG_FILTER, GraphicsContextGL::LINEAR);
    m_context->texParameteri(GraphicsContextGL::TEXTURE_2D, GraphicsContextGL::TEXTURE_MIN_FILTER, GraphicsContextGL::LINEAR);
    m_context->texParameteri(GraphicsContextGL::TEXTURE_2D, GraphicsContextGL::TEXTURE_WRAP_S, GraphicsContextGL::CLAMP_TO_EDGE);
    m_context->texParameteri(GraphicsContextGL::TEXTURE_2D, GraphicsContextGL::TEXTURE_WRAP_T, GraphicsContextGL::CLAMP_TO_EDGE);
    m_context->texImage2DDirect(GraphicsContextGL::TEXTURE_2D, level, internalFormat, width, height, 0, format, type, nullptr);

    m_context->framebufferTexture2D(GraphicsContextGL::FRAMEBUFFER, GraphicsContextGL::COLOR_ATTACHMENT0, GraphicsContextGL::TEXTURE_2D, outputTexture, level);
    GCGLenum status = m_context->checkFramebufferStatus(GraphicsContextGL::FRAMEBUFFER);
    if (status != GraphicsContextGL::FRAMEBUFFER_COMPLETE) {
        LOG(WebGL, "VideoTextureCopierCV::copyVideoTextureToPlatformTexture(%p) - Unable to create framebuffer for outputTexture.", this);
        return false;
    }
    m_context->bindTexture(GraphicsContextGL::TEXTURE_2D, 0);

    m_context->useProgram(m_program);
    m_context->viewport(0, 0, width, height);

    // Bind and set up the texture for the video source.
    m_context->activeTexture(GraphicsContextGL::TEXTURE0);
    m_context->bindTexture(videoTextureTarget, videoTextureName);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_MAG_FILTER, GraphicsContextGL::LINEAR);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_MIN_FILTER, GraphicsContextGL::LINEAR);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_WRAP_S, GraphicsContextGL::CLAMP_TO_EDGE);
    m_context->texParameteri(videoTextureTarget, GraphicsContextGL::TEXTURE_WRAP_T, GraphicsContextGL::CLAMP_TO_EDGE);

    // Configure the drawing parameters.
    m_context->uniform1i(m_textureUniformLocation, 0);
#if USE(OPENGL_ES)
    m_context->uniform2f(m_textureDimensionsUniformLocation, 1, 1);
#else
    m_context->uniform2f(m_textureDimensionsUniformLocation, width, height);
#endif

    m_context->uniform1i(m_flipYUniformLocation, flipY);
    m_context->uniform1i(m_swapColorChannelsUniformLocation, swapColorChannels);
    m_context->uniform1i(m_premultiplyUniformLocation, premultiplyAlpha);

    // Do the actual drawing.
    m_context->enableVertexAttribArray(m_positionAttributeLocation);
    m_context->bindBuffer(GraphicsContextGL::ARRAY_BUFFER, m_vertexBuffer);
    m_context->vertexAttribPointer(m_positionAttributeLocation, 2, GraphicsContextGL::FLOAT, false, 0, 0);
    m_context->drawArrays(GraphicsContextGL::TRIANGLES, 0, 6);

#if USE(OPENGL_ES)
    // flush() must be called here in order to re-synchronize the output texture's contents across the
    // two EAGL contexts.
    m_context->flush();
#endif

    // Clean-up.
    m_context->bindTexture(videoTextureTarget, 0);
    m_context->bindTexture(outputTarget, outputTexture);

    return true;
}

}
