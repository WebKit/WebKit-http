/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LocaleICU_h
#define LocaleICU_h

#include "DateComponents.h"
#include "PlatformLocale.h"
#include <unicode/udat.h>
#include <unicode/unum.h>
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// We should use this class only for LocalizedNumberICU.cpp, LocalizedDateICU.cpp,
// and LocalizedNumberICUTest.cpp.
class LocaleICU : public Locale {
public:
    static PassOwnPtr<LocaleICU> create(const char* localeString);
    virtual ~LocaleICU();

#if ENABLE(CALENDAR_PICKER)
    virtual const Vector<String>& weekDayShortLabels() OVERRIDE;
    virtual unsigned firstDayOfWeek() OVERRIDE;
    virtual bool isRTL() OVERRIDE;
#endif
#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    virtual String dateFormat() OVERRIDE;
    virtual String monthFormat() OVERRIDE;
    virtual String timeFormat() OVERRIDE;
    virtual String shortTimeFormat() OVERRIDE;
    virtual String dateTimeFormatWithSeconds() OVERRIDE;
    virtual String dateTimeFormatWithoutSeconds() OVERRIDE;
    virtual const Vector<String>& monthLabels() OVERRIDE;
    virtual const Vector<String>& shortMonthLabels() OVERRIDE;
    virtual const Vector<String>& standAloneMonthLabels() OVERRIDE;
    virtual const Vector<String>& shortStandAloneMonthLabels() OVERRIDE;
    virtual const Vector<String>& timeAMPMLabels() OVERRIDE;
#endif

private:
    explicit LocaleICU(const char*);
    String decimalSymbol(UNumberFormatSymbol);
    String decimalTextAttribute(UNumberFormatTextAttribute);
    virtual void initializeLocaleData() OVERRIDE;

    bool detectSignAndGetDigitRange(const String& input, bool& isNegative, unsigned& startIndex, unsigned& endIndex);
    unsigned matchedDecimalSymbolIndex(const String& input, unsigned& position);

    bool initializeShortDateFormat();
    UDateFormat* openDateFormat(UDateFormatStyle timeStyle, UDateFormatStyle dateStyle) const;

#if ENABLE(CALENDAR_PICKER)
    void initializeCalendar();
#endif

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    PassOwnPtr<Vector<String> > createLabelVector(const UDateFormat*, UDateFormatSymbolType, int32_t startIndex, int32_t size);
    void initializeDateTimeFormat();
#endif

    CString m_locale;
    UNumberFormat* m_numberFormat;
    UDateFormat* m_shortDateFormat;
    bool m_didCreateDecimalFormat;
    bool m_didCreateShortDateFormat;

#if ENABLE(CALENDAR_PICKER)
    OwnPtr<Vector<String> > m_weekDayShortLabels;
    unsigned m_firstDayOfWeek;
#endif
#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    OwnPtr<Vector<String> > m_monthLabels;
    String m_dateFormat;
    String m_monthFormat;
    String m_timeFormatWithSeconds;
    String m_timeFormatWithoutSeconds;
    String m_dateTimeFormatWithSeconds;
    String m_dateTimeFormatWithoutSeconds;
    UDateFormat* m_mediumTimeFormat;
    UDateFormat* m_shortTimeFormat;
    Vector<String> m_shortMonthLabels;
    Vector<String> m_standAloneMonthLabels;
    Vector<String> m_shortStandAloneMonthLabels;
    Vector<String> m_timeAMPMLabels;
    bool m_didCreateTimeFormat;
#endif
};

} // namespace WebCore
#endif
