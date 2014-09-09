/*
 * Copyright (C) 2011, 2013 Google Inc.  All rights reserved.
 * Copyright (C) 2013 Cable Television Labs, Inc.
 * Copyright (C) 2014 Apple Inc.  All rights reserved.
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

#ifndef WebVTTParser_h
#define WebVTTParser_h

#if ENABLE(VIDEO_TRACK)

#include "BufferedLineReader.h"
#include "DocumentFragment.h"
#include "HTMLNames.h"
#include "TextResourceDecoder.h"
#include "VTTRegion.h"
#include "WebVTTTokenizer.h"
#include <memory>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

class Document;
class ISOWebVTTCue;
class VTTScanner;

class WebVTTParserClient {
public:
    virtual ~WebVTTParserClient() { }

    virtual void newCuesParsed() = 0;
#if ENABLE(WEBVTT_REGIONS)
    virtual void newRegionsParsed() = 0;
#endif
    virtual void fileFailedToParse() = 0;
};

class WebVTTCueData : public RefCounted<WebVTTCueData> {
public:

    static PassRefPtr<WebVTTCueData> create() { return adoptRef(new WebVTTCueData()); }
    virtual ~WebVTTCueData() { }

    double startTime() const { return m_startTime; }
    void setStartTime(double startTime) { m_startTime = startTime; }

    double endTime() const { return m_endTime; }
    void setEndTime(double endTime) { m_endTime = endTime; }

    String id() const { return m_id; }
    void setId(String id) { m_id = id; }

    String content() const { return m_content; }
    void setContent(String content) { m_content = content; }

    String settings() const { return m_settings; }
    void setSettings(String settings) { m_settings = settings; }

    double originalStartTime() const { return m_originalStartTime; }
    void setOriginalStartTime(double time) { m_originalStartTime = time; }

private:
    WebVTTCueData()
        : m_startTime(0)
        , m_endTime(0)
        , m_originalStartTime(0)
    {
    }

    double m_startTime;
    double m_endTime;
    double m_originalStartTime;
    String m_id;
    String m_content;
    String m_settings;
};

class WebVTTParser final {
public:
    enum ParseState {
        Initial,
        Header,
        Id,
        TimingsAndSettings,
        CueText,
        BadCue,
        Finished
    };

    WebVTTParser(WebVTTParserClient*, ScriptExecutionContext*);

    static inline bool isRecognizedTag(const AtomicString& tagName)
    {
        return tagName == iTag
            || tagName == bTag
            || tagName == uTag
            || tagName == rubyTag
            || tagName == rtTag;
    }

    static inline bool isASpace(UChar c)
    {
        // WebVTT space characters are U+0020 SPACE, U+0009 CHARACTER TABULATION (tab), U+000A LINE FEED (LF), U+000C FORM FEED (FF), and U+000D CARRIAGE RETURN    (CR).
        return c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r';
    }
    static inline bool isValidSettingDelimiter(UChar c)
    {
        // ... a WebVTT cue consists of zero or more of the following components, in any order, separated from each other by one or more 
        // U+0020 SPACE characters or U+0009 CHARACTER TABULATION (tab) characters.
        return c == ' ' || c == '\t';
    }
    static bool collectTimeStamp(const String&, double&);

    // Useful functions for parsing percentage settings.
    static bool parseFloatPercentageValue(VTTScanner& valueScanner, float&);
#if ENABLE(WEBVTT_REGIONS)
    static bool parseFloatPercentageValuePair(VTTScanner& valueScanner, char, FloatPoint&);
#endif

    // Input data to the parser to parse.
    void parseBytes(const char*, unsigned);
    void parseFileHeader(const String&);
    void parseCueData(const ISOWebVTTCue&);
    void flush();
    void fileFinished();

    // Transfers ownership of last parsed cues to caller.
    void getNewCues(Vector<RefPtr<WebVTTCueData>>&);
#if ENABLE(WEBVTT_REGIONS)
    void getNewRegions(Vector<RefPtr<VTTRegion>>&);
#endif

    // Create the DocumentFragment representation of the WebVTT cue text.
    static PassRefPtr<DocumentFragment> createDocumentFragmentFromCueText(Document&, const String&);

protected:
    ScriptExecutionContext* m_scriptExecutionContext;
    ParseState m_state;

private:
    void parse();
    void flushPendingCue();
    bool hasRequiredFileIdentifier(const String&);
    ParseState collectCueId(const String&);
    ParseState collectTimingsAndSettings(const String&);
    ParseState collectCueText(const String&);
    ParseState recoverCue(const String&);
    ParseState ignoreBadCue(const String&);

    void createNewCue();
    void resetCueValues();

    void collectMetadataHeader(const String&);
#if ENABLE(WEBVTT_REGIONS)
    void createNewRegion(const String& headerValue);
#endif

    static bool collectTimeStamp(VTTScanner& input, double& timeStamp);

    BufferedLineReader m_lineReader;
    RefPtr<TextResourceDecoder> m_decoder;
    String m_currentId;
    double m_currentStartTime;
    double m_currentEndTime;
    StringBuilder m_currentContent;
    String m_currentSettings;

    WebVTTParserClient* m_client;

    Vector<RefPtr<WebVTTCueData>> m_cuelist;

#if ENABLE(WEBVTT_REGIONS)
    Vector<RefPtr<VTTRegion>> m_regionList;
#endif
};

} // namespace WebCore

#endif
#endif
