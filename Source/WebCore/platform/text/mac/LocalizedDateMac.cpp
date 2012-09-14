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

#include "config.h"
#include "LocalizedDate.h"

#include "LocaleMac.h"
#include <limits>

using namespace std;

namespace WebCore {

double parseLocalizedDate(const String& input, DateComponents::Type type)
{
    switch (type) {
    case DateComponents::Date:
        return LocaleMac::currentLocale()->parseDate(input);
    case DateComponents::DateTime:
    case DateComponents::DateTimeLocal:
    case DateComponents::Month:
    case DateComponents::Time:
    case DateComponents::Week:
    case DateComponents::Invalid:
        break;
    }
    return numeric_limits<double>::quiet_NaN();
}

String formatLocalizedDate(const DateComponents& dateComponents)
{
    switch (dateComponents.type()) {
    case DateComponents::Date:
        return LocaleMac::currentLocale()->formatDate(dateComponents);
    case DateComponents::DateTime:
    case DateComponents::DateTimeLocal:
    case DateComponents::Month:
    case DateComponents::Time:
    case DateComponents::Week:
    case DateComponents::Invalid:
        break;
    }
    return String();
}

#if ENABLE(CALENDAR_PICKER)
String localizedDateFormatText()
{
    return LocaleMac::currentLocale()->dateFormatText();
}

const Vector<String>& monthLabels()
{
    return LocaleMac::currentLocale()->monthLabels();
}

const Vector<String>& weekDayShortLabels()
{
    return LocaleMac::currentLocale()->weekDayShortLabels();
}

unsigned firstDayOfWeek()
{
    return LocaleMac::currentLocale()->firstDayOfWeek();
}
#endif

#if ENABLE(INPUT_TYPE_TIME_MULTIPLE_FIELDS)
String localizedTimeFormatText()
{
    return LocaleMac::currentLocale()->timeFormat();
}

String localizedShortTimeFormatText()
{
    return LocaleMac::currentLocale()->shortTimeFormat();
}

const Vector<String>& timeAMPMLabels()
{
    return LocaleMac::currentLocale()->timeAMPMLabels();
}
#endif

}
