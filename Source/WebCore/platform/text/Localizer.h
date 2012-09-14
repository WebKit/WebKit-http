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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef Localizer_h
#define Localizer_h

#include <wtf/text/WTFString.h>

namespace WebCore {

class Localizer {
public:
    static PassOwnPtr<Localizer> create(const AtomicString&);
    String convertToLocalizedNumber(const String&);
    String convertFromLocalizedNumber(const String&);
#if ENABLE(INPUT_TYPE_TIME_MULTIPLE_FIELDS)
    String localizedDecimalSeparator();
    virtual String timeFormat();
    virtual String shortTimeFormat();
    virtual const Vector<String>& timeAMPMLabels();
#endif
    virtual ~Localizer();

protected:
    enum {
        // 0-9 for digits.
        DecimalSeparatorIndex = 10,
        GroupSeparatorIndex = 11,
        DecimalSymbolsSize
    };

#if ENABLE(INPUT_TYPE_TIME_MULTIPLE_FIELDS)
    String m_localizedTimeFormatText;
    String m_localizedShortTimeFormatText;
    Vector<String> m_timeAMPMLabels;
#endif

    Localizer() : m_hasLocalizerData(false) { }
    virtual void initializeLocalizerData() = 0;
    void setLocalizerData(const Vector<String, DecimalSymbolsSize>&, const String& positivePrefix, const String& positiveSuffix, const String& negativePrefix, const String& negativeSuffix);

private:
    bool detectSignAndGetDigitRange(const String& input, bool& isNegative, unsigned& startIndex, unsigned& endIndex);
    unsigned matchedDecimalSymbolIndex(const String& input, unsigned& position);

    String m_decimalSymbols[DecimalSymbolsSize];
    String m_positivePrefix;
    String m_positiveSuffix;
    String m_negativePrefix;
    String m_negativeSuffix;
    bool m_hasLocalizerData;
};

}
#endif
