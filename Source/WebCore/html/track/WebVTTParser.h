/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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

#include "DocumentFragment.h"
#include "HTMLNames.h"
#include "TextTrackCue.h"
#include "WebVTTTokenizer.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

class Document;

class WebVTTParserClient {
public:
    virtual ~WebVTTParserClient() { }
    
    virtual void newCuesParsed() = 0;
    virtual void fileFailedToParse() = 0;
};

class WebVTTParser {
public:
    virtual ~WebVTTParser() { }
    
    enum ParseState { Initial, Header, Id, TimingsAndSettings, CueText, BadCue };

    static PassOwnPtr<WebVTTParser> create(WebVTTParserClient* client, ScriptExecutionContext* context)
    {
        return adoptPtr(new WebVTTParser(client, context));
    }
    
    static inline bool isRecognizedTag(const AtomicString& tagName)
    {
        return tagName == iTag
            || tagName == bTag
            || tagName == uTag
            || tagName == rubyTag
            || tagName == rtTag;
    }

    static inline bool isASpace(char c)
    {
        // WebVTT space characters are U+0020 SPACE, U+0009 CHARACTER TABULATION (tab), U+000A LINE FEED (LF), U+000C FORM FEED (FF), and U+000D CARRIAGE RETURN    (CR).
        return c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r';
    }
    static inline bool isValidSettingDelimiter(char c)
    {
        // ... a WebVTT cue consists of zero or more of the following components, in any order, separated from each other by one or more 
        // U+0020 SPACE characters or U+0009 CHARACTER TABULATION (tab) characters.
        return c == ' ' || c == '\t';
    }
    static String collectDigits(const String&, unsigned*);
    static String collectWord(const String&, unsigned*);

    // Input data to the parser to parse.
    void parseBytes(const char* data, unsigned length);

    // Transfers ownership of last parsed cues to caller.
    void getNewCues(Vector<RefPtr<TextTrackCue> >&);

    PassRefPtr<DocumentFragment> createDocumentFragmentFromCueText(const String&);
    double collectTimeStamp(const String&, unsigned*);

protected:
    WebVTTParser(WebVTTParserClient*, ScriptExecutionContext*);
    
    ScriptExecutionContext* m_scriptExecutionContext;
    ParseState m_state;

private:
    bool hasRequiredFileIdentifier();
    ParseState collectCueId(const String&);
    ParseState collectTimingsAndSettings(const String&);
    ParseState collectCueText(const String&, unsigned length, unsigned);
    ParseState ignoreBadCue(const String&);

    void createNewCue();
    void resetCueValues();

    void skipWhiteSpace(const String&, unsigned*);
    static void skipLineTerminator(const char* data, unsigned length, unsigned*);
    static String collectNextLine(const char* data, unsigned length, unsigned*);
    
    void constructTreeFromToken(Document*);

    Vector<char> m_identifierData;
    String m_currentId;
    double m_currentStartTime;
    double m_currentEndTime;
    StringBuilder m_currentContent;
    String m_currentSettings;
    
    WebVTTToken m_token;
    OwnPtr<WebVTTTokenizer> m_tokenizer;

    RefPtr<ContainerNode> m_currentNode;

    WebVTTParserClient* m_client;

    Vector<RefPtr<TextTrackCue> > m_cuelist;
};

} // namespace WebCore

#endif
#endif
