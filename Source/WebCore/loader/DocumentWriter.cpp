/*
 * Copyright (C) 2010. Adam Barth. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DocumentWriter.h"

#include "DOMImplementation.h"
#include "DOMWindow.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderStateMachine.h"
#include "FrameView.h"
#include "PlaceholderDocument.h"
#include "PluginDocument.h"
#include "RawDataDocumentParser.h"
#include "ScriptableDocumentParser.h"
#include "SecurityOrigin.h"
#include "SegmentedString.h"
#include "Settings.h"
#include "SinkDocument.h"
#include "TextResourceDecoder.h"


namespace WebCore {

static inline bool canReferToParentFrameEncoding(const Frame* frame, const Frame* parentFrame) 
{
    return parentFrame && parentFrame->document()->securityOrigin()->canAccess(frame->document()->securityOrigin());
}
    
DocumentWriter::DocumentWriter(Frame* frame)
    : m_frame(frame)
    , m_hasReceivedSomeData(false)
    , m_encodingWasChosenByUser(false)
{
}

// This is only called by ScriptController::executeIfJavaScriptURL
// and always contains the result of evaluating a javascript: url.
// This is the <iframe src="javascript:'html'"> case.
void DocumentWriter::replaceDocument(const String& source)
{
    m_frame->loader()->stopAllLoaders();
    begin(m_frame->document()->url(), true, InheritSecurityOrigin);

    if (!source.isNull()) {
        if (!m_hasReceivedSomeData) {
            m_hasReceivedSomeData = true;
            m_frame->document()->setCompatibilityMode(Document::NoQuirksMode);
        }

        // FIXME: This should call DocumentParser::appendBytes instead of append
        // to support RawDataDocumentParsers.
        if (DocumentParser* parser = m_frame->document()->parser())
            parser->append(source);
    }

    end();
}

void DocumentWriter::clear()
{
    m_decoder = 0;
    m_hasReceivedSomeData = false;
    if (!m_encodingWasChosenByUser)
        m_encoding = String();
}

void DocumentWriter::begin()
{
    begin(KURL());
}

PassRefPtr<Document> DocumentWriter::createDocument(const KURL& url)
{
    if (!m_frame->loader()->stateMachine()->isDisplayingInitialEmptyDocument() && m_frame->loader()->client()->shouldUsePluginDocument(m_mimeType))
        return PluginDocument::create(m_frame, url);
    if (!m_frame->loader()->client()->hasHTMLView())
        return PlaceholderDocument::create(m_frame, url);
    return DOMImplementation::createDocument(m_mimeType, m_frame, url, m_frame->inViewSourceMode());
}

void DocumentWriter::begin(const KURL& urlReference, bool dispatch, SecurityOriginSource originSource)
{
    RefPtr<Document> oldDocument = m_frame->document();

    // We grab a local copy of the URL because it's easy for callers to supply
    // a URL that will be deallocated during the execution of this function.
    // For example, see <https://bugs.webkit.org/show_bug.cgi?id=66360>.
    KURL url = urlReference;

    // Create a new document before clearing the frame, because it may need to
    // inherit an aliased security context.
    RefPtr<Document> document = createDocument(url);
    
    // If the new document is for a Plugin but we're supposed to be sandboxed from Plugins,
    // then replace the document with one whose parser will ignore the incoming data (bug 39323)
    if (document->isPluginDocument() && m_frame->loader()->isSandboxed(SandboxPlugins))
        document = SinkDocument::create(m_frame, url);

    // FIXME: Do we need to consult the content security policy here about blocked plug-ins?

    bool resetScripting = !(m_frame->loader()->stateMachine()->isDisplayingInitialEmptyDocument() && m_frame->document()->securityOrigin()->isSecureTransitionTo(url));
    m_frame->loader()->clear(resetScripting, resetScripting);
    clear();
    if (resetScripting)
        m_frame->script()->updatePlatformScriptObjects();

    m_frame->loader()->setOutgoingReferrer(url);
    m_frame->setDocument(document);

    if (m_decoder)
        document->setDecoder(m_decoder.get());
    if (originSource == InheritSecurityOrigin) {
        document->setCookieURL(oldDocument->cookieURL());
        document->setSecurityOrigin(oldDocument->securityOrigin());
    }

    m_frame->domWindow()->setURL(document->url());
    m_frame->domWindow()->setSecurityOrigin(document->securityOrigin());

    m_frame->loader()->didBeginDocument(dispatch);

    document->implicitOpen();

    // We grab a reference to the parser so that we'll always send data to the
    // original parser, even if the document acquires a new parser (e.g., via
    // document.open).
    m_parser = document->parser();

    if (m_frame->view() && m_frame->loader()->client()->hasHTMLView())
        m_frame->view()->setContentsSize(IntSize());
}

TextResourceDecoder* DocumentWriter::createDecoderIfNeeded()
{
    if (!m_decoder) {
        if (Settings* settings = m_frame->settings()) {
            m_decoder = TextResourceDecoder::create(m_mimeType,
                settings->defaultTextEncodingName(),
                settings->usesEncodingDetector());
            Frame* parentFrame = m_frame->tree()->parent();
            // Set the hint encoding to the parent frame encoding only if
            // the parent and the current frames share the security origin.
            // We impose this condition because somebody can make a child frame 
            // containing a carefully crafted html/javascript in one encoding
            // that can be mistaken for hintEncoding (or related encoding) by
            // an auto detector. When interpreted in the latter, it could be
            // an attack vector.
            // FIXME: This might be too cautious for non-7bit-encodings and
            // we may consider relaxing this later after testing.
            if (canReferToParentFrameEncoding(m_frame, parentFrame))
                m_decoder->setHintEncoding(parentFrame->document()->decoder());
        } else
            m_decoder = TextResourceDecoder::create(m_mimeType, String());
        Frame* parentFrame = m_frame->tree()->parent();
        if (m_encoding.isEmpty()) {
            if (canReferToParentFrameEncoding(m_frame, parentFrame))
                m_decoder->setEncoding(parentFrame->document()->inputEncoding(), TextResourceDecoder::EncodingFromParentFrame);
        } else {
            m_decoder->setEncoding(m_encoding,
                m_encodingWasChosenByUser ? TextResourceDecoder::UserChosenEncoding : TextResourceDecoder::EncodingFromHTTPHeader);
        }
        m_frame->document()->setDecoder(m_decoder.get());
    }
    return m_decoder.get();
}

void DocumentWriter::reportDataReceived()
{
    ASSERT(m_decoder);
    if (m_hasReceivedSomeData)
        return;
    m_hasReceivedSomeData = true;
    if (m_decoder->encoding().usesVisualOrdering())
        m_frame->document()->setVisuallyOrdered();
    m_frame->document()->recalcStyle(Node::Force);
}

void DocumentWriter::addData(const char* bytes, size_t length)
{
    m_parser->appendBytes(this, bytes, length);
}

void DocumentWriter::end()
{
    m_frame->loader()->didEndDocument();
    endIfNotLoadingMainResource();
}

void DocumentWriter::endIfNotLoadingMainResource()
{
    // FIXME: This isn't really the check we should be doing. We should re-work
    // how we end parsing to match the model in HTML5.
    if (m_frame->loader()->isLoadingMainResource() || !m_frame->page() || !m_frame->document())
        return;

    // http://bugs.webkit.org/show_bug.cgi?id=10854
    // The frame's last ref may be removed and it can be deleted by checkCompleted(), 
    // so we'll add a protective refcount
    RefPtr<Frame> protector(m_frame);

    if (!m_parser)
        return;
    // FIXME: m_parser->finish() should imply m_parser->flush().
    m_parser->flush(this);
    if (!m_parser)
        return;
    m_parser->finish();
    m_parser = 0;
}

void DocumentWriter::setEncoding(const String& name, bool userChosen)
{
    m_frame->loader()->willSetEncoding();
    m_encoding = name;
    m_encodingWasChosenByUser = userChosen;
}

#if PLATFORM(MAC) || PLATFORM(WIN)
String DocumentWriter::deprecatedFrameEncoding() const
{
    if (m_frame->document()->url().isEmpty())
        return m_encoding;

    if (m_encodingWasChosenByUser && !m_encoding.isEmpty())
        return m_encoding;

    if (m_decoder && m_decoder->encoding().isValid())
        return m_decoder->encoding().name();

    return String();
}
#endif

void DocumentWriter::setDocumentWasLoadedAsPartOfNavigation()
{
    ASSERT(!m_parser->isStopped());
    m_parser->setDocumentWasLoadedAsPartOfNavigation();
}

} // namespace WebCore
