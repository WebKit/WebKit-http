/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "AnimationTranslationUtil.h"

#include "FloatSize.h"
#include "GraphicsLayer.h"
#include "IdentityTransformOperation.h"
#include "Length.h"
#include "LengthFunctions.h"
#include "Matrix3DTransformOperation.h"
#include "MatrixTransformOperation.h"
#include "PerspectiveTransformOperation.h"
#include "RotateTransformOperation.h"
#include "ScaleTransformOperation.h"
#include "SkewTransformOperation.h"
#include "TransformOperations.h"
#include "TranslateTransformOperation.h"

#include "cc/CCActiveAnimation.h"
#include "cc/CCKeyframedAnimationCurve.h"

#include <public/WebTransformOperations.h>
#include <public/WebTransformationMatrix.h>

#include <wtf/OwnPtr.h>
#include <wtf/text/CString.h>

using namespace std;
using namespace WebKit;

namespace WebCore {

WebTransformOperations toWebTransformOperations(const TransformOperations& transformOperations, const FloatSize& boxSize)
{
    // We need to do a deep copy the transformOperations may contain ref pointers to TransformOperation objects.
    WebTransformOperations webTransformOperations;
    for (size_t j = 0; j < transformOperations.size(); ++j) {
        TransformOperation::OperationType operationType = transformOperations.operations()[j]->getOperationType();
        switch (operationType) {
        case TransformOperation::SCALE_X:
        case TransformOperation::SCALE_Y:
        case TransformOperation::SCALE_Z:
        case TransformOperation::SCALE_3D:
        case TransformOperation::SCALE: {
            ScaleTransformOperation* transform = static_cast<ScaleTransformOperation*>(transformOperations.operations()[j].get());
            webTransformOperations.appendScale(transform->x(), transform->y(), transform->z());
            break;
        }
        case TransformOperation::TRANSLATE_X:
        case TransformOperation::TRANSLATE_Y:
        case TransformOperation::TRANSLATE_Z:
        case TransformOperation::TRANSLATE_3D:
        case TransformOperation::TRANSLATE: {
            TranslateTransformOperation* transform = static_cast<TranslateTransformOperation*>(transformOperations.operations()[j].get());
            webTransformOperations.appendTranslate(floatValueForLength(transform->x(), boxSize.width()), floatValueForLength(transform->y(), boxSize.height()), floatValueForLength(transform->z(), 1));
            break;
        }
        case TransformOperation::ROTATE_X:
        case TransformOperation::ROTATE_Y:
        case TransformOperation::ROTATE_3D:
        case TransformOperation::ROTATE: {
            RotateTransformOperation* transform = static_cast<RotateTransformOperation*>(transformOperations.operations()[j].get());
            webTransformOperations.appendRotate(transform->x(), transform->y(), transform->z(), transform->angle());
            break;
        }
        case TransformOperation::SKEW_X:
        case TransformOperation::SKEW_Y:
        case TransformOperation::SKEW: {
            SkewTransformOperation* transform = static_cast<SkewTransformOperation*>(transformOperations.operations()[j].get());
            webTransformOperations.appendSkew(transform->angleX(), transform->angleY());
            break;
        }
        case TransformOperation::MATRIX: {
            MatrixTransformOperation* transform = static_cast<MatrixTransformOperation*>(transformOperations.operations()[j].get());
            TransformationMatrix m = transform->matrix();
            webTransformOperations.appendMatrix(WebTransformationMatrix(m));
            break;
        }
        case TransformOperation::MATRIX_3D: {
            Matrix3DTransformOperation* transform = static_cast<Matrix3DTransformOperation*>(transformOperations.operations()[j].get());
            TransformationMatrix m = transform->matrix();
            webTransformOperations.appendMatrix(WebTransformationMatrix(m));
            break;
        }
        case TransformOperation::PERSPECTIVE: {
            PerspectiveTransformOperation* transform = static_cast<PerspectiveTransformOperation*>(transformOperations.operations()[j].get());
            webTransformOperations.appendPerspective(floatValueForLength(transform->perspective(), 0));
            break;
        }
        case TransformOperation::IDENTITY:
        case TransformOperation::NONE:
            // Do nothing.
            break;
        } // switch
    } // for each operation

    return webTransformOperations;
}

template <class Value, class Keyframe, class Curve>
bool appendKeyframe(Curve& curve, double keyTime, const Value* value, PassOwnPtr<CCTimingFunction> timingFunction, const FloatSize&)
{
    curve.addKeyframe(Keyframe::create(keyTime, value->value(), timingFunction));
    return true;
}

template <>
bool appendKeyframe<TransformAnimationValue, CCTransformKeyframe, CCKeyframedTransformAnimationCurve>(CCKeyframedTransformAnimationCurve& curve, double keyTime, const TransformAnimationValue* value, PassOwnPtr<CCTimingFunction> timingFunction, const FloatSize& boxSize)
{
    WebTransformOperations operations = toWebTransformOperations(*value->value(), boxSize);
    if (operations.apply().isInvertible()) {
        curve.addKeyframe(CCTransformKeyframe::create(keyTime, operations, timingFunction));
        return true;
    }
    return false;
}

template <class Value, class Keyframe, class Curve>
PassOwnPtr<CCActiveAnimation> createActiveAnimation(const KeyframeValueList& valueList, const Animation* animation, size_t animationId, size_t groupId, double timeOffset, CCActiveAnimation::TargetProperty targetProperty, const FloatSize& boxSize)
{
    bool alternate = false;
    bool reverse = false;
    if (animation && animation->isDirectionSet()) {
        Animation::AnimationDirection direction = animation->direction();
        if (direction == Animation::AnimationDirectionAlternate || direction == Animation::AnimationDirectionAlternateReverse)
            alternate = true;
        if (direction == Animation::AnimationDirectionReverse || direction == Animation::AnimationDirectionAlternateReverse)
            reverse = true;
    }

    OwnPtr<Curve> curve = Curve::create();
    Vector<Keyframe> keyframes;

    for (size_t i = 0; i < valueList.size(); i++) {
        size_t index = reverse ? valueList.size() - i - 1 : i;

        const Value* originalValue = static_cast<const Value*>(valueList.at(index));

        OwnPtr<CCTimingFunction> timingFunction;
        const TimingFunction* originalTimingFunction = originalValue->timingFunction();

        // If there hasn't been a timing function associated with this keyframe, use the
        // animation's timing function, if we have one.
        if (!originalTimingFunction && animation->isTimingFunctionSet())
            originalTimingFunction = animation->timingFunction().get();

        if (originalTimingFunction) {
            switch (originalTimingFunction->type()) {
            case TimingFunction::StepsFunction:
                // FIXME: add support for steps timing function.
                return nullptr;
            case TimingFunction::LinearFunction:
                // Don't set the timing function. Keyframes are interpolated linearly if there is no timing function.
                break;
            case TimingFunction::CubicBezierFunction:
                const CubicBezierTimingFunction* originalBezierTimingFunction = static_cast<const CubicBezierTimingFunction*>(originalTimingFunction);
                timingFunction = CCCubicBezierTimingFunction::create(originalBezierTimingFunction->x1(), originalBezierTimingFunction->y1(), originalBezierTimingFunction->x2(), originalBezierTimingFunction->y2());
                break;
            } // switch
        } else
            timingFunction = CCEaseTimingFunction::create();

        double duration = (animation && animation->isDurationSet()) ? animation->duration() : 1;
        double keyTime = originalValue->keyTime() * duration;

        if (reverse)
            keyTime = duration - keyTime;

        bool addedKeyframe = appendKeyframe<Value, Keyframe, Curve>(*curve, keyTime, originalValue, timingFunction.release(), boxSize);
        if (!addedKeyframe)
            return nullptr;
    }

    OwnPtr<CCActiveAnimation> anim = CCActiveAnimation::create(curve.release(), animationId, groupId, targetProperty);

    ASSERT(anim.get());

    if (anim.get()) {
        int iterations = (animation && animation->isIterationCountSet()) ? animation->iterationCount() : 1;
        anim->setIterations(iterations);
        anim->setAlternatesDirection(alternate);
    }

    // In order to avoid skew, the main thread animation cannot tick until it has received the start time of
    // the corresponding impl thread animation.
    anim->setNeedsSynchronizedStartTime(true);

    // If timeOffset > 0, then the animation has started in the past.
    anim->setTimeOffset(timeOffset);

    return anim.release();
}

PassOwnPtr<CCActiveAnimation> createActiveAnimation(const KeyframeValueList& values, const Animation* animation, size_t animationId, size_t groupId, double timeOffset, const FloatSize& boxSize)
{
    if (values.property() == AnimatedPropertyWebkitTransform)
        return createActiveAnimation<TransformAnimationValue, CCTransformKeyframe, CCKeyframedTransformAnimationCurve>(values, animation, animationId, groupId, timeOffset, CCActiveAnimation::Transform, FloatSize(boxSize));

    if (values.property() == AnimatedPropertyOpacity)
        return createActiveAnimation<FloatAnimationValue, CCFloatKeyframe, CCKeyframedFloatAnimationCurve>(values, animation, animationId, groupId, timeOffset, CCActiveAnimation::Opacity, FloatSize());

    return nullptr;
}

} // namespace WebCore
