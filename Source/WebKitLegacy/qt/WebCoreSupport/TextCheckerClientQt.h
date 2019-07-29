/*
 * Copyright (C) 2011 Lindsay Mathieson <lindsay.mathieson@gmail.com>
 * Copyright (C) 2011 Dawit Alemayehu  <adawit@kde.org>
 *
 * All rights reserved.
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

#ifndef TextCheckerClientQt_h
#define TextCheckerClientQt_h

#include "QtPlatformPlugin.h"
#include "qwebkitplatformplugin.h"

#include <WebCore/TextCheckerClient.h>
#include <wtf/Forward.h>

namespace WebCore {

class TextCheckerClientQt final : public TextCheckerClient {
public:
    bool shouldEraseMarkersAfterChangeSelection(TextCheckingType) const final;
    void ignoreWordInSpellDocument(const String&) final;
    void learnWord(const String&) final;
    void checkSpellingOfString(StringView, int* misspellingLocation, int* misspellingLength) final;
    String getAutoCorrectSuggestionForMisspelledWord(const String& misspelledWord) final;
    void checkGrammarOfString(StringView, Vector<GrammarDetail>&, int* badGrammarLocation, int* badGrammarLength) final;
    void getGuessesForWord(const String& word, const String& context, const VisibleSelection&, Vector<String>& guesses) final;
    void requestCheckingOfString(WebCore::TextCheckingRequest&, const VisibleSelection&) final { }

    virtual bool isContinousSpellCheckingEnabled();
    virtual void toggleContinousSpellChecking();

    virtual bool isGrammarCheckingEnabled();
    virtual void toggleGrammarChecking();

private:
    bool loadSpellChecker();

private:
    QtPlatformPlugin m_platformPlugin;
    std::unique_ptr<QWebSpellChecker> m_spellChecker;
};

}

#endif
