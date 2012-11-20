/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012 Company 100, Inc.
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
#include "CoordinatedGraphicsArgumentCoders.h"

#if USE(COORDINATED_GRAPHICS)
#include "WebCoreArgumentCoders.h"
#include <WebCore/Animation.h>
#include <WebCore/Color.h>
#include <WebCore/FloatPoint3D.h>
#include <WebCore/GraphicsLayerAnimation.h>
#include <WebCore/IdentityTransformOperation.h>
#include <WebCore/IntPoint.h>
#include <WebCore/Length.h>
#include <WebCore/Matrix3DTransformOperation.h>
#include <WebCore/MatrixTransformOperation.h>
#include <WebCore/PerspectiveTransformOperation.h>
#include <WebCore/RotateTransformOperation.h>
#include <WebCore/ScaleTransformOperation.h>
#include <WebCore/SkewTransformOperation.h>
#include <WebCore/TimingFunction.h>
#include <WebCore/TransformationMatrix.h>
#include <WebCore/TranslateTransformOperation.h>

#if ENABLE(CSS_FILTERS)
#include <WebCore/FilterOperations.h>
#endif

#if ENABLE(CSS_SHADERS)
#include "WebCustomFilterOperation.h"
#include "WebCustomFilterProgram.h"
#include "WebCustomFilterProgramProxy.h"
#include <WebCore/CustomFilterArrayParameter.h>
#include <WebCore/CustomFilterConstants.h>
#include <WebCore/CustomFilterNumberParameter.h>
#include <WebCore/CustomFilterOperation.h>
#include <WebCore/CustomFilterProgram.h>
#include <WebCore/CustomFilterProgramInfo.h>
#include <WebCore/CustomFilterTransformParameter.h>
#include <WebCore/CustomFilterValidatedProgram.h>
#include <WebCore/ValidatedCustomFilterOperation.h>
#endif

#if USE(GRAPHICS_SURFACE)
#include <WebCore/GraphicsSurface.h>
#endif

using namespace WebCore;
#if ENABLE(CSS_SHADERS)
using namespace WebKit;
#endif

namespace CoreIPC {

void ArgumentCoder<FloatPoint3D>::encode(ArgumentEncoder& encoder, const FloatPoint3D& floatPoint3D)
{
    SimpleArgumentCoder<FloatPoint3D>::encode(encoder, floatPoint3D);
}

bool ArgumentCoder<FloatPoint3D>::decode(ArgumentDecoder* decoder, FloatPoint3D& floatPoint3D)
{
    return SimpleArgumentCoder<FloatPoint3D>::decode(decoder, floatPoint3D);
}

void ArgumentCoder<Length>::encode(ArgumentEncoder& encoder, const Length& length)
{
    SimpleArgumentCoder<Length>::encode(encoder, length);
}

bool ArgumentCoder<Length>::decode(ArgumentDecoder* decoder, Length& length)
{
    return SimpleArgumentCoder<Length>::decode(decoder, length);
}

void ArgumentCoder<TransformationMatrix>::encode(ArgumentEncoder& encoder, const TransformationMatrix& transformationMatrix)
{
    SimpleArgumentCoder<TransformationMatrix>::encode(encoder, transformationMatrix);
}

bool ArgumentCoder<TransformationMatrix>::decode(ArgumentDecoder* decoder, TransformationMatrix& transformationMatrix)
{
    return SimpleArgumentCoder<TransformationMatrix>::decode(decoder, transformationMatrix);
}

#if ENABLE(CSS_FILTERS)
void ArgumentCoder<WebCore::FilterOperations>::encode(ArgumentEncoder& encoder, const WebCore::FilterOperations& filters)
{
    encoder << static_cast<uint32_t>(filters.size());
    for (size_t i = 0; i < filters.size(); ++i) {
        const FilterOperation* filter = filters.at(i);
        FilterOperation::OperationType type = filter->getOperationType();
        encoder.encodeEnum(type);
        switch (type) {
        case FilterOperation::GRAYSCALE:
        case FilterOperation::SEPIA:
        case FilterOperation::SATURATE:
        case FilterOperation::HUE_ROTATE:
            encoder << static_cast<double>(static_cast<const BasicColorMatrixFilterOperation*>(filter)->amount());
            break;
        case FilterOperation::INVERT:
        case FilterOperation::BRIGHTNESS:
        case FilterOperation::CONTRAST:
        case FilterOperation::OPACITY:
            encoder << static_cast<double>(static_cast<const BasicComponentTransferFilterOperation*>(filter)->amount());
            break;
        case FilterOperation::BLUR:
            ArgumentCoder<Length>::encode(encoder, static_cast<const BlurFilterOperation*>(filter)->stdDeviation());
            break;
        case FilterOperation::DROP_SHADOW: {
            const DropShadowFilterOperation* shadow = static_cast<const DropShadowFilterOperation*>(filter);
            ArgumentCoder<IntPoint>::encode(encoder, shadow->location());
            encoder << static_cast<int32_t>(shadow->stdDeviation());
            ArgumentCoder<Color>::encode(encoder, shadow->color());
            break;
        }
#if ENABLE(CSS_SHADERS)
        case FilterOperation::CUSTOM:
            // Custom Filters are converted to VALIDATED_CUSTOM before reaching this point.
            ASSERT_NOT_REACHED();
            break;
        case FilterOperation::VALIDATED_CUSTOM: {
            const ValidatedCustomFilterOperation* customOperation = static_cast<const ValidatedCustomFilterOperation*>(filter);
            ASSERT(customOperation->validatedProgram());
            RefPtr<CustomFilterValidatedProgram> program = customOperation->validatedProgram();
            ASSERT(program->isInitialized());
            ASSERT(program->platformCompiledProgram());
            ASSERT(program->platformCompiledProgram()->client());
            WebCustomFilterProgramProxy* customFilterProgramProxy = static_cast<WebCustomFilterProgramProxy*>(program->platformCompiledProgram()->client());
            const CustomFilterProgramInfo& programInfo = program->programInfo();
            // FIXME: CustomFilterOperation should not need the meshType, it should just be encoded in the program itself.
            // https://bugs.webkit.org/show_bug.cgi?id=102529
            encoder.encodeEnum(programInfo.meshType());
            encoder << customFilterProgramProxy->id();
            CustomFilterParameterList parameters = customOperation->parameters();
            encoder << static_cast<uint32_t>(parameters.size());
            for (size_t i = 0; i < parameters.size(); ++i) {
                RefPtr<CustomFilterParameter> parameter = parameters[i];
                encoder << parameter->name();
                encoder.encodeEnum(parameter->parameterType());

                switch (parameter->parameterType()) {
                case CustomFilterParameter::ARRAY: {
                    CustomFilterArrayParameter* arrayParameter = static_cast<CustomFilterArrayParameter*>(parameter.get());
                    encoder << static_cast<uint32_t>(arrayParameter->size());
                    for (size_t j = 0; j < arrayParameter->size(); ++j)
                        encoder << arrayParameter->valueAt(j);
                    break;
                }
                case CustomFilterParameter::NUMBER: {
                    CustomFilterNumberParameter* nubmerParameter = static_cast<CustomFilterNumberParameter*>(parameter.get());
                    encoder << static_cast<uint32_t>(nubmerParameter->size());
                    for (size_t j = 0; j < nubmerParameter->size(); ++j)
                        encoder << nubmerParameter->valueAt(j);
                    break;
                }
                case CustomFilterParameter::TRANSFORM: {
                    CustomFilterTransformParameter* transformParameter = static_cast<CustomFilterTransformParameter*>(parameter.get());
                    ArgumentCoder<TransformOperations>::encode(encoder, transformParameter->operations());
                    break;
                }
                }
            }

            encoder << customOperation->meshRows();
            encoder << customOperation->meshColumns();
            // FIXME: The ValidatedCustomFilterOperation doesn't have the meshBoxType yet, we just use the default one for now.
            // https://bugs.webkit.org/show_bug.cgi?id=100890
            encoder.encodeEnum(MeshBoxTypeFilter);
            break;
        }
#endif
        default:
            break;
        }
    }
}

bool ArgumentCoder<WebCore::FilterOperations>::decode(ArgumentDecoder* decoder, WebCore::FilterOperations& filters)
{
    uint32_t size;
    if (!decoder->decodeUInt32(size))
        return false;

    Vector<RefPtr<FilterOperation> >& operations = filters.operations();

    for (size_t i = 0; i < size; ++i) {
        FilterOperation::OperationType type;
        RefPtr<FilterOperation> filter;
        if (!decoder->decodeEnum(type))
            return false;

        switch (type) {
        case FilterOperation::GRAYSCALE:
        case FilterOperation::SEPIA:
        case FilterOperation::SATURATE:
        case FilterOperation::HUE_ROTATE: {
            double value;
            if (!decoder->decodeDouble(value))
                return false;
            filter = BasicColorMatrixFilterOperation::create(value, type);
            break;
        }
        case FilterOperation::INVERT:
        case FilterOperation::BRIGHTNESS:
        case FilterOperation::CONTRAST:
        case FilterOperation::OPACITY: {
            double value;
            if (!decoder->decodeDouble(value))
                return false;
            filter = BasicComponentTransferFilterOperation::create(value, type);
            break;
        }
        case FilterOperation::BLUR: {
            Length length;
            if (!ArgumentCoder<Length>::decode(decoder, length))
                return false;
            filter = BlurFilterOperation::create(length, type);
            break;
        }
        case FilterOperation::DROP_SHADOW: {
            IntPoint location;
            int32_t stdDeviation;
            Color color;
            if (!ArgumentCoder<IntPoint>::decode(decoder, location))
                return false;
            if (!decoder->decodeInt32(stdDeviation))
                return false;
            if (!ArgumentCoder<Color>::decode(decoder, color))
                return false;
            filter = DropShadowFilterOperation::create(location, stdDeviation, color, type);
            break;
        }
#if ENABLE(CSS_SHADERS)
        case FilterOperation::CUSTOM:
            // Custom Filters are converted to VALIDATED_CUSTOM before reaching this point.
            ASSERT_NOT_REACHED();
            break;
        case FilterOperation::VALIDATED_CUSTOM: {
            // FIXME: CustomFilterOperation should not need the meshType.
            // https://bugs.webkit.org/show_bug.cgi?id=102529
            CustomFilterMeshType meshType;
            if (!decoder->decodeEnum(meshType))
                return false;
            int programID = 0;
            if (!decoder->decode(programID))
                return false;

            uint32_t parametersSize;
            if (!decoder->decodeUInt32(parametersSize))
                return false;

            CustomFilterParameterList parameters(parametersSize);
            for (size_t i = 0; i < parametersSize; ++i) {
                String name;
                CustomFilterParameter::ParameterType parameterType;
                if (!decoder->decode(name))
                    return false;
                if (!decoder->decodeEnum(parameterType))
                    return false;

                switch (parameterType) {
                case CustomFilterParameter::ARRAY: {
                    RefPtr<CustomFilterArrayParameter> arrayParameter = CustomFilterArrayParameter::create(name);
                    uint32_t arrayParameterSize;
                    if (!decoder->decodeUInt32(arrayParameterSize))
                        return false;
                    double arrayParameterValue;
                    for (size_t j = 0; j < arrayParameterSize; ++j) {
                        if (!decoder->decode(arrayParameterValue))
                            return false;
                        arrayParameter->addValue(arrayParameterValue);
                    }
                    parameters[i] = arrayParameter.release();
                    break;
                }
                case CustomFilterParameter::NUMBER: {
                    RefPtr<CustomFilterNumberParameter> numberParameter = CustomFilterNumberParameter::create(name);
                    uint32_t numberParameterSize;
                    if (!decoder->decodeUInt32(numberParameterSize))
                        return false;
                    double numberParameterValue;
                    for (size_t j = 0; j < numberParameterSize; ++j) {
                        if (!decoder->decode(numberParameterValue))
                            return false;
                        numberParameter->addValue(numberParameterValue);
                    }
                    parameters[i] = numberParameter.release();
                    break;
                }
                case CustomFilterParameter::TRANSFORM: {
                    RefPtr<CustomFilterTransformParameter> transformParameter = CustomFilterTransformParameter::create(name);
                    TransformOperations operations;
                    if (!ArgumentCoder<TransformOperations>::decode(decoder, operations))
                        return false;
                    transformParameter->setOperations(operations);
                    parameters[i] = transformParameter.release();
                    break;
                }
                }
            }

            unsigned meshRows;
            unsigned meshColumns;
            CustomFilterMeshBoxType meshBoxType;
            if (!decoder->decode(meshRows))
                return false;
            if (!decoder->decode(meshColumns))
                return false;
            if (!decoder->decodeEnum(meshBoxType))
                return false;

            // At this point the Shaders are already validated, so we just use WebCustomFilterOperation for transportation.
            filter = WebCustomFilterOperation::create(0, programID, parameters, meshRows, meshColumns, meshBoxType, meshType);
            break;
        }
#endif
        default:
            break;
        }

        if (filter)
            operations.append(filter);
    }

    return true;
}
#endif

#if ENABLE(CSS_SHADERS)
void ArgumentCoder<WebCore::CustomFilterProgramInfo>::encode(ArgumentEncoder& encoder, const CustomFilterProgramInfo& programInfo)
{
    encoder << programInfo.vertexShaderString();
    encoder << programInfo.fragmentShaderString();
    encoder.encodeEnum(programInfo.programType());
    encoder.encodeEnum(programInfo.meshType());
    const CustomFilterProgramMixSettings& mixSettings = programInfo.mixSettings();
    encoder.encodeEnum(mixSettings.blendMode);
    encoder.encodeEnum(mixSettings.compositeOperator);
}

bool ArgumentCoder<WebCore::CustomFilterProgramInfo>::decode(ArgumentDecoder* decoder, CustomFilterProgramInfo& programInfo)
{
    String vertexShaderString;
    String fragmentShaderString;
    CustomFilterProgramType programType;
    CustomFilterMeshType meshType;
    CustomFilterProgramMixSettings mixSettings;
    if (!decoder->decode(vertexShaderString))
        return false;
    if (!decoder->decode(fragmentShaderString))
        return false;
    if (!decoder->decodeEnum(programType))
        return false;
    if (!decoder->decodeEnum(meshType))
        return false;
    if (!decoder->decodeEnum(mixSettings.blendMode))
        return false;
    if (!decoder->decodeEnum(mixSettings.compositeOperator))
        return false;
    programInfo = CustomFilterProgramInfo(vertexShaderString, fragmentShaderString, programType, mixSettings, meshType);
    return true;
}
#endif // ENABLE(CSS_SHADERS)

void ArgumentCoder<TransformOperations>::encode(ArgumentEncoder& encoder, const TransformOperations& transformOperations)
{
    encoder << static_cast<uint32_t>(transformOperations.size());
    for (size_t i = 0; i < transformOperations.size(); ++i) {
        const TransformOperation* operation = transformOperations.at(i);
        encoder.encodeEnum(operation->getOperationType());

        switch (operation->getOperationType()) {
        case TransformOperation::SCALE_X:
        case TransformOperation::SCALE_Y:
        case TransformOperation::SCALE:
        case TransformOperation::SCALE_Z:
        case TransformOperation::SCALE_3D:
            encoder << static_cast<const ScaleTransformOperation*>(operation)->x();
            encoder << static_cast<const ScaleTransformOperation*>(operation)->y();
            encoder << static_cast<const ScaleTransformOperation*>(operation)->z();
            break;
        case TransformOperation::TRANSLATE_X:
        case TransformOperation::TRANSLATE_Y:
        case TransformOperation::TRANSLATE:
        case TransformOperation::TRANSLATE_Z:
        case TransformOperation::TRANSLATE_3D:
            ArgumentCoder<Length>::encode(encoder, static_cast<const TranslateTransformOperation*>(operation)->x());
            ArgumentCoder<Length>::encode(encoder, static_cast<const TranslateTransformOperation*>(operation)->y());
            ArgumentCoder<Length>::encode(encoder, static_cast<const TranslateTransformOperation*>(operation)->z());
            break;
        case TransformOperation::ROTATE:
        case TransformOperation::ROTATE_X:
        case TransformOperation::ROTATE_Y:
        case TransformOperation::ROTATE_3D:
            encoder << static_cast<const RotateTransformOperation*>(operation)->x();
            encoder << static_cast<const RotateTransformOperation*>(operation)->y();
            encoder << static_cast<const RotateTransformOperation*>(operation)->z();
            encoder << static_cast<const RotateTransformOperation*>(operation)->angle();
            break;
        case TransformOperation::SKEW_X:
        case TransformOperation::SKEW_Y:
        case TransformOperation::SKEW:
            encoder << static_cast<const SkewTransformOperation*>(operation)->angleX();
            encoder << static_cast<const SkewTransformOperation*>(operation)->angleY();
            break;
        case TransformOperation::MATRIX:
            ArgumentCoder<TransformationMatrix>::encode(encoder, static_cast<const MatrixTransformOperation*>(operation)->matrix());
            break;
        case TransformOperation::MATRIX_3D:
            ArgumentCoder<TransformationMatrix>::encode(encoder, static_cast<const Matrix3DTransformOperation*>(operation)->matrix());
            break;
        case TransformOperation::PERSPECTIVE:
            ArgumentCoder<Length>::encode(encoder, static_cast<const PerspectiveTransformOperation*>(operation)->perspective());
            break;
        case TransformOperation::IDENTITY:
            break;
        case TransformOperation::NONE:
            ASSERT_NOT_REACHED();
            break;
        }
    }
}

bool ArgumentCoder<TransformOperations>::decode(ArgumentDecoder* decoder, TransformOperations& transformOperations)
{
    uint32_t operationsSize;
    if (!decoder->decodeUInt32(operationsSize))
        return false;

    for (size_t i = 0; i < operationsSize; ++i) {
        TransformOperation::OperationType operationType;
        if (!decoder->decodeEnum(operationType))
            return false;

        switch (operationType) {
        case TransformOperation::SCALE_X:
        case TransformOperation::SCALE_Y:
        case TransformOperation::SCALE:
        case TransformOperation::SCALE_Z:
        case TransformOperation::SCALE_3D: {
            double x, y, z;
            if (!decoder->decode(x))
                return false;
            if (!decoder->decode(y))
                return false;
            if (!decoder->decode(z))
                return false;
            transformOperations.operations().append(ScaleTransformOperation::create(x, y, z, operationType));
            break;
        }
        case TransformOperation::TRANSLATE_X:
        case TransformOperation::TRANSLATE_Y:
        case TransformOperation::TRANSLATE:
        case TransformOperation::TRANSLATE_Z:
        case TransformOperation::TRANSLATE_3D: {
            Length x, y, z;
            if (!ArgumentCoder<Length>::decode(decoder, x))
                return false;
            if (!ArgumentCoder<Length>::decode(decoder, y))
                return false;
            if (!ArgumentCoder<Length>::decode(decoder, z))
                return false;
            transformOperations.operations().append(TranslateTransformOperation::create(x, y, z, operationType));
            break;
        }
        case TransformOperation::ROTATE:
        case TransformOperation::ROTATE_X:
        case TransformOperation::ROTATE_Y:
        case TransformOperation::ROTATE_3D: {
            double x, y, z, angle;
            if (!decoder->decode(x))
                return false;
            if (!decoder->decode(y))
                return false;
            if (!decoder->decode(z))
                return false;
            if (!decoder->decode(angle))
                return false;
            transformOperations.operations().append(RotateTransformOperation::create(x, y, z, angle, operationType));
            break;
        }
        case TransformOperation::SKEW_X:
        case TransformOperation::SKEW_Y:
        case TransformOperation::SKEW: {
            double angleX, angleY;
            if (!decoder->decode(angleX))
                return false;
            if (!decoder->decode(angleY))
                return false;
            transformOperations.operations().append(SkewTransformOperation::create(angleX, angleY, operationType));
            break;
        }
        case TransformOperation::MATRIX: {
            TransformationMatrix matrix;
            if (!ArgumentCoder<TransformationMatrix>::decode(decoder, matrix))
                return false;
            transformOperations.operations().append(MatrixTransformOperation::create(matrix));
            break;
        }
        case TransformOperation::MATRIX_3D: {
            TransformationMatrix matrix;
            if (!ArgumentCoder<TransformationMatrix>::decode(decoder, matrix))
                return false;
            transformOperations.operations().append(Matrix3DTransformOperation::create(matrix));
            break;
        }
        case TransformOperation::PERSPECTIVE: {
            Length perspective;
            if (!ArgumentCoder<Length>::decode(decoder, perspective))
                return false;
            transformOperations.operations().append(PerspectiveTransformOperation::create(perspective));
            break;
        }
        case TransformOperation::IDENTITY:
            transformOperations.operations().append(IdentityTransformOperation::create());
            break;
        case TransformOperation::NONE:
            ASSERT_NOT_REACHED();
            break;
        }
    }
    return true;
}

static void encodeTimingFunction(ArgumentEncoder& encoder, const TimingFunction* timingFunction)
{
    if (!timingFunction) {
        encoder.encodeEnum(TimingFunction::TimingFunctionType(-1));
        return;
    }

    TimingFunction::TimingFunctionType type = timingFunction ? timingFunction->type() : TimingFunction::LinearFunction;
    encoder.encodeEnum(type);
    switch (type) {
    case TimingFunction::LinearFunction:
        break;
    case TimingFunction::CubicBezierFunction: {
        const CubicBezierTimingFunction* cubic = static_cast<const CubicBezierTimingFunction*>(timingFunction);
        encoder << cubic->x1();
        encoder << cubic->y1();
        encoder << cubic->x2();
        encoder << cubic->y2();
        break;
    }
    case TimingFunction::StepsFunction: {
        const StepsTimingFunction* steps = static_cast<const StepsTimingFunction*>(timingFunction);
        encoder << static_cast<uint32_t>(steps->numberOfSteps());
        encoder << steps->stepAtStart();
        break;
    }
    }
}

bool decodeTimingFunction(ArgumentDecoder* decoder, RefPtr<TimingFunction>& timingFunction)
{
    TimingFunction::TimingFunctionType type;
    if (!decoder->decodeEnum(type))
        return false;

    if (type == TimingFunction::TimingFunctionType(-1))
        return true;

    switch (type) {
    case TimingFunction::LinearFunction:
        timingFunction = LinearTimingFunction::create();
        return true;
    case TimingFunction::CubicBezierFunction: {
        double x1, y1, x2, y2;
        if (!decoder->decodeDouble(x1))
            return false;
        if (!decoder->decodeDouble(y1))
            return false;
        if (!decoder->decodeDouble(x2))
            return false;
        if (!decoder->decodeDouble(y2))
            return false;

        timingFunction = CubicBezierTimingFunction::create(x1, y1, x2, y2);
        return true;
    }
    case TimingFunction::StepsFunction: {
        uint32_t numberOfSteps;
        bool stepAtStart;
        if (!decoder->decodeUInt32(numberOfSteps))
            return false;
        if (!decoder->decodeBool(stepAtStart))
            return false;

        timingFunction = StepsTimingFunction::create(numberOfSteps, stepAtStart);
        return true;
    }
    }

    return false;
}

void ArgumentCoder<GraphicsLayerAnimation>::encode(ArgumentEncoder& encoder, const GraphicsLayerAnimation& animation)
{
    encoder << animation.name();
    encoder << animation.boxSize();
    encoder.encodeEnum(animation.state());
    encoder << animation.startTime();
    encoder << animation.pauseTime();
    encoder << animation.listsMatch();

    RefPtr<Animation> animationObject = animation.animation();
    encoder.encodeEnum(animationObject->direction());
    encoder << static_cast<uint32_t>(animationObject->fillMode());
    encoder << animationObject->duration();
    encoder << animationObject->iterationCount();
    encodeTimingFunction(encoder, animationObject->timingFunction().get());

    const KeyframeValueList& keyframes = animation.keyframes();
    encoder.encodeEnum(keyframes.property());
    encoder << static_cast<uint32_t>(keyframes.size());
    for (size_t i = 0; i < keyframes.size(); ++i) {
        const AnimationValue* value = keyframes.at(i);
        encoder << value->keyTime();
        encodeTimingFunction(encoder, value->timingFunction());
        switch (keyframes.property()) {
        case AnimatedPropertyOpacity:
            encoder << static_cast<const FloatAnimationValue*>(value)->value();
            break;
        case AnimatedPropertyWebkitTransform:
            encoder << *static_cast<const TransformAnimationValue*>(value)->value();
            break;
#if ENABLE(CSS_FILTERS)
        case AnimatedPropertyWebkitFilter:
            encoder << *static_cast<const FilterAnimationValue*>(value)->value();
            break;
#endif
        default:
            break;
        }
    }
}

bool ArgumentCoder<GraphicsLayerAnimation>::decode(ArgumentDecoder* decoder, GraphicsLayerAnimation& animation)
{
    String name;
    IntSize boxSize;
    GraphicsLayerAnimation::AnimationState state;
    double startTime;
    double pauseTime;
    bool listsMatch;

    Animation::AnimationDirection direction;
    unsigned fillMode;
    double duration;
    double iterationCount;
    RefPtr<TimingFunction> timingFunction;
    RefPtr<Animation> animationObject;

    if (!decoder->decode(name))
        return false;
    if (!decoder->decode(boxSize))
        return false;
    if (!decoder->decodeEnum(state))
        return false;
    if (!decoder->decodeDouble(startTime))
        return false;
    if (!decoder->decodeDouble(pauseTime))
        return false;
    if (!decoder->decodeBool(listsMatch))
        return false;
    if (!decoder->decodeEnum(direction))
        return false;
    if (!decoder->decodeUInt32(fillMode))
        return false;
    if (!decoder->decodeDouble(duration))
        return false;
    if (!decoder->decodeDouble(iterationCount))
        return false;
    if (!decodeTimingFunction(decoder, timingFunction))
        return false;

    animationObject = Animation::create();
    animationObject->setDirection(direction);
    animationObject->setFillMode(fillMode);
    animationObject->setDuration(duration);
    animationObject->setIterationCount(iterationCount);
    if (timingFunction)
        animationObject->setTimingFunction(timingFunction);

    AnimatedPropertyID property;
    if (!decoder->decodeEnum(property))
        return false;
    KeyframeValueList keyframes(property);
    unsigned keyframesSize;
    if (!decoder->decodeUInt32(keyframesSize))
        return false;
    for (unsigned i = 0; i < keyframesSize; ++i) {
        float keyTime;
        RefPtr<TimingFunction> timingFunction;
        if (!decoder->decode(keyTime))
            return false;
        if (!decodeTimingFunction(decoder, timingFunction))
            return false;

        switch (property) {
        case AnimatedPropertyOpacity: {
            float value;
            if (!decoder->decodeFloat(value))
                return false;
            keyframes.insert(new FloatAnimationValue(keyTime, value, timingFunction));
            break;
        }
        case AnimatedPropertyWebkitTransform: {
            TransformOperations transform;
            if (!decoder->decode(transform))
                return false;
            keyframes.insert(new TransformAnimationValue(keyTime, &transform, timingFunction));
            break;
        }
#if ENABLE(CSS_FILTERS)
        case AnimatedPropertyWebkitFilter: {
            FilterOperations filter;
            if (!decoder->decode(filter))
                return false;
            keyframes.insert(new FilterAnimationValue(keyTime, &filter, timingFunction));
            break;
        }
#endif
        default:
            break;
        }
    }

    animation = GraphicsLayerAnimation(name, keyframes, boxSize, animationObject.get(), startTime, listsMatch);
    animation.setState(state, pauseTime);

    return true;
}

void ArgumentCoder<GraphicsLayerAnimations>::encode(ArgumentEncoder& encoder, const GraphicsLayerAnimations& animations)
{
    encoder << animations.animations();
}

bool ArgumentCoder<GraphicsLayerAnimations>::decode(ArgumentDecoder* decoder, GraphicsLayerAnimations& animations)
{
    return decoder->decode(animations.animations());
}

#if USE(GRAPHICS_SURFACE)
void ArgumentCoder<WebCore::GraphicsSurfaceToken>::encode(ArgumentEncoder& encoder, const WebCore::GraphicsSurfaceToken& token)
{
#if OS(DARWIN)
    encoder << Attachment(token.frontBufferHandle, MACH_MSG_TYPE_MOVE_SEND);
    encoder << Attachment(token.backBufferHandle, MACH_MSG_TYPE_MOVE_SEND);
#elif OS(WINDOWS)
    encoder << reinterpret_cast<uint64_t>(token.frontBufferHandle);
    encoder << reinterpret_cast<uint64_t>(token.backBufferHandle);
#elif OS(LINUX)
    encoder << token.frontBufferHandle;
#endif
}

bool ArgumentCoder<WebCore::GraphicsSurfaceToken>::decode(ArgumentDecoder* decoder, WebCore::GraphicsSurfaceToken& token)
{
#if OS(WINDOWS)
    uint64_t frontBufferHandle;
    if (!decoder->decode(frontBufferHandle))
        return false;
    token.frontBufferHandle = reinterpret_cast<GraphicsSurfaceToken::BufferHandle>(frontBufferHandle);
    uint64_t backBufferHandle;
    if (!decoder->decode(backBufferHandle))
        return false;
    token.backBufferHandle = reinterpret_cast<GraphicsSurfaceToken::BufferHandle>(backBufferHandle);
#elif OS(DARWIN)
    Attachment frontAttachment, backAttachment;
    if (!decoder->decode(frontAttachment))
        return false;
    if (!decoder->decode(backAttachment))
        return false;

    token = GraphicsSurfaceToken(frontAttachment.port(), backAttachment.port());
#elif OS(LINUX)
    if (!decoder->decode(token.frontBufferHandle))
        return false;
#endif
    return true;
}
#endif

} // namespace CoreIPC

#endif // USE(COORDINATED_GRAPHICS)
