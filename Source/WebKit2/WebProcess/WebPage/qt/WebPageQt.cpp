/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebPage.h"

#include "NotImplemented.h"
#include "PopupMenuClient.h"
#include "WebEditorClient.h"
#include "WebEvent.h"
#include "WebPageProxyMessages.h"
#include "WebPopupMenu.h"
#include "WebProcess.h"
#include <QClipboard>
#include <QGuiApplication>
#include <WebCore/DOMWrapperWorld.h>
#include <WebCore/FocusController.h>
#include <WebCore/Frame.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/Page.h>
#include <WebCore/PageGroup.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <WebCore/Range.h>
#include <WebCore/Settings.h>
#include <WebCore/Text.h>
#include <WebCore/TextIterator.h>

#ifndef VK_UNKNOWN
#define VK_UNKNOWN 0
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11 // CTRL key
#define VK_MENU 0x12 // ALT key
#define VK_PAUSE 0x13 // PAUSE key
#define VK_CAPITAL 0x14 // CAPS LOCK key
#define VK_KANA 0x15 // Input Method Editor (IME) Kana mode
#define VK_HANGUL 0x15 // IME Hangul mode
#define VK_JUNJA 0x17 // IME Junja mode
#define VK_FINAL 0x18 // IME final mode
#define VK_HANJA 0x19 // IME Hanja mode 
#define VK_KANJI 0x19 // IME Kanji mode
#define VK_ESCAPE 0x1B // ESC key
#define VK_CONVERT 0x1C // IME convert
#define VK_NONCONVERT 0x1D // IME nonconvert
#define VK_ACCEPT 0x1E // IME accept
#define VK_MODECHANGE 0x1F // IME mode change request
#define VK_SPACE 0x20 // SPACE key
#define VK_PRIOR 0x21 // PAGE UP key
#define VK_NEXT 0x22 // PAGE DOWN key
#define VK_END 0x23 // END key
#define VK_HOME 0x24 // HOME key
#define VK_LEFT 0x25 // LEFT ARROW key
#define VK_UP 0x26 // UP ARROW key
#define VK_RIGHT 0x27 // RIGHT ARROW key
#define VK_DOWN 0x28 // DOWN ARROW key
#define VK_SELECT 0x29 // SELECT key
#define VK_PRINT 0x2A // PRINT key
#define VK_EXECUTE 0x2B // EXECUTE key
#define VK_SNAPSHOT 0x2C // PRINT SCREEN key
#define VK_INSERT 0x2D // INS key
#define VK_DELETE 0x2E // DEL key
#define VK_HELP 0x2F // HELP key
// Windows 2000/XP: For any country/region, the '.' key
#define VK_OEM_PERIOD 0xBE
#endif

using namespace WebCore;

namespace WebKit {
    
void WebPage::platformInitialize()
{
}

void WebPage::platformPreferencesDidChange(const WebPreferencesStore&)
{
}

static const unsigned CtrlKey = 1 << 0;
static const unsigned AltKey = 1 << 1;
static const unsigned ShiftKey = 1 << 2;

struct KeyDownEntry {
    unsigned virtualKey;
    unsigned modifiers;
    const char* name;
};

struct KeyPressEntry {
    unsigned charCode;
    unsigned modifiers;
    const char* name;
};

static const KeyDownEntry keyDownEntries[] = {
    { VK_LEFT,   0,                  "MoveLeft"                                    },
    { VK_LEFT,   ShiftKey,           "MoveLeftAndModifySelection"                  },
    { VK_LEFT,   CtrlKey,            "MoveWordLeft"                                },
    { VK_LEFT,   CtrlKey | ShiftKey, "MoveWordLeftAndModifySelection"              },
    { VK_RIGHT,  0,                  "MoveRight"                                   },
    { VK_RIGHT,  ShiftKey,           "MoveRightAndModifySelection"                 },
    { VK_RIGHT,  CtrlKey,            "MoveWordRight"                               },
    { VK_RIGHT,  CtrlKey | ShiftKey, "MoveWordRightAndModifySelection"             },
    { VK_UP,     0,                  "MoveUp"                                      },
    { VK_UP,     ShiftKey,           "MoveUpAndModifySelection"                    },
    { VK_PRIOR,  ShiftKey,           "MovePageUpAndModifySelection"                },
    { VK_DOWN,   0,                  "MoveDown"                                    },
    { VK_DOWN,   ShiftKey,           "MoveDownAndModifySelection"                  },
    { VK_NEXT,   ShiftKey,           "MovePageDownAndModifySelection"              },
    { VK_PRIOR,  0,                  "MovePageUp"                                  },
    { VK_NEXT,   0,                  "MovePageDown"                                },
    { VK_HOME,   0,                  "MoveToBeginningOfLine"                       },
    { VK_HOME,   ShiftKey,           "MoveToBeginningOfLineAndModifySelection"     },
    { VK_HOME,   CtrlKey,            "MoveToBeginningOfDocument"                   },
    { VK_HOME,   CtrlKey | ShiftKey, "MoveToBeginningOfDocumentAndModifySelection" },
    
    { VK_END,    0,                  "MoveToEndOfLine"                             },
    { VK_END,    ShiftKey,           "MoveToEndOfLineAndModifySelection"           },
    { VK_END,    CtrlKey,            "MoveToEndOfDocument"                         },
    { VK_END,    CtrlKey | ShiftKey, "MoveToEndOfDocumentAndModifySelection"       },
    
    { VK_BACK,   0,                  "DeleteBackward"                              },
    { VK_BACK,   ShiftKey,           "DeleteBackward"                              },
    { VK_DELETE, 0,                  "DeleteForward"                               },
    { VK_BACK,   CtrlKey,            "DeleteWordBackward"                          },
    { VK_DELETE, CtrlKey,            "DeleteWordForward"                           },
    
    { 'B',       CtrlKey,            "ToggleBold"                                  },
    { 'I',       CtrlKey,            "ToggleItalic"                                },
    
    { VK_ESCAPE, 0,                  "Cancel"                                      },
    { VK_OEM_PERIOD, CtrlKey,        "Cancel"                                      },
    { VK_TAB,    0,                  "InsertTab"                                   },
    { VK_TAB,    ShiftKey,           "InsertBacktab"                               },
    { VK_RETURN, 0,                  "InsertNewline"                               },
    { VK_RETURN, CtrlKey,            "InsertNewline"                               },
    { VK_RETURN, AltKey,             "InsertNewline"                               },
    { VK_RETURN, ShiftKey,           "InsertNewline"                               },
    { VK_RETURN, AltKey | ShiftKey,  "InsertNewline"                               },
    
    // It's not quite clear whether clipboard shortcuts and Undo/Redo should be handled
    // in the application or in WebKit. We chose WebKit.
    { 'C',       CtrlKey,            "Copy"                                        },
    { 'V',       CtrlKey,            "Paste"                                       },
    { 'X',       CtrlKey,            "Cut"                                         },
    { 'A',       CtrlKey,            "SelectAll"                                   },
    { VK_INSERT, CtrlKey,            "Copy"                                        },
    { VK_DELETE, ShiftKey,           "Cut"                                         },
    { VK_INSERT, ShiftKey,           "Paste"                                       },
    { 'Z',       CtrlKey,            "Undo"                                        },
    { 'Z',       CtrlKey | ShiftKey, "Redo"                                        },
};

static const KeyPressEntry keyPressEntries[] = {
    { '\t',   0,                  "InsertTab"                                   },
    { '\t',   ShiftKey,           "InsertBacktab"                               },
    { '\r',   0,                  "InsertNewline"                               },
    { '\r',   CtrlKey,            "InsertNewline"                               },
    { '\r',   AltKey,             "InsertNewline"                               },
    { '\r',   ShiftKey,           "InsertNewline"                               },
    { '\r',   AltKey | ShiftKey,  "InsertNewline"                               },
};

const char* WebPage::interpretKeyEvent(const KeyboardEvent* evt)
{
    ASSERT(evt->type() == eventNames().keydownEvent || evt->type() == eventNames().keypressEvent);
    
    static HashMap<int, const char*>* keyDownCommandsMap = 0;
    static HashMap<int, const char*>* keyPressCommandsMap = 0;
    
    if (!keyDownCommandsMap) {
        keyDownCommandsMap = new HashMap<int, const char*>;
        keyPressCommandsMap = new HashMap<int, const char*>;
        
        for (unsigned i = 0; i < (sizeof(keyDownEntries) / sizeof(keyDownEntries[0])); i++)
            keyDownCommandsMap->set(keyDownEntries[i].modifiers << 16 | keyDownEntries[i].virtualKey, keyDownEntries[i].name);
        
        for (unsigned i = 0; i < (sizeof(keyPressEntries) / sizeof(keyPressEntries[0])); i++)
            keyPressCommandsMap->set(keyPressEntries[i].modifiers << 16 | keyPressEntries[i].charCode, keyPressEntries[i].name);
    }
    
    unsigned modifiers = 0;
    if (evt->shiftKey())
        modifiers |= ShiftKey;
    if (evt->altKey())
        modifiers |= AltKey;
    if (evt->ctrlKey())
        modifiers |= CtrlKey;
    
    if (evt->type() == eventNames().keydownEvent) {
        int mapKey = modifiers << 16 | evt->keyEvent()->windowsVirtualKeyCode();
        return mapKey ? keyDownCommandsMap->get(mapKey) : 0;
    }
    
    int mapKey = modifiers << 16 | evt->charCode();
    return mapKey ? keyPressCommandsMap->get(mapKey) : 0;
}

static inline void scroll(Page* page, ScrollDirection direction, ScrollGranularity granularity)
{
    page->focusController()->focusedOrMainFrame()->eventHandler()->scrollRecursively(direction, granularity);
}

static inline void logicalScroll(Page* page, ScrollLogicalDirection direction, ScrollGranularity granularity)
{
    page->focusController()->focusedOrMainFrame()->eventHandler()->logicalScrollRecursively(direction, granularity);
}

bool WebPage::performDefaultBehaviorForKeyEvent(const WebKeyboardEvent& keyboardEvent)
{
    if (keyboardEvent.type() != WebEvent::KeyDown && keyboardEvent.type() != WebEvent::RawKeyDown)
        return false;

    switch (keyboardEvent.windowsVirtualKeyCode()) {
    case VK_BACK:
        if (keyboardEvent.shiftKey())
            m_page->goForward();
        else
            m_page->goBack();
        break;
    case VK_SPACE:
        logicalScroll(m_page.get(), keyboardEvent.shiftKey() ? ScrollBlockDirectionBackward : ScrollBlockDirectionForward, ScrollByPage);
        break;
    case VK_LEFT:
        scroll(m_page.get(), ScrollLeft, ScrollByLine);
        break;
    case VK_RIGHT:
        scroll(m_page.get(), ScrollRight, ScrollByLine);
        break;
    case VK_UP:
        scroll(m_page.get(), ScrollUp, ScrollByLine);
        break;
    case VK_DOWN:
        scroll(m_page.get(), ScrollDown, ScrollByLine);
        break;
    case VK_HOME:
        logicalScroll(m_page.get(), ScrollBlockDirectionBackward, ScrollByDocument);
        break;
    case VK_END:
        logicalScroll(m_page.get(), ScrollBlockDirectionForward, ScrollByDocument);
        break;
    case VK_PRIOR:
        logicalScroll(m_page.get(), ScrollBlockDirectionBackward, ScrollByPage);
        break;
    case VK_NEXT:
        logicalScroll(m_page.get(), ScrollBlockDirectionForward, ScrollByPage);
        break;
    default:
        return false;
    }

    return true;
}

bool WebPage::platformHasLocalDataForURL(const KURL&)
{
    notImplemented();
    return false;
}

String WebPage::cachedResponseMIMETypeForURL(const KURL&)
{
    notImplemented();
    return String();
}

bool WebPage::platformCanHandleRequest(const ResourceRequest&)
{
    notImplemented();
    return true;
}

String WebPage::cachedSuggestedFilenameForURL(const KURL&)
{
    notImplemented();
    return String();
}

PassRefPtr<SharedBuffer> WebPage::cachedResponseDataForURL(const KURL&)
{
    notImplemented();
    return 0;
}

static Frame* targetFrameForEditing(WebPage* page)
{
    Frame* targetFrame = page->corePage()->focusController()->focusedOrMainFrame();

    if (!targetFrame || !targetFrame->editor())
        return 0;

    Editor* editor = targetFrame->editor();
    if (!editor->canEdit())
        return 0;

    if (editor->hasComposition()) {
        // We should verify the parent node of this IME composition node are
        // editable because JavaScript may delete a parent node of the composition
        // node. In this case, WebKit crashes while deleting texts from the parent
        // node, which doesn't exist any longer.
        if (PassRefPtr<Range> range = editor->compositionRange()) {
            Node* node = range->startContainer();
            if (!node || !node->isContentEditable())
                return 0;
        }
    }
    return targetFrame;
}

void WebPage::confirmComposition(const String& compositionString, int64_t selectionStart, int64_t selectionLength)
{
    Frame* targetFrame = targetFrameForEditing(this);
    if (!targetFrame)
        return;

    Editor* editor = targetFrame->editor();
    editor->confirmComposition(compositionString);

    if (selectionStart == -1)
        return;

    Element* scope = targetFrame->selection()->rootEditableElement();
    RefPtr<Range> selectionRange = TextIterator::rangeFromLocationAndLength(scope, selectionStart, selectionLength);
    ASSERT_WITH_MESSAGE(selectionRange, "Invalid selection: [%lld:%lld] in text of length %d", static_cast<long long>(selectionStart), static_cast<long long>(selectionLength), scope->innerText().length());

    if (selectionRange) {
        VisibleSelection selection(selectionRange.get(), SEL_DEFAULT_AFFINITY);
        targetFrame->selection()->setSelection(selection);
    }

    // FIXME: static_cast<WebEditorClient*>(editor->client())->sendSelectionChangedMessage();
}

void WebPage::setComposition(const String& text, Vector<CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementStart, uint64_t replacementLength)
{
    Frame* targetFrame = targetFrameForEditing(this);
    if (!targetFrame)
        return;

    Element* scope = targetFrame->selection()->rootEditableElement();

    if (targetFrame->selection()->isContentEditable()) {
        if (replacementLength > 0) {
            // The layout needs to be uptodate before setting a selection
            targetFrame->document()->updateLayout();

            RefPtr<Range> replacementRange = TextIterator::rangeFromLocationAndLength(scope, replacementStart, replacementLength);

            targetFrame->editor()->setIgnoreCompositionSelectionChange(true);
            targetFrame->selection()->setSelection(VisibleSelection(replacementRange.get(), SEL_DEFAULT_AFFINITY));
            targetFrame->editor()->setIgnoreCompositionSelectionChange(false);
        }

        targetFrame->editor()->setComposition(text, underlines, selectionStart, selectionEnd);

        // FIXME: static_cast<WebEditorClient*>(targetFrame->editor()->client())->sendSelectionChangedMessage();
    }
}

void WebPage::cancelComposition()
{
    Frame* frame = targetFrameForEditing(this);

    frame->editor()->cancelComposition();

    // FIXME: static_cast<WebEditorClient*>(targetFrame->editor()->client())->sendSelectionChangedMessage();
}

void WebPage::registerApplicationScheme(const String& scheme)
{
    QtNetworkAccessManager* qnam = qobject_cast<QtNetworkAccessManager*>(WebProcess::shared().networkAccessManager());
    if (!qnam)
        return;
    qnam->registerApplicationScheme(this, QString(scheme));
}

void WebPage::receivedApplicationSchemeRequest(const QNetworkRequest& request, QtNetworkReply* reply)
{
    QtNetworkRequestData requestData(request, reply);
    m_applicationSchemeReplies.add(requestData.m_replyUuid, reply);
    send(Messages::WebPageProxy::ResolveApplicationSchemeRequest(requestData));
}

void WebPage::applicationSchemeReply(const QtNetworkReplyData& replyData)
{
    if (!m_applicationSchemeReplies.contains(replyData.m_replyUuid))
        return;

    QtNetworkReply* networkReply = m_applicationSchemeReplies.take(replyData.m_replyUuid);
    networkReply->setReplyData(replyData);
    networkReply->finalize();
}

void WebPage::setUserScripts(const Vector<String>& scripts)
{
    // This works because we keep an unique page group for each Page.
    PageGroup* pageGroup = PageGroup::pageGroup(this->pageGroup()->identifier());
    pageGroup->removeUserScriptsFromWorld(mainThreadNormalWorld());
    for (unsigned i = 0; i < scripts.size(); ++i)
        pageGroup->addUserScriptToWorld(mainThreadNormalWorld(), scripts.at(i), KURL(), nullptr, nullptr, InjectAtDocumentEnd, InjectInTopFrameOnly);
}

void WebPage::selectedIndex(int32_t newIndex)
{
    changeSelectedIndex(newIndex);
}

void WebPage::hidePopupMenu()
{
    if (!m_activePopupMenu)
        return;

    m_activePopupMenu->client()->popupDidHide();
    m_activePopupMenu = 0;
}

bool WebPage::handleMouseReleaseEvent(const PlatformMouseEvent& platformMouseEvent)
{
#ifndef QT_NO_CLIPBOARD
    if (platformMouseEvent.button() != WebCore::MiddleButton)
        return false;

    if (qApp->clipboard()->supportsSelection()) {
        WebCore::Frame* focusFrame = m_page->focusController()->focusedOrMainFrame();
        if (focusFrame) {
            focusFrame->editor()->command(AtomicString("PasteGlobalSelection")).execute();
            return true;
        }
    }
#endif
    return false;
}

} // namespace WebKit
