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
#include "DateTimeChooserImpl.h"

#if ENABLE(CALENDAR_PICKER)

#include "CalendarPicker.h"
#include "ChromeClientImpl.h"
#include "DateComponents.h"
#include "DateTimeChooserClient.h"
#include "Language.h"
#include "LocalizedDate.h"
#include "NotImplemented.h"
#include "PickerCommon.h"
#include "RenderTheme.h"
#include "platform/WebKitPlatformSupport.h"
#include <public/WebLocalizedString.h>

using namespace WTF::Unicode;

namespace WebKit {

DateTimeChooserImpl::DateTimeChooserImpl(ChromeClientImpl* chromeClient, WebCore::DateTimeChooserClient* client, const WebCore::DateTimeChooserParameters& parameters)
    : m_chromeClient(chromeClient)
    , m_client(client)
    , m_popup(0)
    , m_parameters(parameters)
{
    ASSERT(m_chromeClient);
    ASSERT(m_client);
    m_popup = m_chromeClient->openPagePopup(this, m_parameters.anchorRectInRootView);
}

DateTimeChooserImpl::~DateTimeChooserImpl()
{
}

void DateTimeChooserImpl::endChooser()
{
    if (!m_popup)
        return;
    m_chromeClient->closePagePopup(m_popup);
}

WebCore::IntSize DateTimeChooserImpl::contentSize()
{
    return WebCore::IntSize(0, 0);
}

void DateTimeChooserImpl::writeDocument(WebCore::DocumentWriter& writer)
{
    WebCore::DateComponents date;
    date.setMillisecondsSinceEpochForDate(m_parameters.minimum);
    String minString = date.toString();
    date.setMillisecondsSinceEpochForDate(m_parameters.maximum);
    String maxString = date.toString();
    String stepString = String::number(m_parameters.step);

    addString("<!DOCTYPE html><head><meta charset='UTF-8'><style>\n", writer);
    writer.addData(WebCore::pickerCommonCss, sizeof(WebCore::pickerCommonCss));
    writer.addData(WebCore::calendarPickerCss, sizeof(WebCore::calendarPickerCss));
    CString extraStyle = WebCore::RenderTheme::defaultTheme()->extraCalendarPickerStyleSheet();
    if (extraStyle.length())
        writer.addData(extraStyle.data(), extraStyle.length());
    addString("</style></head><body><div id=main>Loading...</div><script>\n"
               "window.dialogArguments = {\n", writer);
    addProperty("min", minString, writer);
    addProperty("max", maxString, writer);
    addProperty("step", stepString, writer);
    addProperty("required", m_parameters.required, writer);
    addProperty("currentValue", m_parameters.currentValue, writer);
    addProperty("locale", WebCore::defaultLanguage(), writer);
    addProperty("todayLabel", Platform::current()->queryLocalizedString(WebLocalizedString::CalendarToday), writer);
    addProperty("clearLabel", Platform::current()->queryLocalizedString(WebLocalizedString::CalendarClear), writer);
    addProperty("weekStartDay", WebCore::firstDayOfWeek(), writer);
    addProperty("monthLabels", WebCore::monthLabels(), writer);
    addProperty("dayLabels", WebCore::weekDayShortLabels(), writer);
    Direction dir = direction(WebCore::monthLabels()[0][0]);
    addProperty("isRTL", dir == RightToLeft || dir == RightToLeftArabic, writer);
    addString("}\n", writer);

    writer.addData(WebCore::pickerCommonJs, sizeof(WebCore::pickerCommonJs));
    writer.addData(WebCore::calendarPickerJs, sizeof(WebCore::calendarPickerJs));
    addString("</script></body>\n", writer);
}

void DateTimeChooserImpl::setValueAndClosePopup(int numValue, const String& stringValue)
{
    if (numValue >= 0)
        m_client->didChooseValue(stringValue);
    endChooser();
}

void DateTimeChooserImpl::didClosePopup()
{
    ASSERT(m_client);
    m_popup = 0;
    m_client->didEndChooser();
}

} // namespace WebKit

#endif // ENABLE(CALENDAR_PICKER)
