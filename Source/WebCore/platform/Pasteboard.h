/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef Pasteboard_h
#define Pasteboard_h

#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/WTFString.h>
#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
#endif
#if PLATFORM(GTK)
#include <PasteboardHelper.h>
#endif
#include <wtf/Vector.h>

// FIXME: This class is too high-level to be in the platform directory, since it
// uses the DOM and makes calls to Editor. It should either be divested of its
// knowledge of the frame and editor or moved into the editing directory.

#if PLATFORM(MAC)
OBJC_CLASS NSAttributedString;
OBJC_CLASS NSFileWrapper;
OBJC_CLASS NSArray;
#endif

#if PLATFORM(WIN)
#include <windows.h>
typedef struct HWND__* HWND;
#endif

namespace WebCore {

#if PLATFORM(MAC)
extern const char* WebArchivePboardType;
extern const char* WebSmartPastePboardType;
extern const char* WebURLNamePboardType;
extern const char* WebURLPboardType;
extern const char* WebURLsWithTitlesPboardType;
#endif

    class ArchiveResource;
class Clipboard;
class DocumentFragment;
class Frame;
class HitTestResult;
class KURL;
class Node;
class Range;
class SharedBuffer;
    
class Pasteboard {
    WTF_MAKE_NONCOPYABLE(Pasteboard); WTF_MAKE_FAST_ALLOCATED;
public:
    enum SmartReplaceOption {
        CanSmartReplace,
        CannotSmartReplace
    };

#if PLATFORM(MAC)
    // This is required to support OS X services.
    void writeSelectionForTypes(const Vector<String>& pasteboardTypes, bool canSmartCopyOrDelete, Frame*);
    explicit Pasteboard(const String& pasteboardName);
    static String getStringSelection(Frame*);
    static PassRefPtr<SharedBuffer> getDataSelection(Frame*, const String& pasteboardType);
#endif
    
    static Pasteboard* generalPasteboard();
    void writeSelection(Range*, bool canSmartCopyOrDelete, Frame*);
    void writePlainText(const String&, SmartReplaceOption);
    void writeURL(const KURL&, const String&, Frame* = 0);
    void writeImage(Node*, const KURL&, const String& title);
    void writeClipboard(Clipboard*);
    void clear();
    bool canSmartReplace();
    PassRefPtr<DocumentFragment> documentFragment(Frame*, PassRefPtr<Range>, bool allowPlainText, bool& chosePlainText);
    String plainText(Frame* = 0);
    
#if PLATFORM(QT) || PLATFORM(CHROMIUM) || PLATFORM(GTK)
    bool isSelectionMode() const;
    void setSelectionMode(bool);
#else
    bool isSelectionMode() const { return false; }
    void setSelectionMode(bool) { }
#endif

#if PLATFORM(GTK)
    ~Pasteboard();
#endif

private:
    Pasteboard();

#if PLATFORM(MAC)
    String m_pasteboardName;
#endif

#if PLATFORM(WIN)
    HWND m_owner;
#endif

#if PLATFORM(QT) || PLATFORM(CHROMIUM)
    bool m_selectionMode;
#endif

};

} // namespace WebCore

#endif // Pasteboard_h
