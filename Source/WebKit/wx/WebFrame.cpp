/*
 * Copyright (C) 2007 Kevin Ollivier  All rights reserved.
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

#include "Document.h"
#include "Editor.h"
#include "Element.h"
#include "EventHandler.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClientWx.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "HostWindow.h"
#include "HTMLFrameOwnerElement.h"
#include "markup.h"
#include "Page.h"
#include "PlatformString.h"
#include "PrintContext.h"
#include "RenderTreeAsText.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "ScriptController.h"
#include "ScriptValue.h"
#include "SubstituteData.h"
#include "TextEncoding.h"

#include "JSDOMBinding.h"
#include <runtime/JSValue.h>
#include <runtime/UString.h>
#include <wtf/text/CString.h>

#include "EditorClientWx.h"
#include "FrameLoaderClientWx.h"

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "WebDOMNode.h"

#include "WebDOMSelection.h"
#include "WebFrame.h"
#include "WebView.h"
#include "WebFramePrivate.h"
#include "WebViewPrivate.h"

#include <algorithm>

#include <wx/defs.h>
#include <wx/dc.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <wx/graphics.h>
#include <wx/print.h>
#include <wx/printdlg.h>

// Match Safari's min/max zoom sizes by default
#define MinimumTextSizeMultiplier       0.5f
#define MaximumTextSizeMultiplier       3.0f
#define TextSizeMultiplierRatio         1.2f

using namespace std;

// we need wxGraphicsContext and wxPrinterDC to work together, 
// which requires wx 2.9.x.
#if wxCHECK_VERSION(2, 9, 1)
class wxWebFramePrintout : public wxPrintout {
public:
    wxWebFramePrintout(WebCore::Frame* frame) :
        m_frame(frame),
        m_printContext(frame),
        m_pageWidth(0.0),
        m_fromPage(1),
        m_toPage(1)
    {
    }

    int GetPageCount() { return m_printContext.pageCount(); }
    void SetFirstPage(int page) { m_fromPage = page; }
    void SetLastPage(int page) { m_toPage = page; }

    void InitializeWithPageSize(wxRect pageRect, bool isMM = true)
    {
        if (isMM) {
            double mmToPixelsX = (double)wxGetDisplaySize().GetWidth() /
                                    (double)wxGetDisplaySizeMM().GetWidth();
            double mmToPixelsY = (double)wxGetDisplaySize().GetHeight() /
                                    (double)wxGetDisplaySizeMM().GetHeight();
            // convert mm to pixels
            pageRect.x = pageRect.x * mmToPixelsX;
            pageRect.y = pageRect.y * mmToPixelsY;
            pageRect.width = pageRect.width * mmToPixelsX;
            pageRect.height = pageRect.height * mmToPixelsY;
        }
        m_pageWidth = pageRect.width;
        m_printContext.begin(m_pageWidth);

        float pageHeight = pageRect.height;
        m_printContext.computePageRects(WebCore::FloatRect(pageRect), /* headerHeight */ 0, /* footerHeight */ 0, /* userScaleFactor */ 1.0, pageHeight);
    }
    
    void OnBeginPrinting()
    {
        wxPrinterDC* pdc = dynamic_cast<wxPrinterDC*>(GetDC());
        pdc->SetMapMode(wxMM_POINTS);
        int pixelsW = 0;
        int pixelsH = 0;
        pdc->GetSize(&pixelsW, &pixelsH);
        pixelsW = pdc->DeviceToLogicalXRel(pixelsW);
        pixelsH = pdc->DeviceToLogicalYRel(pixelsH);
        InitializeWithPageSize(wxRect(0, 0, pixelsW, pixelsH - 40), false);
    }
    
    void GetPageInfo(int *minPage, int *maxPage, int *pageFrom, int *pageTo)
    {
        if (minPage)
            *minPage = 1;
        if (maxPage)
            *maxPage = m_printContext.pageCount();
        if (pageFrom)
            *pageFrom = m_fromPage;
        if (pageTo)
            *pageTo = m_toPage;
    }
    
    bool HasPage(int pageNum)
    {
        return pageNum <= m_printContext.pageCount() && pageNum >= m_fromPage && pageNum <= m_toPage;
    }
    
    bool OnPrintPage(int pageNum)
    {
        wxPrinterDC* pdc = dynamic_cast<wxPrinterDC*>(GetDC());
#if wxCHECK_VERSION(2, 9, 2) && defined(wxUSE_CAIRO) && wxUSE_CAIRO
        wxGraphicsRenderer* renderer = wxGraphicsRenderer::GetCairoRenderer();
        if (!renderer)
            renderer = wxGraphicsRenderer::GetDefaultRenderer();
        wxGraphicsContext* context = renderer->CreateContext(*pdc);
        wxGCDC gcdc(context);
#else
        wxGCDC gcdc(pdc);
#endif
        if (!gcdc.IsOk())
            return false;

        WebCore::GraphicsContext ctx(&gcdc);
        m_printContext.spoolPage(ctx, pageNum - 1, m_pageWidth);
        
        return true;
    }
    
    void OnEndPrinting()
    {
        m_printContext.end();   
    }
    
private:
    float m_pageWidth;
    int m_fromPage;
    int m_toPage;
    WebCore::Frame *m_frame;
    WebCore::PrintContext m_printContext;
};
#endif

wxWebFrame* kit(WebCore::Frame* frame)
{
    if (!frame)
        return 0;
    if (!frame->loader())
        return 0;
    
    WebCore::FrameLoaderClientWx* loaderClient = dynamic_cast<WebCore::FrameLoaderClientWx*>(frame->loader()->client());
    if (loaderClient)
        return loaderClient->webFrame();
    
    return 0;
}

wxWebFrame::wxWebFrame(wxWebView* container, wxWebFrame* parent, WebViewFrameData* data) :
    m_textMagnifier(1.0),
    m_isInitialized(false),
    m_beingDestroyed(false)
{

    m_impl = new WebFramePrivate();
 
    WebCore::HTMLFrameOwnerElement* parentFrame = 0;
    
    if (data) {
        parentFrame = data->ownerElement;
    }
    
    WebCore::FrameLoaderClientWx* loaderClient = new WebCore::FrameLoaderClientWx();
    RefPtr<WebCore::Frame> newFrame = WebCore::Frame::create(container->m_impl->page, parentFrame, loaderClient);

    m_impl->frame = newFrame.get();

    if (data)
        newFrame->tree()->setName(data->name);

    // Subframes expect to be added to the FrameTree before init is called.
    if (parentFrame)
        parentFrame->document()->frame()->tree()->appendChild(newFrame.get());
    
    loaderClient->setFrame(this);
    loaderClient->setWebView(container);
    
    if (data && data->ownerElement)
        m_impl->frame->ref();

    m_impl->frame->init();
        
    m_isInitialized = true;
}

wxWebFrame::~wxWebFrame()
{
    if (m_impl)
        delete m_impl;
}

wxString wxWebFrame::GetName()
{
    if (m_impl && m_impl->frame && m_impl->frame->tree())
        return m_impl->frame->tree()->name().string();
    return wxEmptyString;
}

WebCore::Frame* wxWebFrame::GetFrame()
{
    if (m_impl)
        return m_impl->frame;
        
    return 0;
}

void wxWebFrame::Stop()
{
    if (m_impl->frame && m_impl->frame->loader())
        m_impl->frame->loader()->stop();
}

void wxWebFrame::Reload()
{
    if (m_impl->frame && m_impl->frame->loader())
        m_impl->frame->loader()->reload();
}

wxString wxWebFrame::GetPageSource()
{
    if (m_impl->frame) {
        if (m_impl->frame->view() && m_impl->frame->view()->layoutPending())
            m_impl->frame->view()->layout();
    
        WebCore::Document* doc = m_impl->frame->document();
        
        if (doc) {
            wxString source = createMarkup(doc);
            return source;
        }
    }
    return wxEmptyString;
}

void wxWebFrame::SetPageSource(const wxString& source, const wxString& baseUrl, const wxString& mimetype)
{
    if (m_impl->frame && m_impl->frame->loader()) {
        WebCore::KURL url(WebCore::KURL(), baseUrl);

        const wxCharBuffer charBuffer(source.utf8_str());
        const char* contents = charBuffer;

        WTF::PassRefPtr<WebCore::SharedBuffer> sharedBuffer = WebCore::SharedBuffer::create(contents, strlen(contents));
        WebCore::SubstituteData substituteData(sharedBuffer, mimetype, WTF::String("UTF-8"), WebCore::blankURL(), url);

        m_impl->frame->loader()->stop();
        m_impl->frame->loader()->load(WebCore::ResourceRequest(url), substituteData, false);
    }
}

wxString wxWebFrame::GetInnerText()
{
    if (m_impl->frame->view() && m_impl->frame->view()->layoutPending())
        m_impl->frame->view()->layout();
        
    WebCore::Element *documentElement = m_impl->frame->document()->documentElement();
    return documentElement->innerText();
}

wxString wxWebFrame::GetAsMarkup()
{
    if (!m_impl->frame || !m_impl->frame->document())
        return wxEmptyString;

    return createMarkup(m_impl->frame->document());
}

wxString wxWebFrame::GetExternalRepresentation()
{
    if (m_impl->frame->view() && m_impl->frame->view()->layoutPending())
        m_impl->frame->view()->layout();

    return externalRepresentation(m_impl->frame);
}

wxString wxWebFrame::GetSelectionAsHTML()
{
    if (m_impl->frame)
        return m_impl->frame->selection()->toNormalizedRange()->toHTML();
        
    return wxEmptyString;
}

wxString wxWebFrame::GetSelectionAsText()
{
    if (m_impl->frame)
        return m_impl->frame->selection()->toNormalizedRange()->text();
        
    return wxEmptyString;
}

wxWebKitSelection wxWebFrame::GetSelection()
{
    if (m_impl->frame)
        return wxWebKitSelection(m_impl->frame->selection());
        
    return 0;
}

wxString wxWebFrame::RunScript(const wxString& javascript)
{
    wxString returnValue = wxEmptyString;
    if (m_impl->frame && m_impl->frame->loader()) {
        bool hasLoaded = m_impl->frame->loader()->frameHasLoaded();
        wxASSERT_MSG(hasLoaded, wxT("Document must be loaded before calling RunScript."));
        if (hasLoaded) {
            WebCore::ScriptController* controller = m_impl->frame->script();
            bool jsEnabled = controller->canExecuteScripts(WebCore::AboutToExecuteScript); 
            wxASSERT_MSG(jsEnabled, wxT("RunScript requires JavaScript to be enabled."));
            if (jsEnabled) {
                JSC::JSValue result = controller->executeScript(javascript, true).jsValue();
                if (result)
                    returnValue = wxString(result.toString(m_impl->frame->script()->globalObject(WebCore::mainThreadNormalWorld())->globalExec()).utf8().data(), wxConvUTF8);        
            }
        }
    }
    return returnValue;
}

bool wxWebFrame::ExecuteEditCommand(const wxString& command, const wxString& parameter)
{
    if (m_impl->frame && IsEditable())
        return m_impl->frame->editor()->command(command).execute(parameter);
}

EditState wxWebFrame::GetEditCommandState(const wxString& command) const
{
    if (m_impl->frame && IsEditable()) { 
        WebCore::TriState state = m_impl->frame->editor()->command(command).state();
        if (state == WebCore::TrueTriState)
            return EditStateTrue;
        if (state == WebCore::FalseTriState)
            return EditStateFalse;

        return EditStateMixed;
    }
        
    return EditStateFalse;
}

wxString wxWebFrame::GetEditCommandValue(const wxString& command) const
{
    if (m_impl->frame && IsEditable())
        return m_impl->frame->editor()->command(command).value();
        
    return wxEmptyString;
}


bool wxWebFrame::FindString(const wxString& string, bool forward, bool caseSensitive, bool wrapSelection, bool startInSelection)
{
    if (m_impl->frame)
        return m_impl->frame->editor()->findString(string, forward, caseSensitive, wrapSelection, startInSelection);

    return false;
}

void wxWebFrame::LoadURL(const wxString& url)
{
    if (m_impl->frame && m_impl->frame->loader()) {
        WebCore::KURL kurl = WebCore::KURL(WebCore::KURL(), url, WebCore::UTF8Encoding());
        // NB: This is an ugly fix, but CURL won't load sub-resources if the
        // protocol is omitted; sadly, it will not emit an error, either, so
        // there's no way for us to catch this problem the correct way yet.
        if (kurl.protocol().isEmpty()) {
            // is it a file on disk?
            if (wxFileExists(url)) {
                kurl.setProtocol("file");
                kurl.setPath("//" + kurl.path());
            }
            else {
                kurl.setProtocol("http");
                kurl.setPath("//" + kurl.path());
            }
        }
        m_impl->frame->loader()->load(kurl, false);
    }
}

wxString wxWebFrame::GetURL() const
{
    if (m_impl->frame && m_impl->frame->document())
        return m_impl->frame->document()->url().string();
    
    return wxEmptyString;
}


bool wxWebFrame::GoBack()
{
    if (m_impl->frame && m_impl->frame->page())
        return m_impl->frame->page()->goBack();

    return false;
}

bool wxWebFrame::GoForward()
{
    if (m_impl->frame && m_impl->frame->page())
        return m_impl->frame->page()->goForward();

    return false;
}

bool wxWebFrame::CanGoBack()
{
    if (m_impl->frame && m_impl->frame->page())
        return m_impl->frame->page()->canGoBackOrForward(-1);

    return false;
}

bool wxWebFrame::CanGoForward()
{
    if (m_impl->frame && m_impl->frame->page())
        return m_impl->frame->page()->canGoBackOrForward(1);

    return false;
}

void wxWebFrame::Undo()
{
    if (m_impl->frame && m_impl->frame->editor() && CanUndo())
        return m_impl->frame->editor()->undo();
}

void wxWebFrame::Redo()
{
    if (m_impl->frame && m_impl->frame->editor() && CanRedo())
        return m_impl->frame->editor()->redo();
}

bool wxWebFrame::CanUndo()
{
    if (m_impl->frame && m_impl->frame->editor())
        return m_impl->frame->editor()->canUndo();

    return false;
}

bool wxWebFrame::CanRedo()
{
    if (m_impl->frame && m_impl->frame->editor())
        return m_impl->frame->editor()->canRedo();

    return false;
}

bool wxWebFrame::CanIncreaseTextSize() const
{
    if (m_impl->frame && m_impl->frame->view()) {
        if (m_textMagnifier*TextSizeMultiplierRatio <= MaximumTextSizeMultiplier)
            return true;
    }
    return false;
}

void wxWebFrame::IncreaseTextSize()
{
    if (CanIncreaseTextSize()) {
        m_textMagnifier = m_textMagnifier*TextSizeMultiplierRatio;
        m_impl->frame->setTextZoomFactor(m_textMagnifier);
    }
}

bool wxWebFrame::CanDecreaseTextSize() const
{
    if (m_impl->frame && m_impl->frame->view()) {
        if (m_textMagnifier/TextSizeMultiplierRatio >= MinimumTextSizeMultiplier)
            return true;
    }
    return false;
}

void wxWebFrame::DecreaseTextSize()
{        
    if (CanDecreaseTextSize()) {
        m_textMagnifier = m_textMagnifier/TextSizeMultiplierRatio;
        m_impl->frame->setTextZoomFactor(m_textMagnifier);
    }
}

void wxWebFrame::ResetTextSize()
{
    m_textMagnifier = 1.0;
    if (m_impl->frame)
        m_impl->frame->setTextZoomFactor(m_textMagnifier);
}

void wxWebFrame::MakeEditable(bool enable)
{
    if (enable != IsEditable() && m_impl->frame && m_impl->frame->page())
        m_impl->frame->page()->setEditable(enable);
}

bool wxWebFrame::IsEditable() const
{
    if (m_impl->frame && m_impl->frame->page())
        return m_impl->frame->page()->isEditable();
    return false;
}

bool wxWebFrame::CanCopy()
{
    if (m_impl->frame && m_impl->frame->view())
        return (m_impl->frame->editor()->canCopy() || m_impl->frame->editor()->canDHTMLCopy());

    return false;
}

void wxWebFrame::Copy()
{
    if (CanCopy())
        m_impl->frame->editor()->copy();
}

bool wxWebFrame::CanCut()
{
    if (m_impl->frame && m_impl->frame->view())
        return (m_impl->frame->editor()->canCut() || m_impl->frame->editor()->canDHTMLCut());

    return false;
}

void wxWebFrame::Cut()
{
    if (CanCut())
        m_impl->frame->editor()->cut();
}

bool wxWebFrame::CanPaste()
{
    if (m_impl->frame && m_impl->frame->view())
        return (m_impl->frame->editor()->canPaste() || m_impl->frame->editor()->canDHTMLPaste());

    return false;
}

void wxWebFrame::Paste()
{
    if (CanPaste())
        m_impl->frame->editor()->paste();

}

void wxWebFrame::Print(bool showDialog)
{
#if wxCHECK_VERSION(2, 9, 1)
    if (!m_impl->frame)
        return;
    
    wxPrintDialogData printdata;
    printdata.GetPrintData().SetPrintMode(wxPRINT_MODE_PRINTER);
    printdata.GetPrintData().SetNoCopies(1);
#if wxCHECK_VERSION(2, 9, 2)
    printdata.GetPrintData().ConvertFromNative();
#else
    // due to wx bugs, we cannot get the actual native default paper type before showing the dialog until 2.9.2, 
    // so pick a common default instead.
    printdata.GetPrintData().SetPaperId(wxPAPER_LETTER);
#endif

    wxPageSetupDialogData pageSetup(printdata.GetPrintData());

    wxRect paperSize = pageSetup.GetPaperSize();
#ifdef __WXMSW__
    // On Windows, the paper size apparently includes the non-printable areas of the page.
    // Guesstimate the printable page margins until we find a better solution.
    paperSize.Deflate(15, 15);
#endif
    wxWebFramePrintout* printout = new wxWebFramePrintout(m_impl->frame);
    printout->InitializeWithPageSize(paperSize);
    
    printdata.SetMinPage(1);
    printdata.SetMaxPage(printout->GetPageCount());
    printdata.SetFromPage(1);
    printdata.SetToPage(printout->GetPageCount());

    if (showDialog) {
        wxPrintDialog dialog(0, &printdata);
        if (dialog.ShowModal() == wxID_OK) {
            printdata = dialog.GetPrintDialogData();            
            printout->SetFirstPage(printdata.GetFromPage());
            printout->SetLastPage(printdata.GetToPage());
        } else
            return;
    }
    
    wxPrinter printer(&printdata);
        
    printer.Print(0, printout, false);
        
#else
    wxFAIL_MSG(wxT("Printing is only supported in wxWidgets 2.9.1 and above."));
#endif
}

wxWebViewDOMElementInfo wxWebFrame::HitTest(const wxPoint& pos) const
{
    wxWebViewDOMElementInfo domInfo;

    if (m_impl->frame->view()) {
        WebCore::HitTestResult result = m_impl->frame->eventHandler()->hitTestResultAtPoint(m_impl->frame->view()->windowToContents(pos), false);
        if (result.innerNode()) {
            domInfo.SetLink(result.absoluteLinkURL().string());
            domInfo.SetText(result.textContent());
            domInfo.SetImageSrc(result.absoluteImageURL().string());
            domInfo.SetSelected(result.isSelected());
        }
    }

    return domInfo;
}

bool wxWebFrame::ShouldClose() const
{
    if (m_impl->frame)
        return m_impl->frame->loader()->shouldClose();

    return true;
}

wxWebKitCompatibilityMode wxWebFrame::GetCompatibilityMode() const
{
    if (m_impl->frame && m_impl->frame->document())
        return (wxWebKitCompatibilityMode)m_impl->frame->document()->compatibilityMode();

    return QuirksMode;
}

void wxWebFrame::GrantUniversalAccess()
{
    if (m_impl->frame && m_impl->frame->document())
        m_impl->frame->document()->securityOrigin()->grantUniversalAccess();
}
