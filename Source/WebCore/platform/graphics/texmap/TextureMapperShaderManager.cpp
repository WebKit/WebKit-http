/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "TextureMapperShaderManager.h"

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)

#include "LengthFunctions.h"
#include "Logging.h"
#include "TextureMapperGL.h"

namespace WebCore {

#ifndef TEXMAP_OPENGL_ES_2
#define OES2_PRECISION_DEFINITIONS \
    "#define lowp\n#define highp\n"
#define OES2_FRAGMENT_SHADER_DEFAULT_PRECISION
#else
#define OES2_PRECISION_DEFINITIONS
#define OES2_FRAGMENT_SHADER_DEFAULT_PRECISION \
    "precision mediump float; \n"
#endif

#define VERTEX_SHADER(src...) OES2_PRECISION_DEFINITIONS#src
#define FRAGMENT_SHADER(src...) OES2_PRECISION_DEFINITIONS\
                                OES2_FRAGMENT_SHADER_DEFAULT_PRECISION\
                                #src

static const char* fragmentShaderSourceOpacityAndMask =
    FRAGMENT_SHADER(
        uniform sampler2D SourceTexture, MaskTexture;
        uniform lowp float Opacity;
        varying highp vec2 OutTexCoordSource, OutTexCoordMask;
        void main(void)
        {
            lowp vec4 color = texture2D(SourceTexture, OutTexCoordSource);
            lowp vec4 maskColor = texture2D(MaskTexture, OutTexCoordMask);
            lowp float fragmentAlpha = Opacity * maskColor.a;
            gl_FragColor = vec4(color.rgb * fragmentAlpha, color.a * fragmentAlpha);
        }
    );

static const char* fragmentShaderSourceRectOpacityAndMask =
    FRAGMENT_SHADER(
        uniform sampler2DRect SourceTexture, MaskTexture;
        uniform lowp float Opacity;
        varying highp vec2 OutTexCoordSource, OutTexCoordMask;
        void main(void)
        {
            lowp vec4 color = texture2DRect(SourceTexture, OutTexCoordSource);
            lowp vec4 maskColor = texture2DRect(MaskTexture, OutTexCoordMask);
            lowp float fragmentAlpha = Opacity * maskColor.a;
            gl_FragColor = vec4(color.rgb * fragmentAlpha, color.a * fragmentAlpha);
        }
    );

static const char* vertexShaderSourceOpacityAndMask =
    VERTEX_SHADER(
        uniform mat4 InMatrix, InSourceMatrix;
        attribute vec4 InVertex;
        varying highp vec2 OutTexCoordSource, OutTexCoordMask;
        void main(void)
        {
            OutTexCoordSource = vec2(InSourceMatrix * InVertex);
            OutTexCoordMask = vec2(InVertex);
            gl_Position = InMatrix * InVertex;
        }
    );

static const char* fragmentShaderSourceSimple =
    FRAGMENT_SHADER(
        uniform sampler2D SourceTexture;
        uniform lowp float Opacity;
        varying highp vec2 OutTexCoordSource;
        void main(void)
        {
            lowp vec4 color = texture2D(SourceTexture, OutTexCoordSource);
            gl_FragColor = vec4(color.rgb * Opacity, color.a * Opacity);
        }
    );

static const char* fragmentShaderSourceRectSimple =
    FRAGMENT_SHADER(
        uniform sampler2DRect SourceTexture;
        uniform lowp float Opacity;
        varying highp vec2 OutTexCoordSource;
        void main(void)
        {
            lowp vec4 color = texture2DRect(SourceTexture, OutTexCoordSource);
            gl_FragColor = vec4(color.rgb * Opacity, color.a * Opacity);
        }
    );

static const char* vertexShaderSourceSimple =
    VERTEX_SHADER(
        uniform mat4 InMatrix, InSourceMatrix;
        attribute vec4 InVertex;
        varying highp vec2 OutTexCoordSource;
        void main(void)
        {
            OutTexCoordSource = vec2(InSourceMatrix * InVertex);
            gl_Position = InMatrix * InVertex;
        }
    );

static const char* vertexShaderSourceSolidColor =
    VERTEX_SHADER(
        uniform mat4 InMatrix;
        attribute vec4 InVertex;
        void main(void)
        {
            gl_Position = InMatrix * InVertex;
        }
    );


static const char* fragmentShaderSourceSolidColor =
    VERTEX_SHADER(
        uniform vec4 Color;
        void main(void)
        {
            gl_FragColor = Color;
        }
    );

PassRefPtr<TextureMapperShaderProgramSolidColor> TextureMapperShaderManager::solidColorProgram()
{
    return static_pointer_cast<TextureMapperShaderProgramSolidColor>(getShaderProgram(SolidColor));
}

PassRefPtr<TextureMapperShaderProgram> TextureMapperShaderManager::getShaderProgram(ShaderType shaderType)
{
    RefPtr<TextureMapperShaderProgram> program;
    if (shaderType == Invalid)
        return program;

    TextureMapperShaderProgramMap::iterator it = m_textureMapperShaderProgramMap.find(shaderType);
    if (it != m_textureMapperShaderProgramMap.end())
        return it->second;

    switch (shaderType) {
    case Simple:
        program = TextureMapperShaderProgramSimple::create();
        break;
    case RectSimple:
        program = TextureMapperShaderProgramRectSimple::create();
        break;
    case OpacityAndMask:
        program = TextureMapperShaderProgramOpacityAndMask::create();
        break;
    case RectOpacityAndMask:
        program = TextureMapperShaderProgramRectOpacityAndMask::create();
        break;
    case SolidColor:
        program = TextureMapperShaderProgramSolidColor::create();
        break;
    case Invalid:
        ASSERT_NOT_REACHED();
    }
    m_textureMapperShaderProgramMap.add(shaderType, program);
    return program;
}

void TextureMapperShaderProgram::initializeProgram()
{
    const char* vertexShaderSourceProgram = vertexShaderSource();
    const char* fragmentShaderSourceProgram = fragmentShaderSource();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSourceProgram, 0);
    glShaderSource(fragmentShader, 1, &fragmentShaderSourceProgram, 0);
    GLuint programID = glCreateProgram();
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    m_vertexAttrib = glGetAttribLocation(programID, "InVertex");
    m_id = programID;
    m_vertexShader = vertexShader;
    m_fragmentShader = fragmentShader;
}

void TextureMapperShaderProgram::getUniformLocation(GLint &variable, const char* name)
{
    variable = glGetUniformLocation(m_id, name);
    ASSERT(variable >= 0);
}

TextureMapperShaderProgram::~TextureMapperShaderProgram()
{
    GLuint programID = m_id;
    if (!programID)
        return;

    glDetachShader(programID, m_vertexShader);
    glDeleteShader(m_vertexShader);
    glDetachShader(programID, m_fragmentShader);
    glDeleteShader(m_fragmentShader);
    glDeleteProgram(programID);
}

PassRefPtr<TextureMapperShaderProgramSimple> TextureMapperShaderProgramSimple::create()
{
    RefPtr<TextureMapperShaderProgramSimple> program = adoptRef(new TextureMapperShaderProgramSimple());
    // Avoid calling virtual functions in constructor.
    program->initialize();
    return program.release();
}

void TextureMapperShaderProgramSimple::initialize()
{
    initializeProgram();
    getUniformLocation(m_sourceMatrixVariable, "InSourceMatrix");
    getUniformLocation(m_matrixVariable, "InMatrix");
    getUniformLocation(m_sourceTextureVariable, "SourceTexture");
    getUniformLocation(m_opacityVariable, "Opacity");
}

const char* TextureMapperShaderProgramSimple::vertexShaderSource() const
{
    return vertexShaderSourceSimple;
}

const char* TextureMapperShaderProgramSimple::fragmentShaderSource() const
{
    return fragmentShaderSourceSimple;
}

void TextureMapperShaderProgramSimple::prepare(float opacity, const BitmapTexture* maskTexture)
{
    glUniform1f(m_opacityVariable, opacity);
}

PassRefPtr<TextureMapperShaderProgramSolidColor> TextureMapperShaderProgramSolidColor::create()
{
    RefPtr<TextureMapperShaderProgramSolidColor> program = adoptRef(new TextureMapperShaderProgramSolidColor());
    // Avoid calling virtual functions in constructor.
    program->initialize();
    return program.release();
}

void TextureMapperShaderProgramSolidColor::initialize()
{
    initializeProgram();
    getUniformLocation(m_matrixVariable, "InMatrix");
    getUniformLocation(m_colorVariable, "Color");
}

const char* TextureMapperShaderProgramSolidColor::vertexShaderSource() const
{
    return vertexShaderSourceSolidColor;
}

const char* TextureMapperShaderProgramSolidColor::fragmentShaderSource() const
{
    return fragmentShaderSourceSolidColor;
}

PassRefPtr<TextureMapperShaderProgramRectSimple> TextureMapperShaderProgramRectSimple::create()
{
    RefPtr<TextureMapperShaderProgramRectSimple> program = adoptRef(new TextureMapperShaderProgramRectSimple());
    // Avoid calling virtual functions in constructor.
    program->initialize();
    return program.release();
}

const char* TextureMapperShaderProgramRectSimple::fragmentShaderSource() const
{
    return fragmentShaderSourceRectSimple;
}

PassRefPtr<TextureMapperShaderProgramOpacityAndMask> TextureMapperShaderProgramOpacityAndMask::create()
{
    RefPtr<TextureMapperShaderProgramOpacityAndMask> program = adoptRef(new TextureMapperShaderProgramOpacityAndMask());
    // Avoid calling virtual functions in constructor.
    program->initialize();
    return program.release();
}

void TextureMapperShaderProgramOpacityAndMask::initialize()
{
    initializeProgram();
    getUniformLocation(m_matrixVariable, "InMatrix");
    getUniformLocation(m_sourceMatrixVariable, "InSourceMatrix");
    getUniformLocation(m_sourceTextureVariable, "SourceTexture");
    getUniformLocation(m_maskTextureVariable, "MaskTexture");
    getUniformLocation(m_opacityVariable, "Opacity");
}

const char* TextureMapperShaderProgramOpacityAndMask::vertexShaderSource() const
{
    return vertexShaderSourceOpacityAndMask;
}

const char* TextureMapperShaderProgramOpacityAndMask::fragmentShaderSource() const
{
    return fragmentShaderSourceOpacityAndMask;
}

void TextureMapperShaderProgramOpacityAndMask::prepare(float opacity, const BitmapTexture* maskTexture)
{
    glUniform1f(m_opacityVariable, opacity);
    if (!maskTexture || !maskTexture->isValid())
        return;

    const BitmapTextureGL* maskTextureGL = static_cast<const BitmapTextureGL*>(maskTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, maskTextureGL->id());
    glUniform1i(m_maskTextureVariable, 1);
    glActiveTexture(GL_TEXTURE0);
}

PassRefPtr<TextureMapperShaderProgramRectOpacityAndMask> TextureMapperShaderProgramRectOpacityAndMask::create()
{
    RefPtr<TextureMapperShaderProgramRectOpacityAndMask> program = adoptRef(new TextureMapperShaderProgramRectOpacityAndMask());
    // Avoid calling virtual functions in constructor.
    program->initialize();
    return program.release();
}

const char* TextureMapperShaderProgramRectOpacityAndMask::fragmentShaderSource() const
{
    return fragmentShaderSourceRectOpacityAndMask;
}

TextureMapperShaderManager::TextureMapperShaderManager()
{
    ASSERT(initializeOpenGLShims());
}

TextureMapperShaderManager::~TextureMapperShaderManager()
{
}

#if ENABLE(CSS_FILTERS)
StandardFilterProgram::~StandardFilterProgram()
{
    glDetachShader(m_id, m_vertexShader);
    glDeleteShader(m_vertexShader);
    glDetachShader(m_id, m_fragmentShader);
    glDeleteShader(m_fragmentShader);
    glDeleteProgram(m_id);
}

StandardFilterProgram::StandardFilterProgram(FilterOperation::OperationType type, unsigned pass)
    : m_id(0)
{
    const char* vertexShaderSource =
            VERTEX_SHADER(
                attribute vec4 a_vertex;
                attribute vec4 a_texCoord;
                varying highp vec2 v_texCoord;
                void main(void)
                {
                    v_texCoord = vec2(a_texCoord);
                    gl_Position = a_vertex;
                }
            );

#define STANDARD_FILTER(x...) \
        OES2_PRECISION_DEFINITIONS\
        OES2_FRAGMENT_SHADER_DEFAULT_PRECISION\
        "varying highp vec2 v_texCoord;\n"\
        "uniform highp float u_amount;\n"\
        "uniform sampler2D u_texture;\n"\
        #x\
        "void main(void)\n { gl_FragColor = shade(texture2D(u_texture, v_texCoord)); }"

    const char* fragmentShaderSource = 0;
    switch (type) {
    case FilterOperation::GRAYSCALE:
        fragmentShaderSource = STANDARD_FILTER(
            lowp vec4 shade(lowp vec4 color)
            {
                lowp float amount = 1.0 - u_amount;
                return vec4((0.2126 + 0.7874 * amount) * color.r + (0.7152 - 0.7152 * amount) * color.g + (0.0722 - 0.0722 * amount) * color.b,
                            (0.2126 - 0.2126 * amount) * color.r + (0.7152 + 0.2848 * amount) * color.g + (0.0722 - 0.0722 * amount) * color.b,
                            (0.2126 - 0.2126 * amount) * color.r + (0.7152 - 0.7152 * amount) * color.g + (0.0722 + 0.9278 * amount) * color.b,
                            color.a);
            }
        );
        break;
    case FilterOperation::SEPIA:
        fragmentShaderSource = STANDARD_FILTER(
            lowp vec4 shade(lowp vec4 color)
            {
                lowp float amount = 1.0 - u_amount;
                return vec4((0.393 + 0.607 * amount) * color.r + (0.769 - 0.769 * amount) * color.g + (0.189 - 0.189 * amount) * color.b,
                            (0.349 - 0.349 * amount) * color.r + (0.686 + 0.314 * amount) * color.g + (0.168 - 0.168 * amount) * color.b,
                            (0.272 - 0.272 * amount) * color.r + (0.534 - 0.534 * amount) * color.g + (0.131 + 0.869 * amount) * color.b,
                            color.a);
            }
        );
        break;
    case FilterOperation::SATURATE:
        fragmentShaderSource = STANDARD_FILTER(
            lowp vec4 shade(lowp vec4 color)
            {
                return vec4((0.213 + 0.787 * u_amount) * color.r + (0.715 - 0.715 * u_amount) * color.g + (0.072 - 0.072 * u_amount) * color.b,
                            (0.213 - 0.213 * u_amount) * color.r + (0.715 + 0.285 * u_amount) * color.g + (0.072 - 0.072 * u_amount) * color.b,
                            (0.213 - 0.213 * u_amount) * color.r + (0.715 - 0.715 * u_amount) * color.g + (0.072 + 0.928 * u_amount) * color.b,
                            color.a);
            }
        );
        break;
    case FilterOperation::HUE_ROTATE:
        fragmentShaderSource = STANDARD_FILTER(
            lowp vec4 shade(lowp vec4 color)
            {
                highp float pi = 3.14159265358979323846;
                highp float c = cos(u_amount * pi / 180.0);
                highp float s = sin(u_amount * pi / 180.0);
                return vec4(color.r * (0.213 + c * 0.787 - s * 0.213) + color.g * (0.715 - c * 0.715 - s * 0.715) + color.b * (0.072 - c * 0.072 + s * 0.928),
                            color.r * (0.213 - c * 0.213 + s * 0.143) + color.g * (0.715 + c * 0.285 + s * 0.140) + color.b * (0.072 - c * 0.072 - s * 0.283),
                            color.r * (0.213 - c * 0.213 - s * 0.787) +  color.g * (0.715 - c * 0.715 + s * 0.715) + color.b * (0.072 + c * 0.928 + s * 0.072),
                            color.a);
            }
        );
        break;
    case FilterOperation::INVERT:
        fragmentShaderSource = STANDARD_FILTER(
            lowp float invert(lowp float n) { return (1.0 - n) * u_amount + n * (1.0 - u_amount); }
            lowp vec4 shade(lowp vec4 color)
            {
                return vec4(invert(color.r), invert(color.g), invert(color.b), color.a);
            }
        );
        break;
    case FilterOperation::BRIGHTNESS:
        fragmentShaderSource = STANDARD_FILTER(
            lowp vec4 shade(lowp vec4 color)
            {
                return vec4(color.rgb * (1.0 + u_amount), color.a);
            }
        );
        break;
    case FilterOperation::CONTRAST:
        fragmentShaderSource = STANDARD_FILTER(
            lowp float contrast(lowp float n) { return (n - 0.5) * u_amount + 0.5; }
            lowp vec4 shade(lowp vec4 color)
            {
                return vec4(contrast(color.r), contrast(color.g), contrast(color.b), color.a);
            }
        );
        break;
    case FilterOperation::OPACITY:
        fragmentShaderSource = STANDARD_FILTER(
            lowp vec4 shade(lowp vec4 color)
            {
                return vec4(color.r, color.g, color.b, color.a * u_amount);
            }
        );
        break;
    case FilterOperation::BLUR:
        fragmentShaderSource = FRAGMENT_SHADER(
            varying highp vec2 v_texCoord;
            uniform lowp vec2 u_blurRadius;
            uniform sampler2D u_texture;
            const float pi = 3.14159;
            const float e = 2.71828;

            // FIXME: share gaussian formula between shaders.
            lowp float gaussian(lowp float value)
            {
                // Normal distribution formula, when the mean is 0 and the standard deviation is 1.
                return pow(e, -pow(value, 2.) / 2.) / (sqrt(2. * pi));
            }

            lowp vec4 sampleColor(float radius)
            {
                vec2 coord = v_texCoord + radius * u_blurRadius;
                return texture2D(u_texture, coord) * float(coord.x > 0. && coord.y > 0. && coord.x < 1. && coord.y < 1.);
            }

            vec4 blur()
            {
                // Create a normal distribution of 20 values between 0. and 2.
                vec4 total = vec4(0., 0., 0., 0.);
                float totalWeight = 0.;
                for (float i = -2.; i <= 2.; i += .2) {
                    float weight = gaussian(i);
                    total += sampleColor(i) * weight;
                    totalWeight += weight;
                }

                return total / totalWeight;
            }

            void main(void)
            {
                gl_FragColor = blur();
            }
        );
        break;
    case FilterOperation::DROP_SHADOW:
        switch (pass) {
        case 0: {
            // First pass: horizontal alpha blur.
            fragmentShaderSource = FRAGMENT_SHADER(
                varying highp vec2 v_texCoord;
                uniform lowp float u_shadowBlurRadius;
                uniform lowp vec2 u_shadowOffset;
                uniform sampler2D u_texture;
                const float pi = 3.14159;
                const float e = 2.71828;

                // FIXME: share gaussian formula between shaders.
                lowp float gaussian(lowp float value)
                {
                    // Normal distribution formula, when the mean is 0 and the standard deviation is 1.
                    return pow(e, -pow(value, 2.) / 2.) / (sqrt(2. * pi));
                }

                lowp float sampleAlpha(float radius)
                {
                    vec2 coord = v_texCoord - u_shadowOffset + vec2(radius * u_shadowBlurRadius, 0.);
                    return texture2D(u_texture, coord).a * float(coord.x > 0. && coord.y > 0. && coord.x < 1. && coord.y < 1.);
                }

                lowp float shadowBlurHorizontal()
                {
                    // Create a normal distribution of 20 values between -2 and 2.
                    float total = 0.;
                    float totalWeight = 0.;
                    for (float i = -2.; i <= 2.; i += .2) {
                        float weight = gaussian(i);
                        total += sampleAlpha(i) * weight;
                        totalWeight += weight;
                    }

                    return total / totalWeight;
                }

                void main(void)
                {
                    gl_FragColor = vec4(1., 1., 1., 1.) * shadowBlurHorizontal();
                }
            );
            break;
            }
        case 1: {
            // Second pass: vertical alpha blur and composite with origin.
            fragmentShaderSource = FRAGMENT_SHADER(
                varying highp vec2 v_texCoord;
                uniform lowp float u_shadowBlurRadius;
                uniform lowp vec4 u_shadowColor;
                uniform sampler2D u_texture;
                uniform sampler2D u_contentTexture;

                // FIXME: share gaussian formula between shaders.
                const float pi = 3.14159;
                const float e = 2.71828;

                lowp float gaussian(float value)
                {
                    // Normal distribution formula, when the mean is 0 and the standard deviation is 1.
                    return pow(e, -pow(value, 2.) / 2.) / (sqrt(2. * pi));
                }

                lowp float sampleAlpha(float r)
                {
                    vec2 coord = v_texCoord + vec2(0., r * u_shadowBlurRadius);
                    return texture2D(u_texture, coord).a * float(coord.x > 0. && coord.y > 0. && coord.x < 1. && coord.y < 1.);
                }

                lowp float shadowBlurVertical()
                {
                    // Create a normal distribution of 20 values between -2 and 2.
                    float total = 0.;
                    float totalWeight = 0.;
                    for (float i = -2.; i <= 2.; i += .2) {
                        float weight = gaussian(i);
                        total += sampleAlpha(i) * weight;
                        totalWeight += weight;
                    }

                    return total / totalWeight;
                }

                lowp vec4 sourceOver(lowp vec4 source, lowp vec4 destination)
                {
                    // Composite the shadow with the original texture.
                    return source + destination * (1. - source.a);
                }

                void main(void)
                {
                    gl_FragColor = sourceOver(texture2D(u_contentTexture, v_texCoord), shadowBlurVertical() * u_shadowColor);
                }
            );
            break;
            }
            break;
        }
    default:
        break;
    }

    if (!fragmentShaderSource)
        return;
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, 0);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, 0);
    GLuint programID = glCreateProgram();
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);
#if !LOG_DISABLED
    GLchar log[100];
    GLint len;
    glGetShaderInfoLog(fragmentShader, 100, &len, log);
    WTFLog(&LogCompositing, "%s\n", log);
#endif
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    m_vertexAttrib = glGetAttribLocation(programID, "a_vertex");
    m_texCoordAttrib = glGetAttribLocation(programID, "a_texCoord");
    m_textureUniformLocation = glGetUniformLocation(programID, "u_texture");
    switch (type) {
    case FilterOperation::GRAYSCALE:
    case FilterOperation::SEPIA:
    case FilterOperation::SATURATE:
    case FilterOperation::HUE_ROTATE:
    case FilterOperation::INVERT:
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::OPACITY:
        m_uniformLocations.amount = glGetUniformLocation(programID, "u_amount");
        break;
    case FilterOperation::BLUR:
        m_uniformLocations.blur.radius = glGetUniformLocation(programID, "u_blurRadius");
        break;
    case FilterOperation::DROP_SHADOW:
        m_uniformLocations.shadow.blurRadius = glGetUniformLocation(programID, "u_shadowBlurRadius");
        if (!pass)
            m_uniformLocations.shadow.offset = glGetUniformLocation(programID, "u_shadowOffset");
        else {
            // We only need the color and the content texture in the second pass, the first pass is only a horizontal alpha blur.
            m_uniformLocations.shadow.color = glGetUniformLocation(programID, "u_shadowColor");
            m_uniformLocations.shadow.contentTexture = glGetUniformLocation(programID, "u_contentTexture");
        }
        break;
    default:
        break;
    }
    m_id = programID;
    m_vertexShader = vertexShader;
    m_fragmentShader = fragmentShader;
}

PassRefPtr<StandardFilterProgram> StandardFilterProgram::create(FilterOperation::OperationType type, unsigned pass)
{
    RefPtr<StandardFilterProgram> program = adoptRef(new StandardFilterProgram(type, pass));
    if (!program->m_id)
        return 0;

    return program;
}

void StandardFilterProgram::prepare(const FilterOperation& operation, unsigned pass, const IntSize& size, GLuint contentTexture)
{
    glUseProgram(m_id);
    switch (operation.getOperationType()) {
    case FilterOperation::GRAYSCALE:
    case FilterOperation::SEPIA:
    case FilterOperation::SATURATE:
    case FilterOperation::HUE_ROTATE:
        glUniform1f(m_uniformLocations.amount, static_cast<const BasicColorMatrixFilterOperation&>(operation).amount());
        break;
    case FilterOperation::INVERT:
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::OPACITY:
        glUniform1f(m_uniformLocations.amount, static_cast<const BasicComponentTransferFilterOperation&>(operation).amount());
        break;
    case FilterOperation::BLUR: {
        const BlurFilterOperation& blur = static_cast<const BlurFilterOperation&>(operation);
        FloatSize radius;

        // Blur is done in two passes, first horizontally and then vertically. The same shader is used for both.
        if (pass)
            radius.setHeight(floatValueForLength(blur.stdDeviation(), size.height()) / size.height());
        else
            radius.setWidth(floatValueForLength(blur.stdDeviation(), size.width()) / size.width());

        glUniform2f(m_uniformLocations.blur.radius, radius.width(), radius.height());
        break;
    }
    case FilterOperation::DROP_SHADOW: {
        const DropShadowFilterOperation& shadow = static_cast<const DropShadowFilterOperation&>(operation);
        switch (pass) {
        case 0:
            // First pass: vertical alpha blur.
            glUniform2f(m_uniformLocations.shadow.offset, float(shadow.location().x()) / float(size.width()), float(shadow.location().y()) / float(size.height()));
            glUniform1f(m_uniformLocations.shadow.blurRadius, shadow.stdDeviation() / float(size.width()));
            break;
        case 1:
            // Second pass: we need the shadow color and the content texture for compositing.
            glUniform1f(m_uniformLocations.shadow.blurRadius, shadow.stdDeviation() / float(size.height()));
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, contentTexture);
            glUniform1i(m_uniformLocations.shadow.contentTexture, 1);
            float r, g, b, a;
            shadow.color().getRGBA(r, g, b, a);
            glUniform4f(m_uniformLocations.shadow.color, r, g, b, a);
            break;
        }
        break;
    }
    default:
        break;
    }
}

PassRefPtr<StandardFilterProgram> TextureMapperShaderManager::getShaderForFilter(const FilterOperation& filter, unsigned pass)
{
    RefPtr<StandardFilterProgram> program;
    FilterOperation::OperationType type = filter.getOperationType();
    int key = int(type) | (pass << 16);
    FilterMap::iterator iterator = m_filterMap.find(key);
    if (iterator == m_filterMap.end()) {
        program = StandardFilterProgram::create(type, pass);
        if (!program)
            return 0;

        m_filterMap.add(key, program);
    } else
        program = iterator->second;

    return program;
}

unsigned TextureMapperShaderManager::getPassesRequiredForFilter(const FilterOperation& operation) const
{
    switch (operation.getOperationType()) {
    case FilterOperation::GRAYSCALE:
    case FilterOperation::SEPIA:
    case FilterOperation::SATURATE:
    case FilterOperation::HUE_ROTATE:
    case FilterOperation::INVERT:
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
    case FilterOperation::OPACITY:
        return 1;
    case FilterOperation::BLUR:
    case FilterOperation::DROP_SHADOW:
        // We use two-passes (vertical+horizontal) for blur and drop-shadow.
        return 2;
    default:
        return 0;
    }

}

#endif
};

#endif
