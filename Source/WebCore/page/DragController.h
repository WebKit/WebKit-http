/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
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

#ifndef DragController_h
#define DragController_h

#include "DragActions.h"
#include "DragImage.h"
#include "IntPoint.h"
#include "URL.h"

namespace WebCore {

    class DataTransfer;
    class Document;
    class DragClient;
    class DragData;
    class Element;
    class Frame;
    class FrameSelection;
    class HTMLInputElement;
    class IntRect;
    class Page;
    class PlatformMouseEvent;

    struct DragState;

    class DragController {
        WTF_MAKE_NONCOPYABLE(DragController); WTF_MAKE_FAST_ALLOCATED;
    public:
        DragController(Page&, DragClient&);
        ~DragController();

        static PassOwnPtr<DragController> create(Page*, DragClient*);

        DragClient& client() const { return m_client; }

        DragOperation dragEntered(DragData&);
        void dragExited(DragData&);
        DragOperation dragUpdated(DragData&);
        bool performDragOperation(DragData&);

        bool mouseIsOverFileInput() const { return m_fileInputElementUnderMouse; }
        unsigned numberOfItemsToBeAccepted() const { return m_numberOfItemsToBeAccepted; }

        // FIXME: It should be possible to remove a number of these accessors once all
        // drag logic is in WebCore.
        void setDidInitiateDrag(bool initiated) { m_didInitiateDrag = initiated; } 
        bool didInitiateDrag() const { return m_didInitiateDrag; }
        DragOperation sourceDragOperation() const { return m_sourceDragOperation; }
        const URL& draggingImageURL() const { return m_draggingImageURL; }
        void setDragOffset(const IntPoint& offset) { m_dragOffset = offset; }
        const IntPoint& dragOffset() const { return m_dragOffset; }
        DragSourceAction dragSourceAction() const { return m_dragSourceAction; }

        Document* documentUnderMouse() const { return m_documentUnderMouse.get(); }
        DragDestinationAction dragDestinationAction() const { return m_dragDestinationAction; }
        DragSourceAction delegateDragSourceAction(const IntPoint& rootViewPoint);
        
        Element* draggableElement(const Frame*, Element* start, const IntPoint&, DragState&) const;
        void dragEnded();
        
        void placeDragCaret(const IntPoint&);
        
        bool startDrag(Frame& src, const DragState&, DragOperation srcOp, const PlatformMouseEvent& dragEvent, const IntPoint& dragOrigin);
        static const IntSize& maxDragImageSize();
        
        static const int LinkDragBorderInset;
        static const int MaxOriginalImageArea;
        static const int DragIconRightInset;
        static const int DragIconBottomInset;        
        static const float DragImageAlpha;

    private:
        bool dispatchTextInputEventFor(Frame*, DragData&);
        bool canProcessDrag(DragData&);
        bool concludeEditDrag(DragData&);
        DragOperation dragEnteredOrUpdated(DragData&);
        DragOperation operationForLoad(DragData&);
        bool tryDocumentDrag(DragData&, DragDestinationAction, DragOperation&);
        bool tryDHTMLDrag(DragData&, DragOperation&);
        DragOperation dragOperation(DragData&);
        void cancelDrag();
        bool dragIsMove(FrameSelection&, DragData&);
        bool isCopyKeyDown(DragData&);

        void mouseMovedIntoDocument(Document*);

        void doImageDrag(Element&, const IntPoint&, const IntRect&, DataTransfer&, Frame&, IntPoint&);
        void doSystemDrag(DragImageRef, const IntPoint&, const IntPoint&, DataTransfer&, Frame&, bool forLink);
        void cleanupAfterSystemDrag();
        void declareAndWriteDragImage(DataTransfer&, Element&, const URL&, const String& label);

        Page& m_page;
        DragClient& m_client;

        RefPtr<Document> m_documentUnderMouse; // The document the mouse was last dragged over.
        RefPtr<Document> m_dragInitiator; // The Document (if any) that initiated the drag.
        RefPtr<HTMLInputElement> m_fileInputElementUnderMouse;
        unsigned m_numberOfItemsToBeAccepted;
        bool m_documentIsHandlingDrag;

        DragDestinationAction m_dragDestinationAction;
        DragSourceAction m_dragSourceAction;
        bool m_didInitiateDrag;
        DragOperation m_sourceDragOperation; // Set in startDrag when a drag starts from a mouse down within WebKit
        IntPoint m_dragOffset;
        URL m_draggingImageURL;
    };

}

#endif
