/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011-2012 Samsung Electronics
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
#include "TextChecker.h"

#include "TextCheckerState.h"

#if ENABLE(SPELLCHECK)
#include "WebTextChecker.h"
#endif

using namespace WebCore;

namespace WebKit {

static TextCheckerState textCheckerState;

const TextCheckerState& TextChecker::state()
{
#if ENABLE(SPELLCHECK)
    static bool didInitializeState = false;
    if (didInitializeState)
        return textCheckerState;

    WebTextCheckerClient& client = WebTextChecker::shared()->client();
    textCheckerState.isContinuousSpellCheckingEnabled = client.continuousSpellCheckingEnabled();
    textCheckerState.isGrammarCheckingEnabled = client.grammarCheckingEnabled();

    didInitializeState = true;
#endif
    return textCheckerState;
}

bool TextChecker::isContinuousSpellCheckingAllowed()
{
#if ENABLE(SPELLCHECK)
    return WebTextChecker::shared()->client().continuousSpellCheckingAllowed();
#else
    return false;
#endif
}

void TextChecker::setContinuousSpellCheckingEnabled(bool isContinuousSpellCheckingEnabled)
{
#if ENABLE(SPELLCHECK)
    if (state().isContinuousSpellCheckingEnabled == isContinuousSpellCheckingEnabled)
        return;

    textCheckerState.isContinuousSpellCheckingEnabled = isContinuousSpellCheckingEnabled;
    WebTextChecker::shared()->client().setContinuousSpellCheckingEnabled(isContinuousSpellCheckingEnabled);
#else
    UNUSED_PARAM(isContinuousSpellCheckingEnabled);
#endif
}

void TextChecker::setGrammarCheckingEnabled(bool isGrammarCheckingEnabled)
{
#if ENABLE(SPELLCHECK)
    if (state().isGrammarCheckingEnabled == isGrammarCheckingEnabled)
        return;

    textCheckerState.isGrammarCheckingEnabled = isGrammarCheckingEnabled;
    WebTextChecker::shared()->client().setGrammarCheckingEnabled(isGrammarCheckingEnabled);
#else
    UNUSED_PARAM(isGrammarCheckingEnabled);
#endif
}

void TextChecker::continuousSpellCheckingEnabledStateChanged(bool enabled)
{
    TextChecker::setContinuousSpellCheckingEnabled(enabled);
}

void TextChecker::grammarCheckingEnabledStateChanged(bool enabled)
{
#if ENABLE(SPELLCHECK)
    textCheckerState.isGrammarCheckingEnabled = enabled;
#else
    UNUSED_PARAM(enabled);
#endif
}

int64_t TextChecker::uniqueSpellDocumentTag(WebPageProxy* page)
{
#if ENABLE(SPELLCHECK)
    return WebTextChecker::shared()->client().uniqueSpellDocumentTag(page);
#else
    UNUSED_PARAM(page);
    return 0;
#endif
}

void TextChecker::closeSpellDocumentWithTag(int64_t tag)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().closeSpellDocumentWithTag(tag);
#else
    UNUSED_PARAM(tag);
#endif
}

void TextChecker::checkSpellingOfString(int64_t spellDocumentTag, const UChar* text, uint32_t length, int32_t& misspellingLocation, int32_t& misspellingLength)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().checkSpellingOfString(spellDocumentTag, String(text, length), misspellingLocation, misspellingLength);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(text);
    UNUSED_PARAM(length);
    UNUSED_PARAM(misspellingLocation);
    UNUSED_PARAM(misspellingLength);
#endif
}

void TextChecker::checkGrammarOfString(int64_t spellDocumentTag, const UChar* text, uint32_t length, Vector<GrammarDetail>& grammarDetails, int32_t& badGrammarLocation, int32_t& badGrammarLength)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().checkGrammarOfString(spellDocumentTag, String(text, length), grammarDetails, badGrammarLocation, badGrammarLength);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(text);
    UNUSED_PARAM(length);
    UNUSED_PARAM(grammarDetails);
    UNUSED_PARAM(badGrammarLocation);
    UNUSED_PARAM(badGrammarLength);
#endif
}

bool TextChecker::spellingUIIsShowing()
{
#if ENABLE(SPELLCHECK)
    return WebTextChecker::shared()->client().spellingUIIsShowing();
#else
    return false;
#endif
}

void TextChecker::toggleSpellingUIIsShowing()
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().toggleSpellingUIIsShowing();
#endif
}

void TextChecker::updateSpellingUIWithMisspelledWord(int64_t spellDocumentTag, const String& misspelledWord)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().updateSpellingUIWithMisspelledWord(spellDocumentTag, misspelledWord);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(misspelledWord);
#endif
}

void TextChecker::updateSpellingUIWithGrammarString(int64_t spellDocumentTag, const String& badGrammarPhrase, const GrammarDetail& grammarDetail)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().updateSpellingUIWithGrammarString(spellDocumentTag, badGrammarPhrase, grammarDetail);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(badGrammarPhrase);
    UNUSED_PARAM(grammarDetail);
#endif
}

void TextChecker::getGuessesForWord(int64_t spellDocumentTag, const String& word, const String& , Vector<String>& guesses)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().guessesForWord(spellDocumentTag, word, guesses);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(word);
    UNUSED_PARAM(guesses);
#endif
}

void TextChecker::learnWord(int64_t spellDocumentTag, const String& word)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().learnWord(spellDocumentTag, word);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(word);
#endif
}

void TextChecker::ignoreWord(int64_t spellDocumentTag, const String& word)
{
#if ENABLE(SPELLCHECK)
    WebTextChecker::shared()->client().ignoreWord(spellDocumentTag, word);
#else
    UNUSED_PARAM(spellDocumentTag);
    UNUSED_PARAM(word);
#endif
}

} // namespace WebKit
