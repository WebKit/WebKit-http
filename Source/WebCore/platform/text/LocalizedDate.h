/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#ifndef LocalizedDate_h
#define LocalizedDate_h

#include "DateComponents.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// Parses a string representation of a date string localized
// for the browser's current locale for a particular date type.
// If the input string is not valid or an implementation doesn't
// support localized dates, this function returns NaN. If the
// input string is valid this function returns the number
// of milliseconds since 1970-01-01 00:00:00.000 UTC.
double parseLocalizedDate(const String&, DateComponents::Type);

// Serializes the specified date into a formatted date string
// to display to the user. If an implementation doesn't support
// localized dates the function should return an empty string.
String formatLocalizedDate(const DateComponents& dateComponents);

#if ENABLE(CALENDAR_PICKER)
String localizedDateFormatText();

// Returns a vector of string of which size is 12. The first item is a
// localized string of January, and the last item is a localized
// string of December. These strings should not be abbreviations.
const Vector<String>& monthLabels();

// Returns a vector of string of which size is 7. The first item is a
// localized short string of Monday, and the last item is a localized
// short string of Saturday. These strings should be short.
const Vector<String>& weekDayShortLabels();

// The first day of a week. 0 is Sunday, and 6 is Saturday.
unsigned firstDayOfWeek();
#endif

#if ENABLE(INPUT_TYPE_TIME_MULTIPLE_FIELDS)
// Returns time format in Unicode TR35 LDML[1] containing hour, minute, and
// second with optional period(AM/PM), e.g. "h:mm:ss a"
// [1] LDML http://unicode.org/reports/tr35/tr35-6.html#Date_Format_Patterns
String localizedTimeFormatText();

// Returns time format in Unicode TR35 LDML containing hour, and minute
// with optional period(AM/PM), e.g. "h:mm a"
// Note: Some platforms return same value as localizedTimeFormatText().
String localizedShortTimeFormatText();

// Returns localized period field(AM/PM) strings.
const Vector<String>& timeAMPMLabels();
#endif
} // namespace WebCore

#endif // LocalizedDate_h
