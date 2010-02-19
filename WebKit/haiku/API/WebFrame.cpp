/*
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
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


#include "config.h"
#include "WebFrame.h"

#include "Document.h"
#include "EditorClientHaiku.h"
#include "Element.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClientHaiku.h"
#include "FrameView.h"
#include "HTMLFrameOwnerElement.h"
#include "KURL.h"
#include "Page.h"
#include "RenderObject.h"
#include "RenderTreeAsText.h"
#include "RenderView.h"
#include "WebProcess.h"
#include "markup.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"
#include <Entry.h>
#include <String.h>

static const float kMinimumTextSizeMultiplier = 0.5;
static const float kMaximumTextSizeMultiplier = 3;
static const float kTextSizeMultiplierRatio = 1.1;

using namespace WebCore;


class WebFrame::PrivateData {
public:
    PrivateData()
        : ownerElement(0)
        , page(0)
        , frame(0)
        , loaderClient(0)
    {}

    WebCore::String name;
    WebCore::HTMLFrameOwnerElement* ownerElement;
    WebCore::Page* page;
    WTF::RefPtr<WebCore::Frame> frame;
    WebCore::FrameLoaderClientHaiku* loaderClient;
};

// #pragma mark -

WebFrame::WebFrame(WebProcess* webProcess, WebCore::Page* parentPage, WebCore::Frame* parentFrame,
        WebCore::HTMLFrameOwnerElement* ownerElement, const WebCore::String& frameName)
    : m_textMagnifier(1.0)
    , m_isEditable(true)
    , m_beingDestroyed(false)
    , m_title(0)
    , m_data(new PrivateData())
{
	m_data->name = frameName;
	m_data->ownerElement = ownerElement;
	m_data->page = parentPage;
	m_data->loaderClient = new WebCore::FrameLoaderClientHaiku(webProcess, this);
    m_data->frame = WebCore::Frame::create(parentPage, ownerElement, m_data->loaderClient);

    m_data->frame->tree()->setName(frameName);
    if (parentFrame)
        parentFrame->tree()->appendChild(m_data->frame);

    m_data->frame->init();
}

WebFrame::~WebFrame()
{
	delete m_data;
}

void WebFrame::setDispatchTarget(const BMessenger& messenger)
{
    m_data->loaderClient->setDispatchTarget(messenger);
}


WebCore::Frame* WebFrame::frame()
{
    return m_data->frame.get();
}

void WebFrame::loadRequest(BString url)
{
    loadRequest(KURL(KURL(), url.Trim().String()));
}

void WebFrame::loadRequest(KURL url)
{
	if (url.isEmpty())
		return;

    if (m_data->frame && m_data->frame->loader()) {
        if (url.protocol().isEmpty()) {
            if (BEntry(BString(url.string())).Exists()) {
                url.setProtocol("file");
                url.setPath(url.path());
            } else {
                url.setProtocol("http");
                url.setPath("//" + url.path());
            }
        }

        m_data->frame->loader()->load(url, false);
    }
}

BString WebFrame::url() const
{
    return m_data->frame->loader()->url().string();
}

void WebFrame::stopLoading()
{
    if (m_data->frame && m_data->frame->loader())
        m_data->frame->loader()->stop();
}

void WebFrame::reload()
{
    if (m_data->frame && m_data->frame->loader())
        m_data->frame->loader()->reload();
}

bool WebFrame::canGoBack()
{
    if (m_data->frame && m_data->frame->page() && m_data->frame->page()->backForwardList())
        return m_data->frame->page()->canGoBackOrForward(-1);

    return false;
}

bool WebFrame::canGoForward()
{
    if (m_data->frame && m_data->frame->page() && m_data->frame->page()->backForwardList())
        return m_data->frame->page()->canGoBackOrForward(1);

    return false;
}

bool WebFrame::goBack()
{
    if (m_data->frame && m_data->frame->page())
        return m_data->frame->page()->goBack();

    return false;
}

bool WebFrame::goForward()
{
    if (m_data->frame && m_data->frame->page())
        return m_data->frame->page()->goForward();

    return false;
}
bool WebFrame::canCopy()
{
    if (m_data->frame && m_data->frame->view())
        return m_data->frame->editor()->canCopy() || m_data->frame->editor()->canDHTMLCopy();

    return false;
}

bool WebFrame::canCut()
{
    if (m_data->frame && m_data->frame->view())
        return m_data->frame->editor()->canCut() || m_data->frame->editor()->canDHTMLCut();

    return false;
}

bool WebFrame::canPaste()
{
    if (m_data->frame && m_data->frame->view())
        return m_data->frame->editor()->canPaste() || m_data->frame->editor()->canDHTMLPaste();

    return false;
}

void WebFrame::copy()
{
    if (canCopy())
        m_data->frame->editor()->copy();
}

void WebFrame::cut()
{
    if (canCut())
        m_data->frame->editor()->cut();
}

void WebFrame::paste()
{
    if (canPaste())
        m_data->frame->editor()->paste();
}

bool WebFrame::canUndo()
{
    if (m_data->frame && m_data->frame->editor())
        return m_data->frame->editor()->canUndo();

    return false;
}

bool WebFrame::canRedo()
{
    if (m_data->frame && m_data->frame->editor())
        return m_data->frame->editor()->canRedo();

    return false;
}

void WebFrame::undo()
{
    if (m_data->frame && m_data->frame->editor() && canUndo())
        return m_data->frame->editor()->undo();
}

void WebFrame::redo()
{
    if (m_data->frame && m_data->frame->editor() && canRedo())
        return m_data->frame->editor()->redo();
}

bool WebFrame::allowsScrolling()
{
    if (m_data->frame && m_data->frame->view())
        return m_data->frame->view()->canHaveScrollbars();

    return false;
}

void WebFrame::setAllowsScrolling(bool flag)
{
    if (m_data->frame && m_data->frame->view())
        m_data->frame->view()->setCanHaveScrollbars(flag);
}

const char* WebFrame::getPageSource()
{
    if (m_data->frame) {
        if (m_data->frame->view() && m_data->frame->view()->layoutPending())
            m_data->frame->view()->layout();

        WebCore::Document* document = m_data->frame->document();

        if (document) {
            BString source = createMarkup(document);
            return source.String();
        }
    }

    return "";
}

void WebFrame::setPageSource(const char* source)
{
    if (m_data->frame && m_data->frame->loader()) {
        WebCore::FrameLoader* loader = m_data->frame->loader();
        loader->begin();
        loader->write(String(source));
        loader->end();
    }
}

void WebFrame::setTransparent(bool transparent)
{
    if (m_data->frame && m_data->frame->view())
        m_data->frame->view()->setTransparent(transparent);
}

bool WebFrame::isTransparent() const
{
    if (m_data->frame && m_data->frame->view())
        return m_data->frame->view()->isTransparent();

    return false;
}

BString WebFrame::getInnerText()
{
    FrameView* view = m_data->frame->view();

    if (view && view->layoutPending())
        view->layout();

    WebCore::Element *documentElement = m_data->frame->document()->documentElement();
    return documentElement->innerText();
}

BString WebFrame::getAsMarkup()
{
    if (!m_data->frame->document())
        return String();

    return createMarkup(m_data->frame->document());
}

BString WebFrame::getExternalRepresentation()
{
    FrameView* view = m_data->frame->view();

    if (view && view->layoutPending())
        view->layout();

    return externalRepresentation(m_data->frame.get());
}

bool WebFrame::findString(const char* string, bool forward, bool caseSensitive, bool wrapSelection, bool startInSelection)
{
    if (m_data->frame)
        return m_data->frame->findString(string, forward, caseSensitive, wrapSelection, startInSelection);

    return false;
}

bool WebFrame::canIncreaseTextSize() const
{
    if (m_data->frame)
        if (m_textMagnifier * kTextSizeMultiplierRatio <= kMaximumTextSizeMultiplier)
            return true;

    return false;
}

bool WebFrame::canDecreaseTextSize() const
{
    if (m_data->frame)
        return m_textMagnifier / kTextSizeMultiplierRatio >= kMinimumTextSizeMultiplier;

    return false;
}

void WebFrame::increaseTextSize()
{
    if (canIncreaseTextSize()) {
        m_textMagnifier = m_textMagnifier * kTextSizeMultiplierRatio;
        m_data->frame->setZoomFactor(m_textMagnifier, true);
    }
}

void WebFrame::decreaseTextSize()
{
    if (canDecreaseTextSize()) {
        m_textMagnifier = m_textMagnifier / kTextSizeMultiplierRatio;
        m_data->frame->setZoomFactor(m_textMagnifier, true);
    }
}

void WebFrame::resetTextSize()
{
    m_textMagnifier = 1.0;

    if (m_data->frame)
        m_data->frame->setZoomFactor(m_textMagnifier, true);
}
