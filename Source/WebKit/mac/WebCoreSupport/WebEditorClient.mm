/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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

#import "WebEditorClient.h"

#import "DOMCSSStyleDeclarationInternal.h"
#import "DOMDocumentFragmentInternal.h"
#import "DOMHTMLElementInternal.h"
#import "DOMHTMLInputElementInternal.h"
#import "DOMHTMLTextAreaElementInternal.h"
#import "DOMNodeInternal.h"
#import "DOMRangeInternal.h"
#import "WebArchive.h"
#import "WebDataSourceInternal.h"
#import "WebDelegateImplementationCaching.h"
#import "WebDocument.h"
#import "WebEditingDelegatePrivate.h"
#import "WebFormDelegate.h"
#import "WebFrameInternal.h"
#import "WebHTMLView.h"
#import "WebHTMLViewInternal.h"
#import "WebKitLogging.h"
#import "WebKitVersionChecks.h"
#import "WebLocalizableStringsInternal.h"
#import "WebNSURLExtras.h"
#import "WebResourceInternal.h"
#import "WebViewInternal.h"
#import <WebCore/ArchiveResource.h>
#import <WebCore/Document.h>
#import <WebCore/DocumentFragment.h>
#import <WebCore/EditAction.h>
#import <WebCore/EditCommand.h>
#import <WebCore/HTMLInputElement.h>
#import <WebCore/HTMLNames.h>
#import <WebCore/HTMLTextAreaElement.h>
#import <WebCore/KeyboardEvent.h>
#import <WebCore/LegacyWebArchive.h>
#import <WebCore/PlatformKeyboardEvent.h>
#import <WebCore/PlatformString.h>
#import <WebCore/SpellChecker.h>
#import <WebCore/UserTypingGestureIndicator.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <runtime/InitializeThreading.h>
#import <wtf/MainThread.h>
#import <wtf/PassRefPtr.h>

using namespace WebCore;

using namespace HTMLNames;

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
@interface NSSpellChecker (WebNSSpellCheckerDetails)
- (NSString *)languageForWordRange:(NSRange)range inString:(NSString *)string orthography:(NSOrthography *)orthography;
@end
#endif

@interface NSAttributedString (WebNSAttributedStringDetails)
- (id)_initWithDOMRange:(DOMRange*)range;
- (DOMDocumentFragment*)_documentFromRange:(NSRange)range document:(DOMDocument*)document documentAttributes:(NSDictionary *)dict subresources:(NSArray **)subresources;
@end

static WebViewInsertAction kit(EditorInsertAction coreAction)
{
    return static_cast<WebViewInsertAction>(coreAction);
}

static const int InvalidCorrectionPanelTag = 0;


@interface WebEditCommand : NSObject
{
    RefPtr<EditCommand> m_command;   
}

+ (WebEditCommand *)commandWithEditCommand:(PassRefPtr<EditCommand>)command;
- (EditCommand *)command;

@end

@implementation WebEditCommand

+ (void)initialize
{
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    WebCoreObjCFinalizeOnMainThread(self);
}

- (id)initWithEditCommand:(PassRefPtr<EditCommand>)command
{
    ASSERT(command);
    self = [super init];
    if (!self)
        return nil;
    m_command = command;
    return self;
}

- (void)dealloc
{
    if (WebCoreObjCScheduleDeallocateOnMainThread([WebEditCommand class], self))
        return;

    [super dealloc];
}

- (void)finalize
{
    ASSERT_MAIN_THREAD();

    [super finalize];
}

+ (WebEditCommand *)commandWithEditCommand:(PassRefPtr<EditCommand>)command
{
    return [[[WebEditCommand alloc] initWithEditCommand:command] autorelease];
}

- (EditCommand *)command
{
    return m_command.get();
}

@end

@interface WebEditorUndoTarget : NSObject
{
}

- (void)undoEditing:(id)arg;
- (void)redoEditing:(id)arg;

@end

@implementation WebEditorUndoTarget

- (void)undoEditing:(id)arg
{
    ASSERT([arg isKindOfClass:[WebEditCommand class]]);
    [arg command]->unapply();
}

- (void)redoEditing:(id)arg
{
    ASSERT([arg isKindOfClass:[WebEditCommand class]]);
    [arg command]->reapply();
}

@end

void WebEditorClient::pageDestroyed()
{
    delete this;
}

WebEditorClient::WebEditorClient(WebView *webView)
    : m_webView(webView)
    , m_undoTarget([[[WebEditorUndoTarget alloc] init] autorelease])
    , m_haveUndoRedoOperations(false)
{
}

WebEditorClient::~WebEditorClient()
{
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    dismissCorrectionPanel(ReasonForDismissingCorrectionPanelIgnored);
#endif
}

bool WebEditorClient::isContinuousSpellCheckingEnabled()
{
    return [m_webView isContinuousSpellCheckingEnabled];
}

void WebEditorClient::toggleContinuousSpellChecking()
{
    [m_webView toggleContinuousSpellChecking:nil];
}

bool WebEditorClient::isGrammarCheckingEnabled()
{
    return [m_webView isGrammarCheckingEnabled];
}

void WebEditorClient::toggleGrammarChecking()
{
    [m_webView toggleGrammarChecking:nil];
}

int WebEditorClient::spellCheckerDocumentTag()
{
    return [m_webView spellCheckerDocumentTag];
}

bool WebEditorClient::shouldDeleteRange(Range* range)
{
    return [[m_webView _editingDelegateForwarder] webView:m_webView
        shouldDeleteDOMRange:kit(range)];
}

bool WebEditorClient::shouldShowDeleteInterface(HTMLElement* element)
{
    return [[m_webView _editingDelegateForwarder] webView:m_webView
        shouldShowDeleteInterfaceForElement:kit(element)];
}

bool WebEditorClient::smartInsertDeleteEnabled()
{
    return [m_webView smartInsertDeleteEnabled];
}

bool WebEditorClient::isSelectTrailingWhitespaceEnabled()
{
    return [m_webView isSelectTrailingWhitespaceEnabled];
}

bool WebEditorClient::shouldApplyStyle(CSSStyleDeclaration* style, Range* range)
{
    return [[m_webView _editingDelegateForwarder] webView:m_webView
        shouldApplyStyle:kit(style) toElementsInDOMRange:kit(range)];
}

bool WebEditorClient::shouldMoveRangeAfterDelete(Range* range, Range* rangeToBeReplaced)
{
    return [[m_webView _editingDelegateForwarder] webView:m_webView
        shouldMoveRangeAfterDelete:kit(range) replacingRange:kit(rangeToBeReplaced)];
}

bool WebEditorClient::shouldBeginEditing(Range* range)
{
    return [[m_webView _editingDelegateForwarder] webView:m_webView
        shouldBeginEditingInDOMRange:kit(range)];

    return false;
}

bool WebEditorClient::shouldEndEditing(Range* range)
{
    return [[m_webView _editingDelegateForwarder] webView:m_webView
                             shouldEndEditingInDOMRange:kit(range)];
}

bool WebEditorClient::shouldInsertText(const String& text, Range* range, EditorInsertAction action)
{
    WebView* webView = m_webView;
    return [[webView _editingDelegateForwarder] webView:webView shouldInsertText:text replacingDOMRange:kit(range) givenAction:kit(action)];
}

bool WebEditorClient::shouldChangeSelectedRange(Range* fromRange, Range* toRange, EAffinity selectionAffinity, bool stillSelecting)
{
    return [m_webView _shouldChangeSelectedDOMRange:kit(fromRange) toDOMRange:kit(toRange) affinity:kit(selectionAffinity) stillSelecting:stillSelecting];
}

void WebEditorClient::didBeginEditing()
{
    [[NSNotificationCenter defaultCenter] postNotificationName:WebViewDidBeginEditingNotification object:m_webView];
}

void WebEditorClient::respondToChangedContents()
{
    NSView <WebDocumentView> *view = [[[m_webView selectedFrame] frameView] documentView];
    if ([view isKindOfClass:[WebHTMLView class]])
        [(WebHTMLView *)view _updateFontPanel];
    [[NSNotificationCenter defaultCenter] postNotificationName:WebViewDidChangeNotification object:m_webView];    
}

void WebEditorClient::respondToChangedSelection(Frame* frame)
{
    NSView<WebDocumentView> *documentView = [[kit(frame) frameView] documentView];
    if ([documentView isKindOfClass:[WebHTMLView class]])
        [(WebHTMLView *)documentView _selectionChanged];

    // FIXME: This quirk is needed due to <rdar://problem/5009625> - We can phase it out once Aperture can adopt the new behavior on their end
    if (!WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_APERTURE_QUIRK) && [[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.apple.Aperture"])
        return;

    [[NSNotificationCenter defaultCenter] postNotificationName:WebViewDidChangeSelectionNotification object:m_webView];
}

void WebEditorClient::didEndEditing()
{
    [[NSNotificationCenter defaultCenter] postNotificationName:WebViewDidEndEditingNotification object:m_webView];
}

void WebEditorClient::didWriteSelectionToPasteboard()
{
    [[m_webView _editingDelegateForwarder] webView:m_webView didWriteSelectionToPasteboard:[NSPasteboard generalPasteboard]];
}

void WebEditorClient::didSetSelectionTypesForPasteboard()
{
    [[m_webView _editingDelegateForwarder] webView:m_webView didSetSelectionTypesForPasteboard:[NSPasteboard generalPasteboard]];
}

NSString *WebEditorClient::userVisibleString(NSURL *URL)
{
    return [URL _web_userVisibleString];
}

NSURL *WebEditorClient::canonicalizeURL(NSURL *URL)
{
    return [URL _webkit_canonicalize];
}

NSURL *WebEditorClient::canonicalizeURLString(NSString *URLString)
{
    NSURL *URL = nil;
    if ([URLString _webkit_looksLikeAbsoluteURL])
        URL = [[NSURL _web_URLWithUserTypedString:URLString] _webkit_canonicalize];
    return URL;
}

static NSArray *createExcludedElementsForAttributedStringConversion()
{
    NSArray *elements = [[NSArray alloc] initWithObjects: 
        // Omit style since we want style to be inline so the fragment can be easily inserted.
        @"style", 
        // Omit xml so the result is not XHTML.
        @"xml", 
        // Omit tags that will get stripped when converted to a fragment anyway.
        @"doctype", @"html", @"head", @"body", 
        // Omit deprecated tags.
        @"applet", @"basefont", @"center", @"dir", @"font", @"isindex", @"menu", @"s", @"strike", @"u", 
        // Omit object so no file attachments are part of the fragment.
        @"object", nil];
    CFRetain(elements);
    return elements;
}

DocumentFragment* WebEditorClient::documentFragmentFromAttributedString(NSAttributedString *string, Vector<RefPtr<ArchiveResource> >& resources)
{
    static NSArray *excludedElements = createExcludedElementsForAttributedStringConversion();
    
    NSDictionary *dictionary = [[NSDictionary alloc] initWithObjectsAndKeys: excludedElements, NSExcludedElementsDocumentAttribute, 
        nil, @"WebResourceHandler", nil];
    
    NSArray *subResources;
    DOMDocumentFragment* fragment = [string _documentFromRange:NSMakeRange(0, [string length])
                                                      document:[[m_webView mainFrame] DOMDocument]
                                            documentAttributes:dictionary
                                                  subresources:&subResources];
    for (WebResource* resource in subResources)
        resources.append([resource _coreResource]);
    
    [dictionary release];
    return core(fragment);
}

void WebEditorClient::setInsertionPasteboard(NSPasteboard *pasteboard)
{
    [m_webView _setInsertionPasteboard:pasteboard];
}


#ifndef BUILDING_ON_LEOPARD
void WebEditorClient::uppercaseWord()
{
    [m_webView uppercaseWord:nil];
}

void WebEditorClient::lowercaseWord()
{
    [m_webView lowercaseWord:nil];
}

void WebEditorClient::capitalizeWord()
{
    [m_webView capitalizeWord:nil];
}

void WebEditorClient::showSubstitutionsPanel(bool show)
{
    NSPanel *spellingPanel = [[NSSpellChecker sharedSpellChecker] substitutionsPanel];
    if (show)
        [spellingPanel orderFront:nil];
    else
        [spellingPanel orderOut:nil];
}

bool WebEditorClient::substitutionsPanelIsShowing()
{
    return [[[NSSpellChecker sharedSpellChecker] substitutionsPanel] isVisible];
}

void WebEditorClient::toggleSmartInsertDelete()
{
    [m_webView toggleSmartInsertDelete:nil];
}

bool WebEditorClient::isAutomaticQuoteSubstitutionEnabled()
{
    return [m_webView isAutomaticQuoteSubstitutionEnabled];
}

void WebEditorClient::toggleAutomaticQuoteSubstitution()
{
    [m_webView toggleAutomaticQuoteSubstitution:nil];
}

bool WebEditorClient::isAutomaticLinkDetectionEnabled()
{
    return [m_webView isAutomaticLinkDetectionEnabled];
}

void WebEditorClient::toggleAutomaticLinkDetection()
{
    [m_webView toggleAutomaticLinkDetection:nil];
}

bool WebEditorClient::isAutomaticDashSubstitutionEnabled()
{
    return [m_webView isAutomaticDashSubstitutionEnabled];
}

void WebEditorClient::toggleAutomaticDashSubstitution()
{
    [m_webView toggleAutomaticDashSubstitution:nil];
}

bool WebEditorClient::isAutomaticTextReplacementEnabled()
{
    return [m_webView isAutomaticTextReplacementEnabled];
}

void WebEditorClient::toggleAutomaticTextReplacement()
{
    [m_webView toggleAutomaticTextReplacement:nil];
}

bool WebEditorClient::isAutomaticSpellingCorrectionEnabled()
{
    return [m_webView isAutomaticSpellingCorrectionEnabled];
}

void WebEditorClient::toggleAutomaticSpellingCorrection()
{
    [m_webView toggleAutomaticSpellingCorrection:nil];
}
#endif

bool WebEditorClient::shouldInsertNode(Node *node, Range* replacingRange, EditorInsertAction givenAction)
{ 
    return [[m_webView _editingDelegateForwarder] webView:m_webView shouldInsertNode:kit(node) replacingDOMRange:kit(replacingRange) givenAction:(WebViewInsertAction)givenAction];
}

static NSString* undoNameForEditAction(EditAction editAction)
{
    switch (editAction) {
        case EditActionUnspecified: return nil;
        case EditActionSetColor: return UI_STRING_KEY_INTERNAL("Set Color", "Set Color (Undo action name)", "Undo action name");
        case EditActionSetBackgroundColor: return UI_STRING_KEY_INTERNAL("Set Background Color", "Set Background Color (Undo action name)", "Undo action name");
        case EditActionTurnOffKerning: return UI_STRING_KEY_INTERNAL("Turn Off Kerning", "Turn Off Kerning (Undo action name)", "Undo action name");
        case EditActionTightenKerning: return UI_STRING_KEY_INTERNAL("Tighten Kerning", "Tighten Kerning (Undo action name)", "Undo action name");
        case EditActionLoosenKerning: return UI_STRING_KEY_INTERNAL("Loosen Kerning", "Loosen Kerning (Undo action name)", "Undo action name");
        case EditActionUseStandardKerning: return UI_STRING_KEY_INTERNAL("Use Standard Kerning", "Use Standard Kerning (Undo action name)", "Undo action name");
        case EditActionTurnOffLigatures: return UI_STRING_KEY_INTERNAL("Turn Off Ligatures", "Turn Off Ligatures (Undo action name)", "Undo action name");
        case EditActionUseStandardLigatures: return UI_STRING_KEY_INTERNAL("Use Standard Ligatures", "Use Standard Ligatures (Undo action name)", "Undo action name");
        case EditActionUseAllLigatures: return UI_STRING_KEY_INTERNAL("Use All Ligatures", "Use All Ligatures (Undo action name)", "Undo action name");
        case EditActionRaiseBaseline: return UI_STRING_KEY_INTERNAL("Raise Baseline", "Raise Baseline (Undo action name)", "Undo action name");
        case EditActionLowerBaseline: return UI_STRING_KEY_INTERNAL("Lower Baseline", "Lower Baseline (Undo action name)", "Undo action name");
        case EditActionSetTraditionalCharacterShape: return UI_STRING_KEY_INTERNAL("Set Traditional Character Shape", "Set Traditional Character Shape (Undo action name)", "Undo action name");
        case EditActionSetFont: return UI_STRING_KEY_INTERNAL("Set Font", "Set Font (Undo action name)", "Undo action name");
        case EditActionChangeAttributes: return UI_STRING_KEY_INTERNAL("Change Attributes", "Change Attributes (Undo action name)", "Undo action name");
        case EditActionAlignLeft: return UI_STRING_KEY_INTERNAL("Align Left", "Align Left (Undo action name)", "Undo action name");
        case EditActionAlignRight: return UI_STRING_KEY_INTERNAL("Align Right", "Align Right (Undo action name)", "Undo action name");
        case EditActionCenter: return UI_STRING_KEY_INTERNAL("Center", "Center (Undo action name)", "Undo action name");
        case EditActionJustify: return UI_STRING_KEY_INTERNAL("Justify", "Justify (Undo action name)", "Undo action name");
        case EditActionSetWritingDirection: return UI_STRING_KEY_INTERNAL("Set Writing Direction", "Set Writing Direction (Undo action name)", "Undo action name");
        case EditActionSubscript: return UI_STRING_KEY_INTERNAL("Subscript", "Subscript (Undo action name)", "Undo action name");
        case EditActionSuperscript: return UI_STRING_KEY_INTERNAL("Superscript", "Superscript (Undo action name)", "Undo action name");
        case EditActionUnderline: return UI_STRING_KEY_INTERNAL("Underline", "Underline (Undo action name)", "Undo action name");
        case EditActionOutline: return UI_STRING_KEY_INTERNAL("Outline", "Outline (Undo action name)", "Undo action name");
        case EditActionUnscript: return UI_STRING_KEY_INTERNAL("Unscript", "Unscript (Undo action name)", "Undo action name");
        case EditActionDrag: return UI_STRING_KEY_INTERNAL("Drag", "Drag (Undo action name)", "Undo action name");
        case EditActionCut: return UI_STRING_KEY_INTERNAL("Cut", "Cut (Undo action name)", "Undo action name");
        case EditActionPaste: return UI_STRING_KEY_INTERNAL("Paste", "Paste (Undo action name)", "Undo action name");
        case EditActionPasteFont: return UI_STRING_KEY_INTERNAL("Paste Font", "Paste Font (Undo action name)", "Undo action name");
        case EditActionPasteRuler: return UI_STRING_KEY_INTERNAL("Paste Ruler", "Paste Ruler (Undo action name)", "Undo action name");
        case EditActionTyping: return UI_STRING_KEY_INTERNAL("Typing", "Typing (Undo action name)", "Undo action name");
        case EditActionCreateLink: return UI_STRING_KEY_INTERNAL("Create Link", "Create Link (Undo action name)", "Undo action name");
        case EditActionUnlink: return UI_STRING_KEY_INTERNAL("Unlink", "Unlink (Undo action name)", "Undo action name");
        case EditActionInsertList: return UI_STRING_KEY_INTERNAL("Insert List", "Insert List (Undo action name)", "Undo action name");
        case EditActionFormatBlock: return UI_STRING_KEY_INTERNAL("Formatting", "Format Block (Undo action name)", "Undo action name");
        case EditActionIndent: return UI_STRING_KEY_INTERNAL("Indent", "Indent (Undo action name)", "Undo action name");
        case EditActionOutdent: return UI_STRING_KEY_INTERNAL("Outdent", "Outdent (Undo action name)", "Undo action name");
    }
    return nil;
}

void WebEditorClient::registerCommandForUndoOrRedo(PassRefPtr<EditCommand> cmd, bool isRedo)
{
    ASSERT(cmd);
    
    NSUndoManager *undoManager = [m_webView undoManager];
    NSString *actionName = undoNameForEditAction(cmd->editingAction());
    WebEditCommand *command = [WebEditCommand commandWithEditCommand:cmd];
    [undoManager registerUndoWithTarget:m_undoTarget.get() selector:(isRedo ? @selector(redoEditing:) : @selector(undoEditing:)) object:command];
    if (actionName)
        [undoManager setActionName:actionName];
    m_haveUndoRedoOperations = YES;
}

void WebEditorClient::registerCommandForUndo(PassRefPtr<EditCommand> cmd)
{
    registerCommandForUndoOrRedo(cmd, false);
}

void WebEditorClient::registerCommandForRedo(PassRefPtr<EditCommand> cmd)
{
    registerCommandForUndoOrRedo(cmd, true);
}

void WebEditorClient::clearUndoRedoOperations()
{
    if (m_haveUndoRedoOperations) {
        // workaround for <rdar://problem/4645507> NSUndoManager dies
        // with uncaught exception when undo items cleared while
        // groups are open
        NSUndoManager *undoManager = [m_webView undoManager];
        int groupingLevel = [undoManager groupingLevel];
        for (int i = 0; i < groupingLevel; ++i)
            [undoManager endUndoGrouping];
        
        [undoManager removeAllActionsWithTarget:m_undoTarget.get()];
        
        for (int i = 0; i < groupingLevel; ++i)
            [undoManager beginUndoGrouping];
        
        m_haveUndoRedoOperations = NO;
    }    
}

bool WebEditorClient::canCopyCut(Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool WebEditorClient::canPaste(Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool WebEditorClient::canUndo() const
{
    return [[m_webView undoManager] canUndo];
}

bool WebEditorClient::canRedo() const
{
    return [[m_webView undoManager] canRedo];
}

void WebEditorClient::undo()
{
    if (canUndo())
        [[m_webView undoManager] undo];
}

void WebEditorClient::redo()
{
    if (canRedo())
        [[m_webView undoManager] redo];    
}

void WebEditorClient::handleKeyboardEvent(KeyboardEvent* event)
{
    Frame* frame = event->target()->toNode()->document()->frame();
    WebHTMLView *webHTMLView = [[kit(frame) frameView] documentView];
    if ([webHTMLView _interpretKeyEvent:event savingCommands:NO])
        event->setDefaultHandled();
}

void WebEditorClient::handleInputMethodKeydown(KeyboardEvent* event)
{
    Frame* frame = event->target()->toNode()->document()->frame();
    WebHTMLView *webHTMLView = [[kit(frame) frameView] documentView];
    if ([webHTMLView _interpretKeyEvent:event savingCommands:YES])
        event->setDefaultHandled();
}

#define FormDelegateLog(ctrl)  LOG(FormDelegate, "control=%@", ctrl)

void WebEditorClient::textFieldDidBeginEditing(Element* element)
{
    if (!element->hasTagName(inputTag))
        return;

    DOMHTMLInputElement* inputElement = kit(static_cast<HTMLInputElement*>(element));
    FormDelegateLog(inputElement);
    CallFormDelegate(m_webView, @selector(textFieldDidBeginEditing:inFrame:), inputElement, kit(element->document()->frame()));
}

void WebEditorClient::textFieldDidEndEditing(Element* element)
{
    if (!element->hasTagName(inputTag))
        return;

    DOMHTMLInputElement* inputElement = kit(static_cast<HTMLInputElement*>(element));
    FormDelegateLog(inputElement);
    CallFormDelegate(m_webView, @selector(textFieldDidEndEditing:inFrame:), inputElement, kit(element->document()->frame()));
}

void WebEditorClient::textDidChangeInTextField(Element* element)
{
    if (!element->hasTagName(inputTag))
        return;

    if (!UserTypingGestureIndicator::processingUserTypingGesture() || UserTypingGestureIndicator::focusedElementAtGestureStart() != element)
        return;

    DOMHTMLInputElement* inputElement = kit(static_cast<HTMLInputElement*>(element));
    FormDelegateLog(inputElement);
    CallFormDelegate(m_webView, @selector(textDidChangeInTextField:inFrame:), inputElement, kit(element->document()->frame()));
}

static SEL selectorForKeyEvent(KeyboardEvent* event)
{
    // FIXME: This helper function is for the auto-fill code so we can pass a selector to the form delegate.  
    // Eventually, we should move all of the auto-fill code down to WebKit and remove the need for this function by
    // not relying on the selector in the new implementation.
    // The key identifiers are from <http://www.w3.org/TR/DOM-Level-3-Events/keyset.html#KeySet-Set>
    const String& key = event->keyIdentifier();
    if (key == "Up")
        return @selector(moveUp:);
    if (key == "Down")
        return @selector(moveDown:);
    if (key == "U+001B")
        return @selector(cancel:);
    if (key == "U+0009") {
        if (event->shiftKey())
            return @selector(insertBacktab:);
        return @selector(insertTab:);
    }
    if (key == "Enter")
        return @selector(insertNewline:);
    return 0;
}

bool WebEditorClient::doTextFieldCommandFromEvent(Element* element, KeyboardEvent* event)
{
    if (!element->hasTagName(inputTag))
        return NO;

    DOMHTMLInputElement* inputElement = kit(static_cast<HTMLInputElement*>(element));
    FormDelegateLog(inputElement);
    if (SEL commandSelector = selectorForKeyEvent(event))
        return CallFormDelegateReturningBoolean(NO, m_webView, @selector(textField:doCommandBySelector:inFrame:), inputElement, commandSelector, kit(element->document()->frame()));
    return NO;
}

void WebEditorClient::textWillBeDeletedInTextField(Element* element)
{
    if (!element->hasTagName(inputTag))
        return;

    DOMHTMLInputElement* inputElement = kit(static_cast<HTMLInputElement*>(element));
    FormDelegateLog(inputElement);
    // We're using the deleteBackward selector for all deletion operations since the autofill code treats all deletions the same way.
    CallFormDelegateReturningBoolean(NO, m_webView, @selector(textField:doCommandBySelector:inFrame:), inputElement, @selector(deleteBackward:), kit(element->document()->frame()));
}

void WebEditorClient::textDidChangeInTextArea(Element* element)
{
    if (!element->hasTagName(textareaTag))
        return;

    DOMHTMLTextAreaElement* textAreaElement = kit(static_cast<HTMLTextAreaElement*>(element));
    FormDelegateLog(textAreaElement);
    CallFormDelegate(m_webView, @selector(textDidChangeInTextArea:inFrame:), textAreaElement, kit(element->document()->frame()));
}

void WebEditorClient::ignoreWordInSpellDocument(const String& text)
{
    [[NSSpellChecker sharedSpellChecker] ignoreWord:text 
                             inSpellDocumentWithTag:spellCheckerDocumentTag()];
}

void WebEditorClient::learnWord(const String& text)
{
    [[NSSpellChecker sharedSpellChecker] learnWord:text];
}

void WebEditorClient::checkSpellingOfString(const UChar* text, int length, int* misspellingLocation, int* misspellingLength)
{
    NSString* textString = [[NSString alloc] initWithCharactersNoCopy:const_cast<UChar*>(text) length:length freeWhenDone:NO];
    NSRange range = [[NSSpellChecker sharedSpellChecker] checkSpellingOfString:textString startingAt:0 language:nil wrap:NO inSpellDocumentWithTag:spellCheckerDocumentTag() wordCount:NULL];
    [textString release];
    if (misspellingLocation) {
        // WebCore expects -1 to represent "not found"
        if (range.location == NSNotFound)
            *misspellingLocation = -1;
        else
            *misspellingLocation = range.location;
    }
    
    if (misspellingLength)
        *misspellingLength = range.length;
}

String WebEditorClient::getAutoCorrectSuggestionForMisspelledWord(const String& inputWord)
{
    // This method can be implemented using customized algorithms for the particular browser.
    // Currently, it computes an empty string.
    return String();
}

void WebEditorClient::checkGrammarOfString(const UChar* text, int length, Vector<GrammarDetail>& details, int* badGrammarLocation, int* badGrammarLength)
{
    NSArray *grammarDetails;
    NSString* textString = [[NSString alloc] initWithCharactersNoCopy:const_cast<UChar*>(text) length:length freeWhenDone:NO];
    NSRange range = [[NSSpellChecker sharedSpellChecker] checkGrammarOfString:textString startingAt:0 language:nil wrap:NO inSpellDocumentWithTag:spellCheckerDocumentTag() details:&grammarDetails];
    [textString release];
    if (badGrammarLocation)
        // WebCore expects -1 to represent "not found"
        *badGrammarLocation = (range.location == NSNotFound) ? -1 : static_cast<int>(range.location);
    if (badGrammarLength)
        *badGrammarLength = range.length;
    for (NSDictionary *detail in grammarDetails) {
        ASSERT(detail);
        GrammarDetail grammarDetail;
        NSValue *detailRangeAsNSValue = [detail objectForKey:NSGrammarRange];
        ASSERT(detailRangeAsNSValue);
        NSRange detailNSRange = [detailRangeAsNSValue rangeValue];
        ASSERT(detailNSRange.location != NSNotFound);
        ASSERT(detailNSRange.length > 0);
        grammarDetail.location = detailNSRange.location;
        grammarDetail.length = detailNSRange.length;
        grammarDetail.userDescription = [detail objectForKey:NSGrammarUserDescription];
        NSArray *guesses = [detail objectForKey:NSGrammarCorrections];
        for (NSString *guess in guesses)
            grammarDetail.guesses.append(String(guess));
        details.append(grammarDetail);
    }
}

#ifndef BUILDING_ON_LEOPARD
static Vector<TextCheckingResult> core(NSArray *incomingResults, TextCheckingTypeMask checkingTypes)
{
    Vector<TextCheckingResult> results;

    for (NSTextCheckingResult *incomingResult in incomingResults) {
        NSRange resultRange = [incomingResult range];
        NSTextCheckingType resultType = [incomingResult resultType];
        ASSERT(resultRange.location != NSNotFound);
        ASSERT(resultRange.length > 0);
        if (NSTextCheckingTypeSpelling == resultType && 0 != (checkingTypes & NSTextCheckingTypeSpelling)) {
            TextCheckingResult result;
            result.type = TextCheckingTypeSpelling;
            result.location = resultRange.location;
            result.length = resultRange.length;
            results.append(result);
        } else if (NSTextCheckingTypeGrammar == resultType && 0 != (checkingTypes & NSTextCheckingTypeGrammar)) {
            TextCheckingResult result;
            NSArray *details = [incomingResult grammarDetails];
            result.type = TextCheckingTypeGrammar;
            result.location = resultRange.location;
            result.length = resultRange.length;
            for (NSDictionary *incomingDetail in details) {
                ASSERT(incomingDetail);
                GrammarDetail detail;
                NSValue *detailRangeAsNSValue = [incomingDetail objectForKey:NSGrammarRange];
                ASSERT(detailRangeAsNSValue);
                NSRange detailNSRange = [detailRangeAsNSValue rangeValue];
                ASSERT(detailNSRange.location != NSNotFound);
                ASSERT(detailNSRange.length > 0);
                detail.location = detailNSRange.location;
                detail.length = detailNSRange.length;
                detail.userDescription = [incomingDetail objectForKey:NSGrammarUserDescription];
                NSArray *guesses = [incomingDetail objectForKey:NSGrammarCorrections];
                for (NSString *guess in guesses)
                    detail.guesses.append(String(guess));
                result.details.append(detail);
            }
            results.append(result);
        } else if (NSTextCheckingTypeLink == resultType && 0 != (checkingTypes & NSTextCheckingTypeLink)) {
            TextCheckingResult result;
            result.type = TextCheckingTypeLink;
            result.location = resultRange.location;
            result.length = resultRange.length;
            result.replacement = [[incomingResult URL] absoluteString];
            results.append(result);
        } else if (NSTextCheckingTypeQuote == resultType && 0 != (checkingTypes & NSTextCheckingTypeQuote)) {
            TextCheckingResult result;
            result.type = TextCheckingTypeQuote;
            result.location = resultRange.location;
            result.length = resultRange.length;
            result.replacement = [incomingResult replacementString];
            results.append(result);
        } else if (NSTextCheckingTypeDash == resultType && 0 != (checkingTypes & NSTextCheckingTypeDash)) {
            TextCheckingResult result;
            result.type = TextCheckingTypeDash;
            result.location = resultRange.location;
            result.length = resultRange.length;
            result.replacement = [incomingResult replacementString];
            results.append(result);
        } else if (NSTextCheckingTypeReplacement == resultType && 0 != (checkingTypes & NSTextCheckingTypeReplacement)) {
            TextCheckingResult result;
            result.type = TextCheckingTypeReplacement;
            result.location = resultRange.location;
            result.length = resultRange.length;
            result.replacement = [incomingResult replacementString];
            results.append(result);
        } else if (NSTextCheckingTypeCorrection == resultType && 0 != (checkingTypes & NSTextCheckingTypeCorrection)) {
            TextCheckingResult result;
            result.type = TextCheckingTypeCorrection;
            result.location = resultRange.location;
            result.length = resultRange.length;
            result.replacement = [incomingResult replacementString];
            results.append(result);
        }
    }

    return results;
}
#endif

void WebEditorClient::checkTextOfParagraph(const UChar* text, int length, TextCheckingTypeMask checkingTypes, Vector<TextCheckingResult>& results)
{
#ifndef BUILDING_ON_LEOPARD
    NSString *textString = [[NSString alloc] initWithCharactersNoCopy:const_cast<UChar*>(text) length:length freeWhenDone:NO];
    NSArray *incomingResults = [[NSSpellChecker sharedSpellChecker] checkString:textString range:NSMakeRange(0, [textString length]) types:(checkingTypes|NSTextCheckingTypeOrthography) options:nil inSpellDocumentWithTag:spellCheckerDocumentTag() orthography:NULL wordCount:NULL];
    [textString release];
    results = core(incomingResults, checkingTypes);
#endif
}

void WebEditorClient::updateSpellingUIWithGrammarString(const String& badGrammarPhrase, const GrammarDetail& grammarDetail)
{
    NSMutableArray* corrections = [NSMutableArray array];
    for (unsigned i = 0; i < grammarDetail.guesses.size(); i++) {
        NSString* guess = grammarDetail.guesses[i];
        [corrections addObject:guess];
    }
    NSRange grammarRange = NSMakeRange(grammarDetail.location, grammarDetail.length);
    NSString* grammarUserDescription = grammarDetail.userDescription;
    NSMutableDictionary* grammarDetailDict = [NSDictionary dictionaryWithObjectsAndKeys:[NSValue valueWithRange:grammarRange], NSGrammarRange, grammarUserDescription, NSGrammarUserDescription, corrections, NSGrammarCorrections, nil];
    
    [[NSSpellChecker sharedSpellChecker] updateSpellingPanelWithGrammarString:badGrammarPhrase detail:grammarDetailDict];
}

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
void WebEditorClient::showCorrectionPanel(CorrectionPanelInfo::PanelType panelType, const FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings)
{
    m_correctionPanel.show(m_webView, panelType, boundingBoxOfReplacedString, replacedString, replacementString, alternativeReplacementStrings);
}

void WebEditorClient::dismissCorrectionPanel(ReasonForDismissingCorrectionPanel reasonForDismissing)
{
    m_correctionPanel.dismiss(reasonForDismissing);
}

String WebEditorClient::dismissCorrectionPanelSoon(ReasonForDismissingCorrectionPanel reasonForDismissing)
{
    return m_correctionPanel.dismiss(reasonForDismissing);
}

void WebEditorClient::recordAutocorrectionResponse(EditorClient::AutocorrectionResponseType responseType, const String& replacedString, const String& replacementString)
{
    NSCorrectionResponse response = responseType == EditorClient::AutocorrectionReverted ? NSCorrectionResponseReverted : NSCorrectionResponseEdited;
    CorrectionPanel::recordAutocorrectionResponse(m_webView, response, replacedString, replacementString);
}
#endif

void WebEditorClient::updateSpellingUIWithMisspelledWord(const String& misspelledWord)
{
    [[NSSpellChecker sharedSpellChecker] updateSpellingPanelWithMisspelledWord:misspelledWord];
}

void WebEditorClient::showSpellingUI(bool show)
{
    NSPanel *spellingPanel = [[NSSpellChecker sharedSpellChecker] spellingPanel];
    if (show)
        [spellingPanel orderFront:nil];
    else
        [spellingPanel orderOut:nil];
}

bool WebEditorClient::spellingUIIsShowing()
{
    return [[[NSSpellChecker sharedSpellChecker] spellingPanel] isVisible];
}

void WebEditorClient::getGuessesForWord(const String& word, const String& context, Vector<String>& guesses) {
    guesses.clear();
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    NSString* language = nil;
    NSOrthography* orthography = nil;
    NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
    if (context.length()) {
        [checker checkString:context range:NSMakeRange(0, context.length()) types:NSTextCheckingTypeOrthography options:0 inSpellDocumentWithTag:spellCheckerDocumentTag() orthography:&orthography wordCount:0];
        language = [checker languageForWordRange:NSMakeRange(0, context.length()) inString:context orthography:orthography];
    }
    NSArray* stringsArray = [checker guessesForWordRange:NSMakeRange(0, word.length()) inString:word language:language inSpellDocumentWithTag:spellCheckerDocumentTag()];
#else
    NSArray* stringsArray = [[NSSpellChecker sharedSpellChecker] guessesForWord:word];
#endif
    unsigned count = [stringsArray count];

    if (count > 0) {
        NSEnumerator* enumerator = [stringsArray objectEnumerator];
        NSString* string;
        while ((string = [enumerator nextObject]) != nil)
            guesses.append(string);
    }
}

void WebEditorClient::willSetInputMethodState()
{
}

void WebEditorClient::setInputMethodState(bool)
{
}

#ifndef BUILDING_ON_LEOPARD
@interface WebEditorSpellCheckResponder : NSObject
{
    WebCore::SpellChecker* _sender;
    int _sequence;
    TextCheckingTypeMask _types;
    RetainPtr<NSArray> _results;
}
- (id)initWithSender:(WebCore::SpellChecker*)sender sequence:(int)sequence types:(WebCore::TextCheckingTypeMask)types results:(NSArray*)results;
- (void)perform;
@end

@implementation WebEditorSpellCheckResponder
- (id)initWithSender:(WebCore::SpellChecker*)sender sequence:(int)sequence types:(WebCore::TextCheckingTypeMask)types results:(NSArray*)results
{
    self = [super init];
    if (!self)
        return nil;
    _sender = sender;
    _sequence = sequence;
    _types = types;
    _results = results;
    return self;
}

- (void)perform
{
    _sender->didCheck(_sequence, core(_results.get(), _types));
}

@end
#endif

void WebEditorClient::requestCheckingOfString(WebCore::SpellChecker* sender, int sequence, WebCore::TextCheckingTypeMask checkingTypes, const String& text)
{
#ifndef BUILDING_ON_LEOPARD
    NSRange range = NSMakeRange(0, text.length());
    NSRunLoop* currentLoop = [NSRunLoop currentRunLoop];
    [[NSSpellChecker sharedSpellChecker] requestCheckingOfString:text range:range types:NSTextCheckingAllSystemTypes options:0 inSpellDocumentWithTag:0 
                                         completionHandler:^(NSInteger, NSArray* results, NSOrthography*, NSInteger) {
            [currentLoop performSelector:@selector(perform) 
                                  target:[[[WebEditorSpellCheckResponder alloc] initWithSender:sender sequence:sequence types:checkingTypes results:results] autorelease]
                                argument:nil order:0 modes:[NSArray arrayWithObject:NSDefaultRunLoopMode]];
        }];
#endif
}
