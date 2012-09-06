/*
 * CSS Media Query Evaluator
 *
 * Copyright (C) 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
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

#ifndef MediaQueryEvaluator_h
#define MediaQueryEvaluator_h

#include <wtf/text/WTFString.h>

namespace WebCore {
class Frame;
class MediaQueryExp;
class MediaQuerySet;
class RenderStyle;
class StyleResolver;

/**
 * Class that evaluates css media queries as defined in
 * CSS3 Module "Media Queries" (http://www.w3.org/TR/css3-mediaqueries/)
 * Special constructors are needed, if simple media queries are to be
 * evaluated without knowledge of the medium features. This can happen
 * for example when parsing UA stylesheets, if evaluation is done
 * right after parsing.
 *
 * the boolean parameter is used to approximate results of evaluation, if
 * the device characteristics are not known. This can be used to prune the loading
 * of stylesheets to only those which are probable to match.
 */
class MediaQueryEvaluator {
     WTF_MAKE_NONCOPYABLE(MediaQueryEvaluator); WTF_MAKE_FAST_ALLOCATED;
public:
    /** Creates evaluator which evaluates only simple media queries
     *  Evaluator returns true for "all", and returns value of \mediaFeatureResult
     *  for any media features
     */
    MediaQueryEvaluator(bool mediaFeatureResult = false);

    /** Creates evaluator which evaluates only simple media queries
     *  Evaluator  returns true for acceptedMediaType and returns value of \mediafeatureResult
     *  for any media features
     */
    MediaQueryEvaluator(const String& acceptedMediaType, bool mediaFeatureResult = false);
    MediaQueryEvaluator(const char* acceptedMediaType, bool mediaFeatureResult = false);

    /** Creates evaluator which evaluates full media queries
     */
    MediaQueryEvaluator(const String& acceptedMediaType, Frame*, RenderStyle*);

    ~MediaQueryEvaluator();

    bool mediaTypeMatch(const String& mediaTypeToMatch) const;
    bool mediaTypeMatchSpecific(const char* mediaTypeToMatch) const;

    /** Evaluates a list of media queries */
    bool eval(const MediaQuerySet*, StyleResolver* = 0) const;

    /** Evaluates media query subexpression, ie "and (media-feature: value)" part */
    bool eval(const MediaQueryExp*) const;

private:
    String m_mediaType;
    Frame* m_frame; // not owned
    RefPtr<RenderStyle> m_style;
    bool m_expResult;
};

} // namespace
#endif
