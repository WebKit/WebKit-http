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
#include "WebFramePrivate.h"
#include "WebProcess.h"
#include "markup.h"

#include <Entry.h>

static const float kMinimumTextSizeMultiplier = 0.5;
static const float kMaximumTextSizeMultiplier = 3;
static const float kTextSizeMultiplierRatio = 1.1;

using namespace WebCore;


WebFrame::WebFrame(WebProcess* webProcess, WebCore::Page* parentPage, WebCore::Frame* parentFrame,
        WebCore::HTMLFrameOwnerElement* ownerElement, const WebCore::String& frameName)
    : m_textMagnifier(1.0)
    , m_isEditable(true)
    , m_beingDestroyed(false)
    , m_title(0)
    , m_impl(new WebFramePrivate())
{
	m_impl->name = frameName;
	m_impl->ownerElement = ownerElement;
	m_impl->page = parentPage;
	m_impl->loaderClient = new WebCore::FrameLoaderClientHaiku(webProcess, this);
    m_impl->frame = WebCore::Frame::create(parentPage, ownerElement, m_impl->loaderClient);

    m_impl->frame->tree()->setName(frameName);
    if (parentFrame)
        parentFrame->tree()->appendChild(m_impl->frame);

    m_impl->frame->init();
}

WebFrame::~WebFrame()
{
	delete m_impl;
}

void WebFrame::setDispatchTarget(const BMessenger& messenger)
{
    m_impl->loaderClient->setDispatchTarget(messenger);
}


WebCore::Frame* WebFrame::frame()
{
    return m_impl->frame.get();
}

void WebFrame::loadRequest(BString url)
{
    loadRequest(KURL(KURL(), url.Trim().String()));
}

void WebFrame::loadRequest(KURL url)
{
    if (m_impl->frame && m_impl->frame->loader()) {
        if (url.protocol().isEmpty()) {
            if (BEntry(BString(url.string())).Exists()) {
                url.setProtocol("file");
                url.setPath(url.path());
            } else {
                url.setProtocol("http");
                url.setPath("//" + url.path());
            }
        }

        m_impl->frame->loader()->load(url, false);
    }
}

void WebFrame::stopLoading()
{
    if (m_impl->frame && m_impl->frame->loader())
        m_impl->frame->loader()->stop();
}

void WebFrame::reload()
{
    if (m_impl->frame && m_impl->frame->loader())
        m_impl->frame->loader()->reload();
}

bool WebFrame::canGoBack()
{
    if (m_impl->frame && m_impl->frame->page() && m_impl->frame->page()->backForwardList())
        return m_impl->frame->page()->canGoBackOrForward(-1);

    return false;
}

bool WebFrame::canGoForward()
{
    if (m_impl->frame && m_impl->frame->page() && m_impl->frame->page()->backForwardList())
        return m_impl->frame->page()->canGoBackOrForward(1);

    return false;
}

bool WebFrame::goBack()
{
    if (m_impl->frame && m_impl->frame->page())
        return m_impl->frame->page()->goBack();

    return false;
}

bool WebFrame::goForward()
{
    if (m_impl->frame && m_impl->frame->page())
        return m_impl->frame->page()->goForward();

    return false;
}
bool WebFrame::canCopy()
{
    if (m_impl->frame && m_impl->frame->view())
        return m_impl->frame->editor()->canCopy() || m_impl->frame->editor()->canDHTMLCopy();

    return false;
}

bool WebFrame::canCut()
{
    if (m_impl->frame && m_impl->frame->view())
        return m_impl->frame->editor()->canCut() || m_impl->frame->editor()->canDHTMLCut();

    return false;
}

bool WebFrame::canPaste()
{
    if (m_impl->frame && m_impl->frame->view())
        return m_impl->frame->editor()->canPaste() || m_impl->frame->editor()->canDHTMLPaste();

    return false;
}

void WebFrame::copy()
{
    if (canCopy())
        m_impl->frame->editor()->copy();
}

void WebFrame::cut()
{
    if (canCut())
        m_impl->frame->editor()->cut();
}

void WebFrame::paste()
{
    if (canPaste())
        m_impl->frame->editor()->paste();
}

bool WebFrame::canUndo()
{
    if (m_impl->frame && m_impl->frame->editor())
        return m_impl->frame->editor()->canUndo();

    return false;
}

bool WebFrame::canRedo()
{
    if (m_impl->frame && m_impl->frame->editor())
        return m_impl->frame->editor()->canRedo();

    return false;
}

void WebFrame::undo()
{
    if (m_impl->frame && m_impl->frame->editor() && canUndo())
        return m_impl->frame->editor()->undo();
}

void WebFrame::redo()
{
    if (m_impl->frame && m_impl->frame->editor() && canRedo())
        return m_impl->frame->editor()->redo();
}

bool WebFrame::allowsScrolling()
{
    if (m_impl->frame && m_impl->frame->view())
        return m_impl->frame->view()->canHaveScrollbars();

    return false;
}

void WebFrame::setAllowsScrolling(bool flag)
{
    if (m_impl->frame && m_impl->frame->view())
        m_impl->frame->view()->setCanHaveScrollbars(flag);
}

const char* WebFrame::getPageSource()
{
    if (m_impl->frame) {
        if (m_impl->frame->view() && m_impl->frame->view()->layoutPending())
            m_impl->frame->view()->layout();

        WebCore::Document* document = m_impl->frame->document();

        if (document) {
            BString source = createMarkup(document);
            return source.String();
        }
    }

    return "";
}

void WebFrame::setPageSource(const char* source)
{
    if (m_impl->frame && m_impl->frame->loader()) {
        WebCore::FrameLoader* loader = m_impl->frame->loader();
        loader->begin();
        loader->write(String(source));
        loader->end();
    }
}

void WebFrame::setTransparent(bool transparent)
{
    if (m_impl->frame && m_impl->frame->view())
        m_impl->frame->view()->setTransparent(transparent);
}

bool WebFrame::isTransparent() const
{
    if (m_impl->frame && m_impl->frame->view())
        return m_impl->frame->view()->isTransparent();

    return false;
}

BString WebFrame::getInnerText()
{
    FrameView* view = m_impl->frame->view();

    if (view && view->layoutPending())
        view->layout();

    WebCore::Element *documentElement = m_impl->frame->document()->documentElement();
    return documentElement->innerText();
}

BString WebFrame::getAsMarkup()
{
    if (!m_impl->frame->document())
        return String();

    return createMarkup(m_impl->frame->document());
}

BString WebFrame::getExternalRepresentation()
{
    FrameView* view = m_impl->frame->view();

    if (view && view->layoutPending())
        view->layout();

    return externalRepresentation(m_impl->frame.get());
}

bool WebFrame::findString(const char* string, bool forward, bool caseSensitive, bool wrapSelection, bool startInSelection)
{
    if (m_impl->frame)
        return m_impl->frame->findString(string, forward, caseSensitive, wrapSelection, startInSelection);

    return false;
}

bool WebFrame::canIncreaseTextSize() const
{
    if (m_impl->frame)
        if (m_textMagnifier * kTextSizeMultiplierRatio <= kMaximumTextSizeMultiplier)
            return true;

    return false;
}

bool WebFrame::canDecreaseTextSize() const
{
    if (m_impl->frame)
        return m_textMagnifier / kTextSizeMultiplierRatio >= kMinimumTextSizeMultiplier;

    return false;
}

void WebFrame::increaseTextSize()
{
    if (canIncreaseTextSize()) {
        m_textMagnifier = m_textMagnifier * kTextSizeMultiplierRatio;
        m_impl->frame->setZoomFactor(m_textMagnifier, true);
    }
}

void WebFrame::decreaseTextSize()
{
    if (canDecreaseTextSize()) {
        m_textMagnifier = m_textMagnifier / kTextSizeMultiplierRatio;
        m_impl->frame->setZoomFactor(m_textMagnifier, true);
    }
}

void WebFrame::resetTextSize()
{
    m_textMagnifier = 1.0;

    if (m_impl->frame)
        m_impl->frame->setZoomFactor(m_textMagnifier, true);
}
