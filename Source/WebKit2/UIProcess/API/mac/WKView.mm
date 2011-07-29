/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WKView.h"

#import "AttributedString.h"
#import "DataReference.h"
#import "DrawingAreaProxyImpl.h"
#import "EditorState.h"
#import "FindIndicator.h"
#import "FindIndicatorWindow.h"
#import "LayerTreeContext.h"
#import "Logging.h"
#import "NativeWebKeyboardEvent.h"
#import "NativeWebMouseEvent.h"
#import "NativeWebWheelEvent.h"
#import "PDFViewController.h"
#import "PageClientImpl.h"
#import "PasteboardTypes.h"
#import "RunLoop.h"
#import "TextChecker.h"
#import "TextCheckerState.h"
#import "WKAPICast.h"
#import "WKFullScreenWindowController.h"
#import "WKPrintingView.h"
#import "WKStringCF.h"
#import "WKTextInputWindowController.h"
#import "WKViewInternal.h"
#import "WKViewPrivate.h"
#import "WebContext.h"
#import "WebEventFactory.h"
#import "WebFullScreenManagerProxy.h"
#import "WebPage.h"
#import "WebPageProxy.h"
#import "WebProcessProxy.h"
#import "WebSystemInterface.h"
#import <QuartzCore/QuartzCore.h>
#import <WebCore/ColorMac.h>
#import <WebCore/DragController.h>
#import <WebCore/DragData.h>
#import <WebCore/LocalizedStrings.h>
#import <WebCore/FloatRect.h>
#import <WebCore/IntRect.h>
#import <WebCore/KeyboardEvent.h>
#import <WebCore/PlatformMouseEvent.h>
#import <WebCore/PlatformScreen.h>
#import <WebCore/Region.h>
#import <WebKitSystemInterface.h>
#import <wtf/RefPtr.h>
#import <wtf/RetainPtr.h>

@interface NSApplication (WKNSApplicationDetails)
- (void)speakString:(NSString *)string;
- (void)_setCurrentEvent:(NSEvent *)event;
@end

@interface NSObject (WKNSTextInputContextDetails)
- (BOOL)wantsToHandleMouseEvents;
- (BOOL)handleMouseEvent:(NSEvent *)event;
@end

@interface NSWindow (WKNSWindowDetails)
- (NSRect)_growBoxRect;
- (id)_growBoxOwner;
- (void)_setShowOpaqueGrowBoxForOwner:(id)owner;
- (BOOL)_updateGrowBoxForWindowFrameChange;
- (NSRect)_intersectBottomCornersWithRect:(NSRect)viewRect;
- (void)_maskRoundedBottomCorners:(NSRect)clipRect;
@end

using namespace WebKit;
using namespace WebCore;

namespace WebKit {

typedef id <NSValidatedUserInterfaceItem> ValidationItem;
typedef Vector<RetainPtr<ValidationItem> > ValidationVector;
typedef HashMap<String, ValidationVector> ValidationMap;

}

struct WKViewInterpretKeyEventsParameters {
    bool eventInterpretationHadSideEffects;
    bool consumedByIM;
    bool executingSavedKeypressCommands;
    Vector<KeypressCommand>* commands;
};

@interface WKViewData : NSObject {
@public
    OwnPtr<PageClientImpl> _pageClient;
    RefPtr<WebPageProxy> _page;

    // For ToolTips.
    NSToolTipTag _lastToolTipTag;
    id _trackingRectOwner;
    void* _trackingRectUserData;

    RetainPtr<NSView> _layerHostingView;

    RetainPtr<id> _remoteAccessibilityChild;
    
    // For asynchronous validation.
    ValidationMap _validationMap;

    OwnPtr<PDFViewController> _pdfViewController;

    OwnPtr<FindIndicatorWindow> _findIndicatorWindow;
    // We keep here the event when resending it to
    // the application to distinguish the case of a new event from one 
    // that has been already sent to WebCore.
    RetainPtr<NSEvent> _keyDownEventBeingResent;
    WKViewInterpretKeyEventsParameters* _interpretKeyEventsParameters;

    NSSize _resizeScrollOffset;

    // The identifier of the plug-in we want to send complex text input to, or 0 if there is none.
    uint64_t _pluginComplexTextInputIdentifier;

    bool _inBecomeFirstResponder;
    bool _inResignFirstResponder;
    NSEvent *_mouseDownEvent;
    BOOL _ignoringMouseDraggedEvents;
    BOOL _dragHasStarted;

#if ENABLE(GESTURE_EVENTS)
    id _endGestureMonitor;
#endif
    
#if ENABLE(FULLSCREEN_API)
    RetainPtr<WKFullScreenWindowController> _fullScreenWindowController;
#endif

    BOOL _hasSpellCheckerDocumentTag;
    NSInteger _spellCheckerDocumentTag;

    BOOL _inSecureInputState;

    NSRect _windowBottomCornerIntersectionRect;
}
@end

@interface WKResponderChainSink : NSResponder {
    NSResponder *_lastResponderInChain;
    bool _didReceiveUnhandledCommand;
}
- (id)initWithResponderChain:(NSResponder *)chain;
- (void)detach;
- (bool)didReceiveUnhandledCommand;
@end

@implementation WKViewData
@end

@implementation WKView

- (id)initWithFrame:(NSRect)frame
{
    return [self initWithFrame:frame contextRef:toAPI(WebContext::sharedProcessContext())];
}

- (id)initWithFrame:(NSRect)frame contextRef:(WKContextRef)contextRef
{   
    return [self initWithFrame:frame contextRef:contextRef pageGroupRef:nil];
}

- (void)_registerDraggedTypes
{
    NSMutableSet *types = [[NSMutableSet alloc] initWithArray:PasteboardTypes::forEditing()];
    [types addObjectsFromArray:PasteboardTypes::forURL()];
    [self registerForDraggedTypes:[types allObjects]];
    [types release];
}

- (void)_updateRemoteAccessibilityRegistration:(BOOL)registerProcess
{
    // When the tree is connected/disconnected, the remote accessibility registration
    // needs to be updated with the pid of the remote process. If the process is going
    // away, that information is not present in WebProcess
    pid_t pid = 0;
    if (registerProcess && _data->_page->process())
        pid = _data->_page->process()->processIdentifier();
    else if (!registerProcess) {
        pid = WKAXRemoteProcessIdentifier(_data->_remoteAccessibilityChild.get());
        _data->_remoteAccessibilityChild = nil;
    }
    if (pid)
        WKAXRegisterRemoteProcess(registerProcess, pid); 
}

- (id)initWithFrame:(NSRect)frame contextRef:(WKContextRef)contextRef pageGroupRef:(WKPageGroupRef)pageGroupRef
{
    self = [super initWithFrame:frame];
    if (!self)
        return nil;

    [NSApp registerServicesMenuSendTypes:PasteboardTypes::forSelection() returnTypes:PasteboardTypes::forEditing()];

    InitWebCoreSystemInterface();
    RunLoop::initializeMainRunLoop();

    NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:frame
                                                                options:(NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect)
                                                                  owner:self
                                                               userInfo:nil];
    [self addTrackingArea:trackingArea];
    [trackingArea release];

    _data = [[WKViewData alloc] init];

    _data->_pageClient = PageClientImpl::create(self);
    _data->_page = toImpl(contextRef)->createWebPage(_data->_pageClient.get(), toImpl(pageGroupRef));
    _data->_page->initializeWebPage();
#if ENABLE(FULLSCREEN_API)
    _data->_page->fullScreenManager()->setWebView(self);
#endif
    _data->_mouseDownEvent = nil;
    _data->_ignoringMouseDraggedEvents = NO;

    [self _registerDraggedTypes];

    WebContext::statistics().wkViewCount++;

    return self;
}

- (void)dealloc
{
    _data->_page->close();

    ASSERT(!_data->_inSecureInputState);

    [_data release];
    _data = nil;

    WebContext::statistics().wkViewCount--;

    [super dealloc];
}

- (WKPageRef)pageRef
{
    return toAPI(_data->_page.get());
}

- (void)setDrawsBackground:(BOOL)drawsBackground
{
    _data->_page->setDrawsBackground(drawsBackground);
}

- (BOOL)drawsBackground
{
    return _data->_page->drawsBackground();
}

- (void)setDrawsTransparentBackground:(BOOL)drawsTransparentBackground
{
    _data->_page->setDrawsTransparentBackground(drawsTransparentBackground);
}

- (BOOL)drawsTransparentBackground
{
    return _data->_page->drawsTransparentBackground();
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    NSSelectionDirection direction = [[self window] keyViewSelectionDirection];

    _data->_inBecomeFirstResponder = true;
    
    [self _updateSecureInputState];
    _data->_page->viewStateDidChange(WebPageProxy::ViewIsFocused);

    _data->_inBecomeFirstResponder = false;

    if (direction != NSDirectSelection)
        _data->_page->setInitialFocus(direction == NSSelectingNext);

    return YES;
}

- (BOOL)resignFirstResponder
{
    _data->_inResignFirstResponder = true;

    if (_data->_page->editorState().hasComposition && !_data->_page->editorState().shouldIgnoreCompositionSelectionChange)
        _data->_page->confirmCompositionWithoutDisturbingSelection();
    [self _resetTextInputState];
    
    if (!_data->_page->maintainsInactiveSelection())
        _data->_page->clearSelection();
    
    _data->_page->viewStateDidChange(WebPageProxy::ViewIsFocused);

    _data->_inResignFirstResponder = false;

    return YES;
}

- (void)viewWillStartLiveResize
{
    _data->_page->viewWillStartLiveResize();
}

- (void)viewDidEndLiveResize
{
    _data->_page->viewWillEndLiveResize();
}

- (BOOL)isFlipped
{
    return YES;
}

- (void)setFrame:(NSRect)rect andScrollBy:(NSSize)offset
{
    ASSERT(NSEqualSizes(_data->_resizeScrollOffset, NSZeroSize));

    _data->_resizeScrollOffset = offset;
    [self setFrame:rect];
}

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];
    
    if (![self frameSizeUpdatesDisabled])
        [self _setDrawingAreaSize:size];
}

- (void)_updateWindowAndViewFrames
{
    NSWindow *window = [self window];
    ASSERT(window);
    
    NSRect windowFrameInScreenCoordinates = [window frame];
    NSRect viewFrameInWindowCoordinates = [self convertRect:[self frame] toView:nil];
    NSPoint accessibilityPosition = [[self accessibilityAttributeValue:NSAccessibilityPositionAttribute] pointValue];
    
    _data->_page->windowAndViewFramesChanged(enclosingIntRect(windowFrameInScreenCoordinates), enclosingIntRect(viewFrameInWindowCoordinates), IntPoint(accessibilityPosition));
}

- (void)renewGState
{
    // Hide the find indicator.
    _data->_findIndicatorWindow = nullptr;

    // Update the view frame.
    if ([self window])
        [self _updateWindowAndViewFrames];

    [super renewGState];
}

typedef HashMap<SEL, String> SelectorNameMap;

// Map selectors into Editor command names.
// This is not needed for any selectors that have the same name as the Editor command.
static const SelectorNameMap* createSelectorExceptionMap()
{
    SelectorNameMap* map = new HashMap<SEL, String>;
    
    map->add(@selector(insertNewlineIgnoringFieldEditor:), "InsertNewline");
    map->add(@selector(insertParagraphSeparator:), "InsertNewline");
    map->add(@selector(insertTabIgnoringFieldEditor:), "InsertTab");
    map->add(@selector(pageDown:), "MovePageDown");
    map->add(@selector(pageDownAndModifySelection:), "MovePageDownAndModifySelection");
    map->add(@selector(pageUp:), "MovePageUp");
    map->add(@selector(pageUpAndModifySelection:), "MovePageUpAndModifySelection");
    map->add(@selector(scrollPageDown:), "ScrollPageForward");
    map->add(@selector(scrollPageUp:), "ScrollPageBackward");
    
    return map;
}

static String commandNameForSelector(SEL selector)
{
    // Check the exception map first.
    static const SelectorNameMap* exceptionMap = createSelectorExceptionMap();
    SelectorNameMap::const_iterator it = exceptionMap->find(selector);
    if (it != exceptionMap->end())
        return it->second;
    
    // Remove the trailing colon.
    // No need to capitalize the command name since Editor command names are
    // not case sensitive.
    const char* selectorName = sel_getName(selector);
    size_t selectorNameLength = strlen(selectorName);
    if (selectorNameLength < 2 || selectorName[selectorNameLength - 1] != ':')
        return String();
    return String(selectorName, selectorNameLength - 1);
}

// Editing commands

#define WEBCORE_COMMAND(command) - (void)command:(id)sender { _data->_page->executeEditCommand(commandNameForSelector(_cmd)); }

WEBCORE_COMMAND(alignCenter)
WEBCORE_COMMAND(alignJustified)
WEBCORE_COMMAND(alignLeft)
WEBCORE_COMMAND(alignRight)
WEBCORE_COMMAND(copy)
WEBCORE_COMMAND(cut)
WEBCORE_COMMAND(delete)
WEBCORE_COMMAND(deleteBackward)
WEBCORE_COMMAND(deleteBackwardByDecomposingPreviousCharacter)
WEBCORE_COMMAND(deleteForward)
WEBCORE_COMMAND(deleteToBeginningOfLine)
WEBCORE_COMMAND(deleteToBeginningOfParagraph)
WEBCORE_COMMAND(deleteToEndOfLine)
WEBCORE_COMMAND(deleteToEndOfParagraph)
WEBCORE_COMMAND(deleteToMark)
WEBCORE_COMMAND(deleteWordBackward)
WEBCORE_COMMAND(deleteWordForward)
WEBCORE_COMMAND(ignoreSpelling)
WEBCORE_COMMAND(indent)
WEBCORE_COMMAND(insertBacktab)
WEBCORE_COMMAND(insertLineBreak)
WEBCORE_COMMAND(insertNewline)
WEBCORE_COMMAND(insertNewlineIgnoringFieldEditor)
WEBCORE_COMMAND(insertParagraphSeparator)
WEBCORE_COMMAND(insertTab)
WEBCORE_COMMAND(insertTabIgnoringFieldEditor)
WEBCORE_COMMAND(makeTextWritingDirectionLeftToRight)
WEBCORE_COMMAND(makeTextWritingDirectionNatural)
WEBCORE_COMMAND(makeTextWritingDirectionRightToLeft)
WEBCORE_COMMAND(moveBackward)
WEBCORE_COMMAND(moveBackwardAndModifySelection)
WEBCORE_COMMAND(moveDown)
WEBCORE_COMMAND(moveDownAndModifySelection)
WEBCORE_COMMAND(moveForward)
WEBCORE_COMMAND(moveForwardAndModifySelection)
WEBCORE_COMMAND(moveLeft)
WEBCORE_COMMAND(moveLeftAndModifySelection)
WEBCORE_COMMAND(moveParagraphBackwardAndModifySelection)
WEBCORE_COMMAND(moveParagraphForwardAndModifySelection)
WEBCORE_COMMAND(moveRight)
WEBCORE_COMMAND(moveRightAndModifySelection)
WEBCORE_COMMAND(moveToBeginningOfDocument)
WEBCORE_COMMAND(moveToBeginningOfDocumentAndModifySelection)
WEBCORE_COMMAND(moveToBeginningOfLine)
WEBCORE_COMMAND(moveToBeginningOfLineAndModifySelection)
WEBCORE_COMMAND(moveToBeginningOfParagraph)
WEBCORE_COMMAND(moveToBeginningOfParagraphAndModifySelection)
WEBCORE_COMMAND(moveToBeginningOfSentence)
WEBCORE_COMMAND(moveToBeginningOfSentenceAndModifySelection)
WEBCORE_COMMAND(moveToEndOfDocument)
WEBCORE_COMMAND(moveToEndOfDocumentAndModifySelection)
WEBCORE_COMMAND(moveToEndOfLine)
WEBCORE_COMMAND(moveToEndOfLineAndModifySelection)
WEBCORE_COMMAND(moveToEndOfParagraph)
WEBCORE_COMMAND(moveToEndOfParagraphAndModifySelection)
WEBCORE_COMMAND(moveToEndOfSentence)
WEBCORE_COMMAND(moveToEndOfSentenceAndModifySelection)
WEBCORE_COMMAND(moveToLeftEndOfLine)
WEBCORE_COMMAND(moveToLeftEndOfLineAndModifySelection)
WEBCORE_COMMAND(moveToRightEndOfLine)
WEBCORE_COMMAND(moveToRightEndOfLineAndModifySelection)
WEBCORE_COMMAND(moveUp)
WEBCORE_COMMAND(moveUpAndModifySelection)
WEBCORE_COMMAND(moveWordBackward)
WEBCORE_COMMAND(moveWordBackwardAndModifySelection)
WEBCORE_COMMAND(moveWordForward)
WEBCORE_COMMAND(moveWordForwardAndModifySelection)
WEBCORE_COMMAND(moveWordLeft)
WEBCORE_COMMAND(moveWordLeftAndModifySelection)
WEBCORE_COMMAND(moveWordRight)
WEBCORE_COMMAND(moveWordRightAndModifySelection)
WEBCORE_COMMAND(outdent)
WEBCORE_COMMAND(pageDown)
WEBCORE_COMMAND(pageDownAndModifySelection)
WEBCORE_COMMAND(pageUp)
WEBCORE_COMMAND(pageUpAndModifySelection)
WEBCORE_COMMAND(paste)
WEBCORE_COMMAND(pasteAsPlainText)
WEBCORE_COMMAND(scrollPageDown)
WEBCORE_COMMAND(scrollPageUp)
WEBCORE_COMMAND(scrollToBeginningOfDocument)
WEBCORE_COMMAND(scrollToEndOfDocument)
WEBCORE_COMMAND(selectAll)
WEBCORE_COMMAND(selectLine)
WEBCORE_COMMAND(selectParagraph)
WEBCORE_COMMAND(selectSentence)
WEBCORE_COMMAND(selectToMark)
WEBCORE_COMMAND(selectWord)
WEBCORE_COMMAND(setMark)
WEBCORE_COMMAND(subscript)
WEBCORE_COMMAND(superscript)
WEBCORE_COMMAND(swapWithMark)
WEBCORE_COMMAND(takeFindStringFromSelection)
WEBCORE_COMMAND(transpose)
WEBCORE_COMMAND(underline)
WEBCORE_COMMAND(unscript)
WEBCORE_COMMAND(yank)
WEBCORE_COMMAND(yankAndSelect)

#undef WEBCORE_COMMAND

// This method is needed to support Mac OS X services.

- (BOOL)writeSelectionToPasteboard:(NSPasteboard *)pasteboard types:(NSArray *)types
{
    Vector<String> pasteboardTypes;
    size_t numTypes = [types count];
    for (size_t i = 0; i < numTypes; ++i)
        pasteboardTypes.append([types objectAtIndex:i]);
    return _data->_page->writeSelectionToPasteboard([pasteboard name], pasteboardTypes);
}

- (void)centerSelectionInVisibleArea:(id)sender 
{ 
    _data->_page->centerSelectionInVisibleArea();
}

// This method is needed to support Mac OS X services.

- (id)validRequestorForSendType:(NSString *)sendType returnType:(NSString *)returnType
{
    BOOL isValidSendType = !sendType || ([PasteboardTypes::forSelection() containsObject:sendType] && !_data->_page->editorState().selectionIsNone);
    BOOL isValidReturnType = NO;
    if (!returnType)
        isValidReturnType = YES;
    else if ([PasteboardTypes::forEditing() containsObject:returnType] && _data->_page->editorState().isContentEditable) {
        // We can insert strings in any editable context.  We can insert other types, like images, only in rich edit contexts.
        isValidReturnType = _data->_page->editorState().isContentRichlyEditable || [returnType isEqualToString:NSStringPboardType];
    }
    if (isValidSendType && isValidReturnType)
        return self;
    return [[self nextResponder] validRequestorForSendType:sendType returnType:returnType];
}

// This method is needed to support Mac OS X services.

- (BOOL)readSelectionFromPasteboard:(NSPasteboard *)pasteboard 
{
    return _data->_page->readSelectionFromPasteboard([pasteboard name]);
}

/*

When possible, editing-related methods should be implemented in WebCore with the
EditorCommand mechanism and invoked via WEBCORE_COMMAND, rather than implementing
individual methods here with Mac-specific code.

Editing-related methods still unimplemented that are implemented in WebKit1:

- (void)capitalizeWord:(id)sender;
- (void)changeFont:(id)sender;
- (void)complete:(id)sender;
- (void)copyFont:(id)sender;
- (void)lowercaseWord:(id)sender;
- (void)makeBaseWritingDirectionLeftToRight:(id)sender;
- (void)makeBaseWritingDirectionNatural:(id)sender;
- (void)makeBaseWritingDirectionRightToLeft:(id)sender;
- (void)pasteFont:(id)sender;
- (void)scrollLineDown:(id)sender;
- (void)scrollLineUp:(id)sender;
- (void)showGuessPanel:(id)sender;
- (void)uppercaseWord:(id)sender;

Some other editing-related methods still unimplemented:

- (void)changeCaseOfLetter:(id)sender;
- (void)copyRuler:(id)sender;
- (void)insertContainerBreak:(id)sender;
- (void)insertDoubleQuoteIgnoringSubstitution:(id)sender;
- (void)insertSingleQuoteIgnoringSubstitution:(id)sender;
- (void)pasteRuler:(id)sender;
- (void)toggleRuler:(id)sender;
- (void)transposeWords:(id)sender;

*/

// Menu items validation

static NSMenuItem *menuItem(id <NSValidatedUserInterfaceItem> item)
{
    if (![(NSObject *)item isKindOfClass:[NSMenuItem class]])
        return nil;
    return (NSMenuItem *)item;
}

static NSToolbarItem *toolbarItem(id <NSValidatedUserInterfaceItem> item)
{
    if (![(NSObject *)item isKindOfClass:[NSToolbarItem class]])
        return nil;
    return (NSToolbarItem *)item;
}

static void validateCommandCallback(WKStringRef commandName, bool isEnabled, int32_t state, WKErrorRef error, void* context)
{
    // If the process exits before the command can be validated, we'll be called back with an error.
    if (error)
        return;
    
    WKView* wkView = static_cast<WKView*>(context);
    ASSERT(wkView);
    
    [wkView _setUserInterfaceItemState:nsStringFromWebCoreString(toImpl(commandName)->string()) enabled:isEnabled state:state];
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    SEL action = [item action];

    if (action == @selector(showGuessPanel:)) {
        if (NSMenuItem *menuItem = ::menuItem(item))
            [menuItem setTitle:contextMenuItemTagShowSpellingPanel(![[[NSSpellChecker sharedSpellChecker] spellingPanel] isVisible])];
        return _data->_page->editorState().isContentEditable;
    }

    if (action == @selector(checkSpelling:) || action == @selector(changeSpelling:))
        return _data->_page->editorState().isContentEditable;

    if (action == @selector(toggleContinuousSpellChecking:)) {
        bool enabled = TextChecker::isContinuousSpellCheckingAllowed();
        bool checked = enabled && TextChecker::state().isContinuousSpellCheckingEnabled;
        [menuItem(item) setState:checked ? NSOnState : NSOffState];
        return enabled;
    }

    if (action == @selector(toggleGrammarChecking:)) {
        bool checked = TextChecker::state().isGrammarCheckingEnabled;
        [menuItem(item) setState:checked ? NSOnState : NSOffState];
        return YES;
    }

    if (action == @selector(toggleAutomaticSpellingCorrection:)) {
        bool checked = TextChecker::state().isAutomaticSpellingCorrectionEnabled;
        [menuItem(item) setState:checked ? NSOnState : NSOffState];
        return _data->_page->editorState().isContentEditable;
    }

    if (action == @selector(orderFrontSubstitutionsPanel:)) {
        if (NSMenuItem *menuItem = ::menuItem(item))
            [menuItem setTitle:contextMenuItemTagShowSubstitutions(![[[NSSpellChecker sharedSpellChecker] substitutionsPanel] isVisible])];
        return _data->_page->editorState().isContentEditable;
    }

    if (action == @selector(toggleSmartInsertDelete:)) {
        bool checked = _data->_page->isSmartInsertDeleteEnabled();
        [menuItem(item) setState:checked ? NSOnState : NSOffState];
        return _data->_page->editorState().isContentEditable;
    }

    if (action == @selector(toggleAutomaticQuoteSubstitution:)) {
        bool checked = TextChecker::state().isAutomaticQuoteSubstitutionEnabled;
        [menuItem(item) setState:checked ? NSOnState : NSOffState];
        return _data->_page->editorState().isContentEditable;
    }

    if (action == @selector(toggleAutomaticDashSubstitution:)) {
        bool checked = TextChecker::state().isAutomaticDashSubstitutionEnabled;
        [menuItem(item) setState:checked ? NSOnState : NSOffState];
        return _data->_page->editorState().isContentEditable;
    }

    if (action == @selector(toggleAutomaticLinkDetection:)) {
        bool checked = TextChecker::state().isAutomaticLinkDetectionEnabled;
        [menuItem(item) setState:checked ? NSOnState : NSOffState];
        return _data->_page->editorState().isContentEditable;
    }

    if (action == @selector(toggleAutomaticTextReplacement:)) {
        bool checked = TextChecker::state().isAutomaticTextReplacementEnabled;
        [menuItem(item) setState:checked ? NSOnState : NSOffState];
        return _data->_page->editorState().isContentEditable;
    }

    if (action == @selector(uppercaseWord:) || action == @selector(lowercaseWord:) || action == @selector(capitalizeWord:))
        return _data->_page->editorState().selectionIsRange && _data->_page->editorState().isContentEditable;
    
    if (action == @selector(stopSpeaking:))
        return [NSApp isSpeaking];
    
    // The centerSelectionInVisibleArea: selector is enabled if there's a selection range or if there's an insertion point in an editable area.
    if (action == @selector(centerSelectionInVisibleArea:))
        return _data->_page->editorState().selectionIsRange || (_data->_page->editorState().isContentEditable && !_data->_page->editorState().selectionIsNone);

    // Next, handle editor commands. Start by returning YES for anything that is not an editor command.
    // Returning YES is the default thing to do in an AppKit validate method for any selector that is not recognized.
    String commandName = commandNameForSelector([item action]);
    if (!Editor::commandIsSupportedFromMenuOrKeyBinding(commandName))
        return YES;

    // Add this item to the vector of items for a given command that are awaiting validation.
    pair<ValidationMap::iterator, bool> addResult = _data->_validationMap.add(commandName, ValidationVector());
    addResult.first->second.append(item);
    if (addResult.second) {
        // If we are not already awaiting validation for this command, start the asynchronous validation process.
        // FIXME: Theoretically, there is a race here; when we get the answer it might be old, from a previous time
        // we asked for the same command; there is no guarantee the answer is still valid.
        _data->_page->validateCommand(commandName, ValidateCommandCallback::create(self, validateCommandCallback));
    }

    // Treat as enabled until we get the result back from the web process and _setUserInterfaceItemState is called.
    // FIXME <rdar://problem/8803459>: This means disabled items will flash enabled at first for a moment.
    // But returning NO here would be worse; that would make keyboard commands such as command-C fail.
    return YES;
}

static void speakString(WKStringRef string, WKErrorRef error, void*)
{
    if (error)
        return;
    if (!string)
        return;

    NSString *convertedString = toImpl(string)->string();
    [NSApp speakString:convertedString];
}

- (IBAction)startSpeaking:(id)sender
{
    _data->_page->getSelectionOrContentsAsString(StringCallback::create(0, speakString));
}

- (IBAction)stopSpeaking:(id)sender
{
    [NSApp stopSpeaking:sender];
}

- (IBAction)showGuessPanel:(id)sender
{
    NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
    if (!checker) {
        LOG_ERROR("No NSSpellChecker");
        return;
    }
    
    NSPanel *spellingPanel = [checker spellingPanel];
    if ([spellingPanel isVisible]) {
        [spellingPanel orderOut:sender];
        return;
    }
    
    _data->_page->advanceToNextMisspelling(true);
    [spellingPanel orderFront:sender];
}

- (IBAction)checkSpelling:(id)sender
{
    _data->_page->advanceToNextMisspelling(false);
}

- (void)changeSpelling:(id)sender
{
    NSString *word = [[sender selectedCell] stringValue];

    _data->_page->changeSpellingToWord(word);
}

- (IBAction)toggleContinuousSpellChecking:(id)sender
{
    bool spellCheckingEnabled = !TextChecker::state().isContinuousSpellCheckingEnabled;
    TextChecker::setContinuousSpellCheckingEnabled(spellCheckingEnabled);

    _data->_page->process()->updateTextCheckerState();
}

- (BOOL)isGrammarCheckingEnabled
{
    return TextChecker::state().isGrammarCheckingEnabled;
}

- (void)setGrammarCheckingEnabled:(BOOL)flag
{
    if (static_cast<bool>(flag) == TextChecker::state().isGrammarCheckingEnabled)
        return;
    
    TextChecker::setGrammarCheckingEnabled(flag);
    _data->_page->process()->updateTextCheckerState();
}

- (IBAction)toggleGrammarChecking:(id)sender
{
    bool grammarCheckingEnabled = !TextChecker::state().isGrammarCheckingEnabled;
    TextChecker::setGrammarCheckingEnabled(grammarCheckingEnabled);

    _data->_page->process()->updateTextCheckerState();
}

- (IBAction)toggleAutomaticSpellingCorrection:(id)sender
{
    TextChecker::setAutomaticSpellingCorrectionEnabled(!TextChecker::state().isAutomaticSpellingCorrectionEnabled);

    _data->_page->process()->updateTextCheckerState();
}

- (void)orderFrontSubstitutionsPanel:(id)sender
{
    NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
    if (!checker) {
        LOG_ERROR("No NSSpellChecker");
        return;
    }
    
    NSPanel *substitutionsPanel = [checker substitutionsPanel];
    if ([substitutionsPanel isVisible]) {
        [substitutionsPanel orderOut:sender];
        return;
    }
    [substitutionsPanel orderFront:sender];
}

- (IBAction)toggleSmartInsertDelete:(id)sender
{
    _data->_page->setSmartInsertDeleteEnabled(!_data->_page->isSmartInsertDeleteEnabled());
}

- (BOOL)isAutomaticQuoteSubstitutionEnabled
{
    return TextChecker::state().isAutomaticQuoteSubstitutionEnabled;
}

- (void)setAutomaticQuoteSubstitutionEnabled:(BOOL)flag
{
    if (static_cast<bool>(flag) == TextChecker::state().isAutomaticQuoteSubstitutionEnabled)
        return;

    TextChecker::setAutomaticQuoteSubstitutionEnabled(flag);
    _data->_page->process()->updateTextCheckerState();
}

- (void)toggleAutomaticQuoteSubstitution:(id)sender
{
    TextChecker::setAutomaticQuoteSubstitutionEnabled(!TextChecker::state().isAutomaticQuoteSubstitutionEnabled);
    _data->_page->process()->updateTextCheckerState();
}

- (BOOL)isAutomaticDashSubstitutionEnabled
{
    return TextChecker::state().isAutomaticDashSubstitutionEnabled;
}

- (void)setAutomaticDashSubstitutionEnabled:(BOOL)flag
{
    if (static_cast<bool>(flag) == TextChecker::state().isAutomaticDashSubstitutionEnabled)
        return;

    TextChecker::setAutomaticDashSubstitutionEnabled(flag);
    _data->_page->process()->updateTextCheckerState();
}

- (void)toggleAutomaticDashSubstitution:(id)sender
{
    TextChecker::setAutomaticDashSubstitutionEnabled(!TextChecker::state().isAutomaticDashSubstitutionEnabled);
    _data->_page->process()->updateTextCheckerState();
}

- (BOOL)isAutomaticLinkDetectionEnabled
{
    return TextChecker::state().isAutomaticLinkDetectionEnabled;
}

- (void)setAutomaticLinkDetectionEnabled:(BOOL)flag
{
    if (static_cast<bool>(flag) == TextChecker::state().isAutomaticLinkDetectionEnabled)
        return;

    TextChecker::setAutomaticLinkDetectionEnabled(flag);
    _data->_page->process()->updateTextCheckerState();
}

- (void)toggleAutomaticLinkDetection:(id)sender
{
    TextChecker::setAutomaticLinkDetectionEnabled(!TextChecker::state().isAutomaticLinkDetectionEnabled);
    _data->_page->process()->updateTextCheckerState();
}

- (BOOL)isAutomaticTextReplacementEnabled
{
    return TextChecker::state().isAutomaticTextReplacementEnabled;
}

- (void)setAutomaticTextReplacementEnabled:(BOOL)flag
{
    if (static_cast<bool>(flag) == TextChecker::state().isAutomaticTextReplacementEnabled)
        return;

    TextChecker::setAutomaticTextReplacementEnabled(flag);
    _data->_page->process()->updateTextCheckerState();
}

- (void)toggleAutomaticTextReplacement:(id)sender
{
    TextChecker::setAutomaticTextReplacementEnabled(!TextChecker::state().isAutomaticTextReplacementEnabled);
    _data->_page->process()->updateTextCheckerState();
}

- (void)uppercaseWord:(id)sender
{
    _data->_page->uppercaseWord();
}

- (void)lowercaseWord:(id)sender
{
    _data->_page->lowercaseWord();
}

- (void)capitalizeWord:(id)sender
{
    _data->_page->capitalizeWord();
}

- (void)displayIfNeeded
{
#if !defined(BUILDING_ON_SNOW_LEOPARD)
    // FIXME: We should remove this code when <rdar://problem/9362085> is resolved. In the meantime,
    // it is necessary to disable scren updates so we get a chance to redraw the corners before this 
    // display is visible.
    NSWindow *window = [self window];
    BOOL shouldMaskWindow = window && !NSIsEmptyRect(_data->_windowBottomCornerIntersectionRect);
    if (shouldMaskWindow)
        NSDisableScreenUpdates();
#endif

    [super displayIfNeeded];

#if !defined(BUILDING_ON_SNOW_LEOPARD)
    if (shouldMaskWindow) {
        [window _maskRoundedBottomCorners:_data->_windowBottomCornerIntersectionRect];
        NSEnableScreenUpdates();
        _data->_windowBottomCornerIntersectionRect = NSZeroRect;
    }
#endif
}

// Events

// Override this so that AppKit will send us arrow keys as key down events so we can
// support them via the key bindings mechanism.
- (BOOL)_wantsKeyDownForEvent:(NSEvent *)event
{
    return YES;
}

- (void)_setMouseDownEvent:(NSEvent *)event
{
    ASSERT(!event || [event type] == NSLeftMouseDown || [event type] == NSRightMouseDown || [event type] == NSOtherMouseDown);
    
    if (event == _data->_mouseDownEvent)
        return;
    
    [_data->_mouseDownEvent release];
    _data->_mouseDownEvent = [event retain];
}

#define NATIVE_MOUSE_EVENT_HANDLER(Selector) \
    - (void)Selector:(NSEvent *)theEvent \
    { \
        if ([[self inputContext] handleEvent:theEvent]) { \
            LOG(TextInput, "%s was handled by text input context", String(#Selector).substring(0, String(#Selector).find("Internal")).ascii().data()); \
            return; \
        } \
        NativeWebMouseEvent webEvent(theEvent, self); \
        _data->_page->handleMouseEvent(webEvent); \
    }

NATIVE_MOUSE_EVENT_HANDLER(mouseEntered)
NATIVE_MOUSE_EVENT_HANDLER(mouseExited)
NATIVE_MOUSE_EVENT_HANDLER(mouseMovedInternal)
NATIVE_MOUSE_EVENT_HANDLER(mouseDownInternal)
NATIVE_MOUSE_EVENT_HANDLER(mouseUpInternal)
NATIVE_MOUSE_EVENT_HANDLER(mouseDraggedInternal)
NATIVE_MOUSE_EVENT_HANDLER(otherMouseDown)
NATIVE_MOUSE_EVENT_HANDLER(otherMouseDragged)
NATIVE_MOUSE_EVENT_HANDLER(otherMouseMoved)
NATIVE_MOUSE_EVENT_HANDLER(otherMouseUp)
NATIVE_MOUSE_EVENT_HANDLER(rightMouseDown)
NATIVE_MOUSE_EVENT_HANDLER(rightMouseDragged)
NATIVE_MOUSE_EVENT_HANDLER(rightMouseUp)

#undef NATIVE_MOUSE_EVENT_HANDLER

#define NATIVE_EVENT_HANDLER(Selector, Type) \
    - (void)Selector:(NSEvent *)theEvent \
    { \
        NativeWeb##Type##Event webEvent = NativeWeb##Type##Event(theEvent, self); \
        _data->_page->handle##Type##Event(webEvent); \
    }

NATIVE_EVENT_HANDLER(scrollWheel, Wheel)

#undef NATIVE_EVENT_HANDLER

- (void)mouseMoved:(NSEvent *)event
{
    // When a view is first responder, it gets mouse moved events even when the mouse is outside its visible rect.
    if (self == [[self window] firstResponder] && !NSPointInRect([self convertPoint:[event locationInWindow] fromView:nil], [self visibleRect]))
        return;

    [self mouseMovedInternal:event];
}

- (void)mouseDown:(NSEvent *)event
{
    [self _setMouseDownEvent:event];
    _data->_ignoringMouseDraggedEvents = NO;
    _data->_dragHasStarted = NO;
    [self mouseDownInternal:event];
}

- (void)mouseUp:(NSEvent *)event
{
    [self _setMouseDownEvent:nil];
    [self mouseUpInternal:event];
}

- (void)mouseDragged:(NSEvent *)event
{
    if (_data->_ignoringMouseDraggedEvents)
        return;
    [self mouseDraggedInternal:event];
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event
{
    // There's a chance that responding to this event will run a nested event loop, and
    // fetching a new event might release the old one. Retaining and then autoreleasing
    // the current event prevents that from causing a problem inside WebKit or AppKit code.
    [[event retain] autorelease];
    
    if (![self hitTest:[event locationInWindow]])
        return NO;
    
    [self _setMouseDownEvent:event];
    bool result = _data->_page->acceptsFirstMouse([event eventNumber], WebEventFactory::createWebMouseEvent(event, self));
    [self _setMouseDownEvent:nil];
    return result;
}

- (BOOL)shouldDelayWindowOrderingForEvent:(NSEvent *)event
{
    // If this is the active window or we don't have a range selection, there is no need to perform additional checks
    // and we can avoid making a synchronous call to the WebProcess.
    if ([[self window] isKeyWindow] || _data->_page->editorState().selectionIsNone || !_data->_page->editorState().selectionIsRange)
        return NO;

    // There's a chance that responding to this event will run a nested event loop, and
    // fetching a new event might release the old one. Retaining and then autoreleasing
    // the current event prevents that from causing a problem inside WebKit or AppKit code.
    [[event retain] autorelease];
    
    if (![self hitTest:[event locationInWindow]])
        return NO;
    
    [self _setMouseDownEvent:event];
    bool result = _data->_page->shouldDelayWindowOrderingForEvent(WebEventFactory::createWebMouseEvent(event, self));
    [self _setMouseDownEvent:nil];
    return result;
}

#if ENABLE(GESTURE_EVENTS)

static const short kIOHIDEventTypeScroll = 6;

- (void)shortCircuitedEndGestureWithEvent:(NSEvent *)event
{
    if ([event subtype] != kIOHIDEventTypeScroll)
        return;

    WebGestureEvent webEvent = WebEventFactory::createWebGestureEvent(event, self);
    _data->_page->handleGestureEvent(webEvent);

    if (_data->_endGestureMonitor) {
        [NSEvent removeMonitor:_data->_endGestureMonitor];
        _data->_endGestureMonitor = nil;
    }
}

- (void)beginGestureWithEvent:(NSEvent *)event
{
    if ([event subtype] != kIOHIDEventTypeScroll)
        return;

    WebGestureEvent webEvent = WebEventFactory::createWebGestureEvent(event, self);
    _data->_page->handleGestureEvent(webEvent);

    if (!_data->_endGestureMonitor) {
        _data->_endGestureMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskEndGesture handler:^(NSEvent *blockEvent) {
            [self shortCircuitedEndGestureWithEvent:blockEvent];
            return blockEvent;
        }];
    }
}
#endif

- (void)doCommandBySelector:(SEL)selector
{
    LOG(TextInput, "doCommandBySelector:\"%s\"", sel_getName(selector));

    WKViewInterpretKeyEventsParameters* parameters = _data->_interpretKeyEventsParameters;
    if (parameters)
        parameters->consumedByIM = false;

    // As in insertText:replacementRange:, we assume that the call comes from an input method if there is marked text.
    bool isFromInputMethod = _data->_page->editorState().hasComposition;

    if (parameters && !isFromInputMethod)
        parameters->commands->append(KeypressCommand(NSStringFromSelector(selector)));
    else {
        // FIXME: Send the command to Editor synchronously and only send it along the
        // responder chain if it's a selector that does not correspond to an editing command.
        [super doCommandBySelector:selector];
    }
}

- (void)insertText:(id)string
{
    // Unlike and NSTextInputClient variant with replacementRange, this NSResponder method is called when there is no input context,
    // so text input processing isn't performed. We are not going to actually insert any text in that case, but saving an insertText
    // command ensures that a keypress event is dispatched as appropriate.
    [self insertText:string replacementRange:NSMakeRange(NSNotFound, 0)];
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange
{
    BOOL isAttributedString = [string isKindOfClass:[NSAttributedString class]];
    ASSERT(isAttributedString || [string isKindOfClass:[NSString class]]);

    if (replacementRange.location != NSNotFound)
        LOG(TextInput, "insertText:\"%@\" replacementRange:(%u, %u)", isAttributedString ? [string string] : string, replacementRange.location, replacementRange.length);
    else
        LOG(TextInput, "insertText:\"%@\"", isAttributedString ? [string string] : string);
    WKViewInterpretKeyEventsParameters* parameters = _data->_interpretKeyEventsParameters;
    if (parameters)
        parameters->consumedByIM = false;

    NSString *text;
    bool isFromInputMethod = _data->_page->editorState().hasComposition;

    if (isAttributedString) {
        // FIXME: We ignore most attributes from the string, so for example inserting from Character Palette loses font and glyph variation data.
        text = [string string];
    } else
        text = string;

    // insertText can be called for several reasons:
    // - If it's from normal key event processing (including key bindings), we may need to save the action to perform it later.
    // - If it's from an input method, then we should go ahead and insert the text now. We assume it's from the input method if we have marked text.
    // FIXME: In theory, this could be wrong for some input methods, so we should try to find another way to determine if the call is from the input method.
    // - If it's sent outside of keyboard event processing (e.g. from Character Viewer, or when confirming an inline input area with a mouse),
    // then we also execute it immediately, as there will be no other chance.
    if (parameters && !isFromInputMethod) {
        ASSERT(replacementRange.location == NSNotFound);
        parameters->commands->append(KeypressCommand("insertText:", text));
        return;
    }

    String eventText = text;
    eventText.replace(NSBackTabCharacter, NSTabCharacter); // same thing is done in KeyEventMac.mm in WebCore
    bool eventHandled = _data->_page->insertText(eventText, replacementRange.location, NSMaxRange(replacementRange));

    if (parameters)
        parameters->eventInterpretationHadSideEffects |= eventHandled;
}

- (BOOL)_handleStyleKeyEquivalent:(NSEvent *)event
{
    if (!_data->_page->editorState().isContentEditable)
        return NO;

    if (([event modifierFlags] & NSDeviceIndependentModifierFlagsMask) != NSCommandKeyMask)
        return NO;
    
    // Here we special case cmd+b and cmd+i but not cmd+u, for historic reason.
    // This should not be changed, since it could break some Mac applications that
    // rely on this inherent behavior.
    // See https://bugs.webkit.org/show_bug.cgi?id=24943
    
    NSString *string = [event characters];
    if ([string caseInsensitiveCompare:@"b"] == NSOrderedSame) {
        _data->_page->executeEditCommand("ToggleBold");
        return YES;
    }
    if ([string caseInsensitiveCompare:@"i"] == NSOrderedSame) {
        _data->_page->executeEditCommand("ToggleItalic");
        return YES;
    }
    
    return NO;
}

- (BOOL)performKeyEquivalent:(NSEvent *)event
{
    // There's a chance that responding to this event will run a nested event loop, and
    // fetching a new event might release the old one. Retaining and then autoreleasing
    // the current event prevents that from causing a problem inside WebKit or AppKit code.
    [[event retain] autorelease];
    
    BOOL eventWasSentToWebCore = (_data->_keyDownEventBeingResent == event);

    // Pass key combos through WebCore if there is a key binding available for
    // this event. This lets web pages have a crack at intercepting key-modified keypresses.
    // But don't do it if we have already handled the event.
    // Pressing Esc results in a fake event being sent - don't pass it to WebCore.
    if (!eventWasSentToWebCore && event == [NSApp currentEvent] && self == [[self window] firstResponder]) {
        _data->_page->handleKeyboardEvent(NativeWebKeyboardEvent(event, self));
        return YES;
    }
    
    return [self _handleStyleKeyEquivalent:event] || [super performKeyEquivalent:event];
}

- (void)keyUp:(NSEvent *)theEvent
{
    _data->_page->handleKeyboardEvent(NativeWebKeyboardEvent(theEvent, self));
}

- (void)keyDown:(NSEvent *)theEvent
{
    // There's a chance that responding to this event will run a nested event loop, and
    // fetching a new event might release the old one. Retaining and then autoreleasing
    // the current event prevents that from causing a problem inside WebKit or AppKit code.
    [[theEvent retain] autorelease];

    if (_data->_pluginComplexTextInputIdentifier) {
        // Try feeding the keyboard event directly to the plug-in.
        NSString *string = nil;
        if ([[WKTextInputWindowController sharedTextInputWindowController] interpretKeyEvent:theEvent string:&string]) {
            if (string)
                _data->_page->sendComplexTextInputToPlugin(_data->_pluginComplexTextInputIdentifier, string);
            return;
        }
    }

    // We could be receiving a key down from AppKit if we have re-sent an event
    // that maps to an action that is currently unavailable (for example a copy when
    // there is no range selection).
    // If this is the case we should ignore the key down.
    if (_data->_keyDownEventBeingResent == theEvent) {
        [super keyDown:theEvent];
        return;
    }
    _data->_page->handleKeyboardEvent(NativeWebKeyboardEvent(theEvent, self));
}

- (void)flagsChanged:(NSEvent *)theEvent
{
    // There's a chance that responding to this event will run a nested event loop, and
    // fetching a new event might release the old one. Retaining and then autoreleasing
    // the current event prevents that from causing a problem inside WebKit or AppKit code.
    [[theEvent retain] autorelease];

    unsigned short keyCode = [theEvent keyCode];

    // Don't make an event from the num lock and function keys
    if (!keyCode || keyCode == 10 || keyCode == 63)
        return;

    _data->_page->handleKeyboardEvent(NativeWebKeyboardEvent(theEvent, self));
}

- (void)_executeSavedKeypressCommands
{
    WKViewInterpretKeyEventsParameters* parameters = _data->_interpretKeyEventsParameters;
    if (!parameters || parameters->commands->isEmpty())
        return;

    // We could be called again if the execution of one command triggers a call to selectedRange.
    // In this case, the state is up to date, and we don't need to execute any more saved commands to return a result.
    if (parameters->executingSavedKeypressCommands)
        return;

    parameters->executingSavedKeypressCommands = true;
    parameters->eventInterpretationHadSideEffects |= _data->_page->executeKeypressCommands(*parameters->commands);
    parameters->commands->clear();
    parameters->executingSavedKeypressCommands = false;
}

- (void)_notifyInputContextAboutDiscardedComposition
{
    // <rdar://problem/9359055>: -discardMarkedText can only be called for active contexts.
    // FIXME: We fail to ever notify the input context if something (e.g. a navigation) happens while the window is not key.
    // This is not a problem when the window is key, because we discard marked text on resigning first responder.
    if (![[self window] isKeyWindow] || self != [[self window] firstResponder])
        return;

    [[super inputContext] discardMarkedText]; // Inform the input method that we won't have an inline input area despite having been asked to.
}

- (NSTextInputContext *)inputContext
{
    WKViewInterpretKeyEventsParameters* parameters = _data->_interpretKeyEventsParameters;

    if (_data->_pluginComplexTextInputIdentifier && !parameters)
        return [[WKTextInputWindowController sharedTextInputWindowController] inputContext];

    // Disable text input machinery when in non-editable content. An invisible inline input area affects performance, and can prevent Expose from working.
    if (!_data->_page->editorState().isContentEditable)
        return nil;

    return [super inputContext];
}

- (NSRange)selectedRange
{
    [self _executeSavedKeypressCommands];

    uint64_t selectionStart;
    uint64_t selectionLength;
    _data->_page->getSelectedRange(selectionStart, selectionLength);

    NSRange result = NSMakeRange(selectionStart, selectionLength);
    if (result.location == NSNotFound)
        LOG(TextInput, "selectedRange -> (NSNotFound, %u)", result.length);
    else
        LOG(TextInput, "selectedRange -> (%u, %u)", result.location, result.length);

    return result;
}

- (BOOL)hasMarkedText
{
    WKViewInterpretKeyEventsParameters* parameters = _data->_interpretKeyEventsParameters;

    BOOL result;
    if (parameters) {
        result = _data->_page->editorState().hasComposition;
        if (result) {
            // A saved command can confirm a composition, but it cannot start a new one.
            [self _executeSavedKeypressCommands];
            result = _data->_page->editorState().hasComposition;
        }
    } else {
        uint64_t location;
        uint64_t length;
        _data->_page->getMarkedRange(location, length);
        result = location != NSNotFound;
    }

    LOG(TextInput, "hasMarkedText -> %u", result);
    return result;
}

- (void)unmarkText
{
    [self _executeSavedKeypressCommands];

    LOG(TextInput, "unmarkText");

    // Use pointer to get parameters passed to us by the caller of interpretKeyEvents.
    WKViewInterpretKeyEventsParameters* parameters = _data->_interpretKeyEventsParameters;

    if (parameters) {
        parameters->eventInterpretationHadSideEffects = true;
        parameters->consumedByIM = false;
    }

    _data->_page->confirmComposition();
}

- (NSArray *)validAttributesForMarkedText
{
    static NSArray *validAttributes;
    if (!validAttributes) {
        validAttributes = [[NSArray alloc] initWithObjects:
                           NSUnderlineStyleAttributeName, NSUnderlineColorAttributeName,
                           NSMarkedClauseSegmentAttributeName, nil];
        // NSText also supports the following attributes, but it's
        // hard to tell which are really required for text input to
        // work well; I have not seen any input method make use of them yet.
        //     NSFontAttributeName, NSForegroundColorAttributeName,
        //     NSBackgroundColorAttributeName, NSLanguageAttributeName.
        CFRetain(validAttributes);
    }
    LOG(TextInput, "validAttributesForMarkedText -> (...)");
    return validAttributes;
}

static void extractUnderlines(NSAttributedString *string, Vector<CompositionUnderline>& result)
{
    int length = [[string string] length];
    
    int i = 0;
    while (i < length) {
        NSRange range;
        NSDictionary *attrs = [string attributesAtIndex:i longestEffectiveRange:&range inRange:NSMakeRange(i, length - i)];
        
        if (NSNumber *style = [attrs objectForKey:NSUnderlineStyleAttributeName]) {
            Color color = Color::black;
            if (NSColor *colorAttr = [attrs objectForKey:NSUnderlineColorAttributeName])
                color = colorFromNSColor([colorAttr colorUsingColorSpaceName:NSDeviceRGBColorSpace]);
            result.append(CompositionUnderline(range.location, NSMaxRange(range), color, [style intValue] > 1));
        }
        
        i = range.location + range.length;
    }
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)newSelRange replacementRange:(NSRange)replacementRange
{
    [self _executeSavedKeypressCommands];

    BOOL isAttributedString = [string isKindOfClass:[NSAttributedString class]];
    ASSERT(isAttributedString || [string isKindOfClass:[NSString class]]);

    LOG(TextInput, "setMarkedText:\"%@\" selectedRange:(%u, %u)", isAttributedString ? [string string] : string, newSelRange.location, newSelRange.length);

    // Use pointer to get parameters passed to us by the caller of interpretKeyEvents.
    WKViewInterpretKeyEventsParameters* parameters = _data->_interpretKeyEventsParameters;

    if (parameters) {
        parameters->eventInterpretationHadSideEffects = true;
        parameters->consumedByIM = false;
    }
    
    Vector<CompositionUnderline> underlines;
    NSString *text;

    if (isAttributedString) {
        // FIXME: We ignore most attributes from the string, so an input method cannot specify e.g. a font or a glyph variation.
        text = [string string];
        extractUnderlines(string, underlines);
    } else
        text = string;

    if (_data->_page->editorState().isInPasswordField) {
        // In password fields, we only allow ASCII dead keys, and don't allow inline input, matching NSSecureTextInputField.
        // Allowing ASCII dead keys is necessary to enable full Roman input when using a Vietnamese keyboard.
        ASSERT(!_data->_page->editorState().hasComposition);
        [self _notifyInputContextAboutDiscardedComposition];
        if ([text length] == 1 && [[text decomposedStringWithCanonicalMapping] characterAtIndex:0] < 0x80) {
            _data->_page->insertText(text, replacementRange.location, NSMaxRange(replacementRange));
        } else
            NSBeep();
        return;
    }

    _data->_page->setComposition(text, underlines, newSelRange.location, NSMaxRange(newSelRange), replacementRange.location, NSMaxRange(replacementRange));
}

- (NSRange)markedRange
{
    [self _executeSavedKeypressCommands];

    uint64_t location;
    uint64_t length;
    _data->_page->getMarkedRange(location, length);

    LOG(TextInput, "markedRange -> (%u, %u)", location, length);
    return NSMakeRange(location, length);
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)nsRange actualRange:(NSRangePointer)actualRange
{
    [self _executeSavedKeypressCommands];

    if (!_data->_page->editorState().isContentEditable) {
        LOG(TextInput, "attributedSubstringFromRange:(%u, %u) -> nil", nsRange.location, nsRange.length);
        return nil;
    }

    if (_data->_page->editorState().isInPasswordField)
        return nil;

    AttributedString result;
    _data->_page->getAttributedSubstringFromRange(nsRange.location, NSMaxRange(nsRange), result);

    if (actualRange)
        *actualRange = nsRange;

    LOG(TextInput, "attributedSubstringFromRange:(%u, %u) -> \"%@\"", nsRange.location, nsRange.length, [result.string.get() string]);
    return [[result.string.get() retain] autorelease];
}

- (NSUInteger)characterIndexForPoint:(NSPoint)thePoint
{
    [self _executeSavedKeypressCommands];

    NSWindow *window = [self window];
    
    if (window)
        thePoint = [window convertScreenToBase:thePoint];
    thePoint = [self convertPoint:thePoint fromView:nil];  // the point is relative to the main frame 
    
    uint64_t result = _data->_page->characterIndexForPoint(IntPoint(thePoint));
    LOG(TextInput, "characterIndexForPoint:(%f, %f) -> %u", thePoint.x, thePoint.y, result);
    return result;
}

- (NSRect)firstRectForCharacterRange:(NSRange)theRange actualRange:(NSRangePointer)actualRange
{ 
    [self _executeSavedKeypressCommands];

    // Just to match NSTextView's behavior. Regression tests cannot detect this;
    // to reproduce, use a test application from http://bugs.webkit.org/show_bug.cgi?id=4682
    // (type something; try ranges (1, -1) and (2, -1).
    if ((theRange.location + theRange.length < theRange.location) && (theRange.location + theRange.length != 0))
        theRange.length = 0;
    
    NSRect resultRect = _data->_page->firstRectForCharacterRange(theRange.location, theRange.length);
    resultRect = [self convertRect:resultRect toView:nil];
    
    NSWindow *window = [self window];
    if (window)
        resultRect.origin = [window convertBaseToScreen:resultRect.origin];

    if (actualRange)
        *actualRange = theRange;

    LOG(TextInput, "firstRectForCharacterRange:(%u, %u) -> (%f, %f, %f, %f)", theRange.location, theRange.length, resultRect.origin.x, resultRect.origin.y, resultRect.size.width, resultRect.size.height);
    return resultRect;
}

- (void)draggedImage:(NSImage *)anImage endedAt:(NSPoint)aPoint operation:(NSDragOperation)operation
{
    NSPoint windowImageLoc = [[self window] convertScreenToBase:aPoint];
    NSPoint windowMouseLoc = windowImageLoc;
   
    // Prevent queued mouseDragged events from coming after the drag and fake mouseUp event.
    _data->_ignoringMouseDraggedEvents = YES;
    
    _data->_page->dragEnded(IntPoint(windowMouseLoc), globalPoint(windowMouseLoc, [self window]), operation);
}

- (DragApplicationFlags)applicationFlags:(id <NSDraggingInfo>)draggingInfo
{
    uint32_t flags = 0;
    if ([NSApp modalWindow])
        flags = DragApplicationIsModal;
    if ([[self window] attachedSheet])
        flags |= DragApplicationHasAttachedSheet;
    if ([draggingInfo draggingSource] == self)
        flags |= DragApplicationIsSource;
    if ([[NSApp currentEvent] modifierFlags] & NSAlternateKeyMask)
        flags |= DragApplicationIsCopyKeyDown;
    return static_cast<DragApplicationFlags>(flags);
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)draggingInfo
{
    IntPoint client([self convertPoint:[draggingInfo draggingLocation] fromView:nil]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);

    _data->_page->resetDragOperation();
    _data->_page->dragEntered(&dragData, [[draggingInfo draggingPasteboard] name]);
    return NSDragOperationCopy;
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)draggingInfo
{
    IntPoint client([self convertPoint:[draggingInfo draggingLocation] fromView:nil]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);
    _data->_page->dragUpdated(&dragData, [[draggingInfo draggingPasteboard] name]);
    return _data->_page->dragOperation();
}

- (void)draggingExited:(id <NSDraggingInfo>)draggingInfo
{
    IntPoint client([self convertPoint:[draggingInfo draggingLocation] fromView:nil]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);
    _data->_page->dragExited(&dragData, [[draggingInfo draggingPasteboard] name]);
    _data->_page->resetDragOperation();
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)draggingInfo
{
    return YES;
}

// FIXME: This code is more or less copied from Pasteboard::getBestURL.
// It would be nice to be able to share the code somehow.
static void maybeCreateSandboxExtensionFromPasteboard(NSPasteboard *pasteboard, SandboxExtension::Handle& sandboxExtensionHandle)
{
    NSArray *types = [pasteboard types];
    if (![types containsObject:NSFilenamesPboardType])
        return;

    NSArray *files = [pasteboard propertyListForType:NSFilenamesPboardType];
    if ([files count] != 1)
        return;

    NSString *file = [files objectAtIndex:0];
    BOOL isDirectory;
    if (![[NSFileManager defaultManager] fileExistsAtPath:file isDirectory:&isDirectory])
        return;

    if (isDirectory)
        return;

    SandboxExtension::createHandle("/", SandboxExtension::ReadOnly, sandboxExtensionHandle);
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)draggingInfo
{
    IntPoint client([self convertPoint:[draggingInfo draggingLocation] fromView:nil]);
    IntPoint global(globalPoint([draggingInfo draggingLocation], [self window]));
    DragData dragData(draggingInfo, client, global, static_cast<DragOperation>([draggingInfo draggingSourceOperationMask]), [self applicationFlags:draggingInfo]);

    SandboxExtension::Handle sandboxExtensionHandle;
    maybeCreateSandboxExtensionFromPasteboard([draggingInfo draggingPasteboard], sandboxExtensionHandle);

    _data->_page->performDrag(&dragData, [[draggingInfo draggingPasteboard] name], sandboxExtensionHandle);

    return YES;
}

// This code is needed to support drag and drop when the drag types cannot be matched.
// This is the case for elements that do not place content
// in the drag pasteboard automatically when the drag start (i.e. dragging a DIV element).
- (NSView *)_hitTest:(NSPoint *)point dragTypes:(NSSet *)types
{
    if ([[self superview] mouse:*point inRect:[self frame]])
        return self;
    return nil;
}

- (BOOL)_windowResizeMouseLocationIsInVisibleScrollerThumb:(NSPoint)loc
{
    NSPoint localPoint = [self convertPoint:loc fromView:nil];
    NSRect visibleThumbRect = NSRect(_data->_page->visibleScrollerThumbRect());
    return NSMouseInRect(localPoint, visibleThumbRect, [self isFlipped]);
}

- (void)_updateWindowVisibility
{
    _data->_page->updateWindowIsVisible(![[self window] isMiniaturized]);
}

- (BOOL)_ownsWindowGrowBox
{
    NSWindow* window = [self window];
    if (!window)
        return NO;

    NSView *superview = [self superview];
    if (!superview)
        return NO;

    NSRect growBoxRect = [window _growBoxRect];
    if (NSIsEmptyRect(growBoxRect))
        return NO;

    NSRect visibleRect = [self visibleRect];
    if (NSIsEmptyRect(visibleRect))
        return NO;

    NSRect visibleRectInWindowCoords = [self convertRect:visibleRect toView:nil];
    if (!NSIntersectsRect(growBoxRect, visibleRectInWindowCoords))
        return NO;

    return YES;
}

- (BOOL)_updateGrowBoxForWindowFrameChange
{
    // Temporarily enable the resize indicator to make a the _ownsWindowGrowBox calculation work.
    BOOL wasShowingIndicator = [[self window] showsResizeIndicator];
    if (!wasShowingIndicator)
        [[self window] setShowsResizeIndicator:YES];

    BOOL ownsGrowBox = [self _ownsWindowGrowBox];
    _data->_page->setWindowResizerSize(ownsGrowBox ? enclosingIntRect([[self window] _growBoxRect]).size() : IntSize());

    if (ownsGrowBox)
        [[self window] _setShowOpaqueGrowBoxForOwner:(_data->_page->hasHorizontalScrollbar() || _data->_page->hasVerticalScrollbar() ? self : nil)];
    else
        [[self window] _setShowOpaqueGrowBoxForOwner:nil];

    // Once WebCore can draw the window resizer, this should read:
    // if (wasShowingIndicator)
    //     [[self window] setShowsResizeIndicator:!ownsGrowBox];
    if (!wasShowingIndicator)
        [[self window] setShowsResizeIndicator:NO];

    return ownsGrowBox;
}

- (void)addWindowObserversForWindow:(NSWindow *)window
{
    if (window) {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowDidBecomeKey:)
                                                     name:NSWindowDidBecomeKeyNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowDidResignKey:)
                                                     name:NSWindowDidResignKeyNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowDidMiniaturize:) 
                                                     name:NSWindowDidMiniaturizeNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowDidDeminiaturize:)
                                                     name:NSWindowDidDeminiaturizeNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowFrameDidChange:)
                                                     name:NSWindowDidMoveNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowFrameDidChange:) 
                                                     name:NSWindowDidResizeNotification object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowDidOrderOffScreen:) 
                                                     name:@"NSWindowDidOrderOffScreenNotification" object:window];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowDidOrderOnScreen:) 
                                                     name:@"_NSWindowDidBecomeVisible" object:window];
    }
}

- (void)removeWindowObservers
{
    NSWindow *window = [self window];
    if (!window)
        return;

    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidBecomeKeyNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidResignKeyNotification object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidMiniaturizeNotification object:window];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidDeminiaturizeNotification object:window];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidMoveNotification object:window];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidResizeNotification object:window];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:@"NSWindowDidOrderOffScreenNotification" object:window];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:@"_NSWindowDidBecomeVisible" object:window];
}

- (void)viewWillMoveToWindow:(NSWindow *)window
{
    NSWindow *currentWindow = [self window];
    if (window == currentWindow)
        return;
    
    [self removeWindowObservers];
    [self addWindowObserversForWindow:window];
    
    if ([currentWindow _growBoxOwner] == self)
        [currentWindow _setShowOpaqueGrowBoxForOwner:nil];
}

- (void)viewDidMoveToWindow
{
    // We want to make sure to update the active state while hidden, so if the view is about to become visible, we
    // update the active state first and then make it visible. If the view is about to be hidden, we hide it first and then
    // update the active state.
    if ([self window]) {
        _data->_page->viewStateDidChange(WebPageProxy::ViewWindowIsActive);
        _data->_page->viewStateDidChange(WebPageProxy::ViewIsVisible | WebPageProxy::ViewIsInWindow);
        [self _updateWindowVisibility];
        [self _updateWindowAndViewFrames];
        
        // Initialize remote accessibility when the window connection has been established.
        NSData *remoteElementToken = WKAXRemoteTokenForElement(self);
        NSData *remoteWindowToken = WKAXRemoteTokenForElement([self accessibilityAttributeValue:NSAccessibilityWindowAttribute]);
        CoreIPC::DataReference elementToken = CoreIPC::DataReference(reinterpret_cast<const uint8_t*>([remoteElementToken bytes]), [remoteElementToken length]);
        CoreIPC::DataReference windowToken = CoreIPC::DataReference(reinterpret_cast<const uint8_t*>([remoteWindowToken bytes]), [remoteWindowToken length]);
        _data->_page->registerUIProcessAccessibilityTokens(elementToken, windowToken);
            
    } else {
        _data->_page->viewStateDidChange(WebPageProxy::ViewIsVisible);
        _data->_page->viewStateDidChange(WebPageProxy::ViewWindowIsActive | WebPageProxy::ViewIsInWindow);

#if ENABLE(GESTURE_EVENTS)
        if (_data->_endGestureMonitor) {
            [NSEvent removeMonitor:_data->_endGestureMonitor];
            _data->_endGestureMonitor = nil;
        }
#endif
#if !defined(BUILDING_ON_SNOW_LEOPARD)
        WKHideWordDefinitionWindow();
#endif
    }
}

- (void)_windowDidBecomeKey:(NSNotification *)notification
{
    NSWindow *keyWindow = [notification object];
    if (keyWindow == [self window] || keyWindow == [[self window] attachedSheet]) {
        [self _updateSecureInputState];
        _data->_page->viewStateDidChange(WebPageProxy::ViewWindowIsActive);
    }
}

- (void)_windowDidResignKey:(NSNotification *)notification
{
    NSWindow *formerKeyWindow = [notification object];
    if (formerKeyWindow == [self window] || formerKeyWindow == [[self window] attachedSheet]) {
        [self _updateSecureInputState];
        _data->_page->viewStateDidChange(WebPageProxy::ViewWindowIsActive);
    }
}

- (void)_windowDidMiniaturize:(NSNotification *)notification
{
    [self _updateWindowVisibility];
}

- (void)_windowDidDeminiaturize:(NSNotification *)notification
{
    [self _updateWindowVisibility];
}

- (void)_windowFrameDidChange:(NSNotification *)notification
{
    [self _updateWindowAndViewFrames];
}

- (void)_windowDidOrderOffScreen:(NSNotification *)notification
{
    _data->_page->viewStateDidChange(WebPageProxy::ViewIsVisible);
}

- (void)_windowDidOrderOnScreen:(NSNotification *)notification
{
    _data->_page->viewStateDidChange(WebPageProxy::ViewIsVisible);
}

static void drawPageBackground(CGContextRef context, WebPageProxy* page, const IntRect& rect)
{
    if (!page->drawsBackground())
        return;

    CGContextSaveGState(context);
    CGContextSetBlendMode(context, kCGBlendModeCopy);

    CGColorRef backgroundColor;
    if (page->drawsTransparentBackground())
        backgroundColor = CGColorGetConstantColor(kCGColorClear);
    else
        backgroundColor = CGColorGetConstantColor(kCGColorWhite);

    CGContextSetFillColorWithColor(context, backgroundColor);
    CGContextFillRect(context, rect);

    CGContextRestoreGState(context);
}

- (void)drawRect:(NSRect)rect
{
    LOG(View, "drawRect: x:%g, y:%g, width:%g, height:%g", rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    _data->_page->endPrinting();
    CGContextRef context = static_cast<CGContextRef>([[NSGraphicsContext currentContext] graphicsPort]);

    if (DrawingAreaProxyImpl* drawingArea = static_cast<DrawingAreaProxyImpl*>(_data->_page->drawingArea())) {
        const NSRect *rectsBeingDrawn;
        NSInteger numRectsBeingDrawn;
        [self getRectsBeingDrawn:&rectsBeingDrawn count:&numRectsBeingDrawn];
        for (NSInteger i = 0; i < numRectsBeingDrawn; ++i) {
            Region unpaintedRegion;
            IntRect rect = enclosingIntRect(rectsBeingDrawn[i]);
            drawingArea->paint(context, rect, unpaintedRegion);

            Vector<IntRect> unpaintedRects = unpaintedRegion.rects();
            for (size_t i = 0; i < unpaintedRects.size(); ++i)
                drawPageBackground(context, _data->_page.get(), unpaintedRects[i]);
        }
    } else 
        drawPageBackground(context, _data->_page.get(), enclosingIntRect(rect));

    _data->_page->didDraw();
}

- (BOOL)isOpaque
{
    return _data->_page->drawsBackground();
}

- (BOOL)mouseDownCanMoveWindow
{
    // -[NSView mouseDownCanMoveWindow] returns YES when the NSView is transparent,
    // but we don't want a drag in the NSView to move the window, even if it's transparent.
    return NO;
}

- (void)viewDidHide
{
    _data->_page->viewStateDidChange(WebPageProxy::ViewIsVisible);
}

- (void)viewDidUnhide
{
    _data->_page->viewStateDidChange(WebPageProxy::ViewIsVisible);
}

- (id)accessibilityFocusedUIElement
{
    if (_data->_pdfViewController)
        return NSAccessibilityUnignoredDescendant(_data->_pdfViewController->pdfView());

    return _data->_remoteAccessibilityChild.get();
}

- (BOOL)accessibilityIsIgnored
{
    return NO;
}

- (id)accessibilityHitTest:(NSPoint)point
{
    if (_data->_pdfViewController)
        return [_data->_pdfViewController->pdfView() accessibilityHitTest:point];
    
    return _data->_remoteAccessibilityChild.get();
}

- (id)accessibilityAttributeValue:(NSString*)attribute
{
    if ([attribute isEqualToString:NSAccessibilityChildrenAttribute]) {

        id child = nil;
        if (_data->_pdfViewController)
            child = NSAccessibilityUnignoredDescendant(_data->_pdfViewController->pdfView());
        else if (_data->_remoteAccessibilityChild)
            child = _data->_remoteAccessibilityChild.get();
        
        if (!child)
            return nil;
        return [NSArray arrayWithObject:child];
    }
    if ([attribute isEqualToString:NSAccessibilityRoleAttribute])
        return NSAccessibilityGroupRole;
    if ([attribute isEqualToString:NSAccessibilityRoleDescriptionAttribute])
        return NSAccessibilityRoleDescription(NSAccessibilityGroupRole, nil);
    if ([attribute isEqualToString:NSAccessibilityParentAttribute])
        return NSAccessibilityUnignoredAncestor([self superview]);
    if ([attribute isEqualToString:NSAccessibilityEnabledAttribute])
        return [NSNumber numberWithBool:YES];
    
    return [super accessibilityAttributeValue:attribute];
}

- (NSView *)hitTest:(NSPoint)point
{
    NSView *hitView = [super hitTest:point];
    if (hitView && _data && hitView == _data->_layerHostingView)
        hitView = self;

    return hitView;
}

- (NSInteger)conversationIdentifier
{
    return (NSInteger)self;
}


- (BOOL)canChangeFrameLayout:(WKFrameRef)frameRef
{
    // PDF documents are already paginated, so we can't change them to add headers and footers.
    return !toImpl(frameRef)->isMainFrame() || !_data->_pdfViewController;
}

- (NSPrintOperation *)printOperationWithPrintInfo:(NSPrintInfo *)printInfo forFrame:(WKFrameRef)frameRef
{
    LOG(View, "Creating an NSPrintOperation for frame '%s'", toImpl(frameRef)->url().utf8().data());

    // Only the top frame can currently contain a PDF view.
    if (_data->_pdfViewController) {
        if (!toImpl(frameRef)->isMainFrame())
            return 0;
        return _data->_pdfViewController->makePrintOperation(printInfo);
    } else {
        RetainPtr<WKPrintingView> printingView(AdoptNS, [[WKPrintingView alloc] initWithFrameProxy:toImpl(frameRef) view:self]);
        // NSPrintOperation takes ownership of the view.
        NSPrintOperation *printOperation = [NSPrintOperation printOperationWithView:printingView.get()];
        [printOperation setCanSpawnSeparateThread:YES];
        [printOperation setJobTitle:toImpl(frameRef)->title()];
        printingView->_printOperation = printOperation;
        return printOperation;
    }
}

@end

@implementation WKView (Internal)

- (PassOwnPtr<WebKit::DrawingAreaProxy>)_createDrawingAreaProxy
{
    return DrawingAreaProxyImpl::create(_data->_page.get());
}

- (BOOL)_isFocused
{
    if (_data->_inBecomeFirstResponder)
        return YES;
    if (_data->_inResignFirstResponder)
        return NO;
    return [[self window] firstResponder] == self;
}

- (void)_processDidCrash
{
    [self setNeedsDisplay:YES];
    [self _updateRemoteAccessibilityRegistration:NO];
}

- (void)_pageClosed
{
    [self _updateRemoteAccessibilityRegistration:NO];
}

- (void)_didRelaunchProcess
{
    [self setNeedsDisplay:YES];
}

- (void)_setCursor:(NSCursor *)cursor
{
    if ([NSCursor currentCursor] == cursor)
        return;
    [cursor set];
}

- (void)_setUserInterfaceItemState:(NSString *)commandName enabled:(BOOL)isEnabled state:(int)newState
{
    ValidationVector items = _data->_validationMap.take(commandName);
    size_t size = items.size();
    for (size_t i = 0; i < size; ++i) {
        ValidationItem item = items[i].get();
        [menuItem(item) setState:newState];
        [menuItem(item) setEnabled:isEnabled];
        [toolbarItem(item) setEnabled:isEnabled];
        // FIXME <rdar://problem/8803392>: If the item is neither a menu nor toolbar item, it will be left enabled.
    }
}

- (void)_resendKeyDownEvent:(NSEvent *)event
{
    // resending the event may destroy this WKView
    RetainPtr<WKView> protector(self);

    ASSERT(!_data->_keyDownEventBeingResent);
    _data->_keyDownEventBeingResent = event;
    [NSApp _setCurrentEvent:event];
    [NSApp sendEvent:event];

    _data->_keyDownEventBeingResent = nullptr;
}

- (BOOL)_interpretKeyEvent:(NSEvent *)event savingCommandsTo:(Vector<WebCore::KeypressCommand>&)commands
{
    ASSERT(!_data->_interpretKeyEventsParameters);
    ASSERT(commands.isEmpty());

    if ([event type] == NSFlagsChanged)
        return NO;

    WKViewInterpretKeyEventsParameters parameters;
    parameters.eventInterpretationHadSideEffects = false;
    parameters.executingSavedKeypressCommands = false;
    // We assume that an input method has consumed the event, and only change this assumption if one of the NSTextInput methods is called.
    // We assume the IM will *not* consume hotkey sequences.
    parameters.consumedByIM = !([event modifierFlags] & NSCommandKeyMask);
    parameters.commands = &commands;
    _data->_interpretKeyEventsParameters = &parameters;

    [self interpretKeyEvents:[NSArray arrayWithObject:event]];

    _data->_interpretKeyEventsParameters = 0;

    // An input method may consume an event and not tell us (e.g. when displaying a candidate window),
    // in which case we should not bubble the event up the DOM.
    if (parameters.consumedByIM)
        return YES;

    // If we have already executed all or some of the commands, the event is "handled". Note that there are additional checks on web process side.
    return parameters.eventInterpretationHadSideEffects;
}

- (NSRect)_convertToDeviceSpace:(NSRect)rect
{
    return toDeviceSpace(rect, [self window]);
}

- (NSRect)_convertToUserSpace:(NSRect)rect
{
    return toUserSpace(rect, [self window]);
}

// Any non-zero value will do, but using something recognizable might help us debug some day.
#define TRACKING_RECT_TAG 0xBADFACE

- (NSTrackingRectTag)addTrackingRect:(NSRect)rect owner:(id)owner userData:(void *)data assumeInside:(BOOL)assumeInside
{
    ASSERT(_data->_trackingRectOwner == nil);
    _data->_trackingRectOwner = owner;
    _data->_trackingRectUserData = data;
    return TRACKING_RECT_TAG;
}

- (NSTrackingRectTag)_addTrackingRect:(NSRect)rect owner:(id)owner userData:(void *)data assumeInside:(BOOL)assumeInside useTrackingNum:(int)tag
{
    ASSERT(tag == 0 || tag == TRACKING_RECT_TAG);
    ASSERT(_data->_trackingRectOwner == nil);
    _data->_trackingRectOwner = owner;
    _data->_trackingRectUserData = data;
    return TRACKING_RECT_TAG;
}

- (void)_addTrackingRects:(NSRect *)rects owner:(id)owner userDataList:(void **)userDataList assumeInsideList:(BOOL *)assumeInsideList trackingNums:(NSTrackingRectTag *)trackingNums count:(int)count
{
    ASSERT(count == 1);
    ASSERT(trackingNums[0] == 0 || trackingNums[0] == TRACKING_RECT_TAG);
    ASSERT(_data->_trackingRectOwner == nil);
    _data->_trackingRectOwner = owner;
    _data->_trackingRectUserData = userDataList[0];
    trackingNums[0] = TRACKING_RECT_TAG;
}

- (void)removeTrackingRect:(NSTrackingRectTag)tag
{
    if (!_data)
        return;

    if (tag == 0)
        return;
    
    if (tag == TRACKING_RECT_TAG) {
        _data->_trackingRectOwner = nil;
        return;
    }
    
    if (tag == _data->_lastToolTipTag) {
        [super removeTrackingRect:tag];
        _data->_lastToolTipTag = 0;
        return;
    }

    // If any other tracking rect is being removed, we don't know how it was created
    // and it's possible there's a leak involved (see 3500217)
    ASSERT_NOT_REACHED();
}

- (void)_removeTrackingRects:(NSTrackingRectTag *)tags count:(int)count
{
    int i;
    for (i = 0; i < count; ++i) {
        int tag = tags[i];
        if (tag == 0)
            continue;
        ASSERT(tag == TRACKING_RECT_TAG);
        if (_data != nil) {
            _data->_trackingRectOwner = nil;
        }
    }
}

- (void)_sendToolTipMouseExited
{
    // Nothing matters except window, trackingNumber, and userData.
    NSEvent *fakeEvent = [NSEvent enterExitEventWithType:NSMouseExited
        location:NSMakePoint(0, 0)
        modifierFlags:0
        timestamp:0
        windowNumber:[[self window] windowNumber]
        context:NULL
        eventNumber:0
        trackingNumber:TRACKING_RECT_TAG
        userData:_data->_trackingRectUserData];
    [_data->_trackingRectOwner mouseExited:fakeEvent];
}

- (void)_sendToolTipMouseEntered
{
    // Nothing matters except window, trackingNumber, and userData.
    NSEvent *fakeEvent = [NSEvent enterExitEventWithType:NSMouseEntered
        location:NSMakePoint(0, 0)
        modifierFlags:0
        timestamp:0
        windowNumber:[[self window] windowNumber]
        context:NULL
        eventNumber:0
        trackingNumber:TRACKING_RECT_TAG
        userData:_data->_trackingRectUserData];
    [_data->_trackingRectOwner mouseEntered:fakeEvent];
}

- (NSString *)view:(NSView *)view stringForToolTip:(NSToolTipTag)tag point:(NSPoint)point userData:(void *)data
{
    return nsStringFromWebCoreString(_data->_page->toolTip());
}

- (void)_toolTipChangedFrom:(NSString *)oldToolTip to:(NSString *)newToolTip
{
    if (oldToolTip)
        [self _sendToolTipMouseExited];

    if (newToolTip && [newToolTip length] > 0) {
        // See radar 3500217 for why we remove all tooltips rather than just the single one we created.
        [self removeAllToolTips];
        NSRect wideOpenRect = NSMakeRect(-100000, -100000, 200000, 200000);
        _data->_lastToolTipTag = [self addToolTipRect:wideOpenRect owner:self userData:NULL];
        [self _sendToolTipMouseEntered];
    }
}

- (void)_setFindIndicator:(PassRefPtr<FindIndicator>)findIndicator fadeOut:(BOOL)fadeOut
{
    if (!findIndicator) {
        _data->_findIndicatorWindow = nullptr;
        return;
    }

    if (!_data->_findIndicatorWindow)
        _data->_findIndicatorWindow = FindIndicatorWindow::create(self);

    _data->_findIndicatorWindow->setFindIndicator(findIndicator, fadeOut);
}

- (void)_enterAcceleratedCompositingMode:(const LayerTreeContext&)layerTreeContext
{
    ASSERT(!_data->_layerHostingView);
    ASSERT(!layerTreeContext.isEmpty());

    // Create an NSView that will host our layer tree.
    _data->_layerHostingView.adoptNS([[NSView alloc] initWithFrame:[self bounds]]);
    [_data->_layerHostingView.get() setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    [self addSubview:_data->_layerHostingView.get()];

    // Create a root layer that will back the NSView.
    RetainPtr<CALayer> rootLayer(AdoptNS, [[CALayer alloc] init]);
#ifndef NDEBUG
    [rootLayer.get() setName:@"Hosting root layer"];
#endif

    CALayer *renderLayer = WKMakeRenderLayer(layerTreeContext.contextID);
    [rootLayer.get() addSublayer:renderLayer];

    [_data->_layerHostingView.get() setLayer:rootLayer.get()];
    [_data->_layerHostingView.get() setWantsLayer:YES];

    [CATransaction commit];
}

- (void)_exitAcceleratedCompositingMode
{
    ASSERT(_data->_layerHostingView);

    [_data->_layerHostingView.get() removeFromSuperview];
    [_data->_layerHostingView.get() setLayer:nil];
    [_data->_layerHostingView.get() setWantsLayer:NO];
    
    _data->_layerHostingView = nullptr;
}

- (void)_setAccessibilityWebProcessToken:(NSData *)data
{
    _data->_remoteAccessibilityChild = WKAXRemoteElementForToken(data);
    [self _updateRemoteAccessibilityRegistration:YES];
}

- (void)_setComplexTextInputEnabled:(BOOL)complexTextInputEnabled pluginComplexTextInputIdentifier:(uint64_t)pluginComplexTextInputIdentifier
{
    BOOL inputSourceChanged = _data->_pluginComplexTextInputIdentifier;

    if (complexTextInputEnabled) {
        // Check if we're already allowing text input for this plug-in.
        if (pluginComplexTextInputIdentifier == _data->_pluginComplexTextInputIdentifier)
            return;

        _data->_pluginComplexTextInputIdentifier = pluginComplexTextInputIdentifier;

    } else {
        // Check if we got a request to disable complex text input for a plug-in that is not the current plug-in.
        if (pluginComplexTextInputIdentifier != _data->_pluginComplexTextInputIdentifier)
            return;

        _data->_pluginComplexTextInputIdentifier = 0;
    }

    if (inputSourceChanged) {
        // Inform the out of line window that the input source changed.
        [[WKTextInputWindowController sharedTextInputWindowController] keyboardInputSourceChanged];
    }
}

- (void)_setPageHasCustomRepresentation:(BOOL)pageHasCustomRepresentation
{
    _data->_pdfViewController = nullptr;

    if (pageHasCustomRepresentation)
        _data->_pdfViewController = PDFViewController::create(self);
}

- (void)_didFinishLoadingDataForCustomRepresentationWithSuggestedFilename:(const String&)suggestedFilename dataReference:(const CoreIPC::DataReference&)dataReference
{
    ASSERT(_data->_pdfViewController);

    _data->_pdfViewController->setPDFDocumentData(_data->_page->mainFrame()->mimeType(), suggestedFilename, dataReference);
}

- (double)_customRepresentationZoomFactor
{
    if (!_data->_pdfViewController)
        return 1;

    return _data->_pdfViewController->zoomFactor();
}

- (void)_setCustomRepresentationZoomFactor:(double)zoomFactor
{
    if (!_data->_pdfViewController)
        return;

    _data->_pdfViewController->setZoomFactor(zoomFactor);
}

- (void)_findStringInCustomRepresentation:(NSString *)string withFindOptions:(WebKit::FindOptions)options maxMatchCount:(NSUInteger)count
{
    if (!_data->_pdfViewController)
        return;

    _data->_pdfViewController->findString(string, options, count);
}

- (void)_countStringMatchesInCustomRepresentation:(NSString *)string withFindOptions:(WebKit::FindOptions)options maxMatchCount:(NSUInteger)count
{
    if (!_data->_pdfViewController)
        return;

    _data->_pdfViewController->countStringMatches(string, options, count);
}

- (void)_setDragImage:(NSImage *)image at:(NSPoint)clientPoint linkDrag:(BOOL)linkDrag
{
    // We need to prevent re-entering this call to avoid crashing in AppKit.
    // Given the asynchronous nature of WebKit2 this can now happen.
    if (_data->_dragHasStarted)
        return;
    
    _data->_dragHasStarted = YES;
    
    // The call to super could release this WKView.
    RetainPtr<WKView> protector(self);
    
    [super dragImage:image
                  at:clientPoint
              offset:NSZeroSize
               event:(linkDrag) ? [NSApp currentEvent] :_data->_mouseDownEvent
          pasteboard:[NSPasteboard pasteboardWithName:NSDragPboard]
              source:self
           slideBack:YES];
    _data->_dragHasStarted = NO;
}

- (void)_updateSecureInputState
{
    if (![[self window] isKeyWindow] || ![self _isFocused]) {
        if (_data->_inSecureInputState) {
            DisableSecureEventInput();
            _data->_inSecureInputState = NO;
        }
        return;
    }
    // WKView has a single input context for all editable areas (except for plug-ins).
    NSTextInputContext *context = [super inputContext];
    bool isInPasswordField = _data->_page->editorState().isInPasswordField;

    if (isInPasswordField) {
        if (!_data->_inSecureInputState)
            EnableSecureEventInput();
        static NSArray *romanInputSources = [[NSArray alloc] initWithObjects:&NSAllRomanInputSourcesLocaleIdentifier count:1];
        [context setAllowedInputSourceLocales:romanInputSources];
    } else {
        if (_data->_inSecureInputState)
            DisableSecureEventInput();
        [context setAllowedInputSourceLocales:nil];
    }
    _data->_inSecureInputState = isInPasswordField;
}

- (void)_updateTextInputStateIncludingSecureInputState:(BOOL)updateSecureInputState
{
    const EditorState& editorState = _data->_page->editorState();
    if (updateSecureInputState) {
        // This is a temporary state when editing. Flipping secure input state too quickly can expose race conditions.
        if (!editorState.selectionIsNone)
            [self _updateSecureInputState];
    }

    if (!editorState.hasComposition || editorState.shouldIgnoreCompositionSelectionChange)
        return;

    _data->_page->confirmCompositionWithoutDisturbingSelection();

    [self _notifyInputContextAboutDiscardedComposition];
}

- (void)_resetTextInputState
{
    [self _notifyInputContextAboutDiscardedComposition];

    if (_data->_inSecureInputState) {
        DisableSecureEventInput();
        _data->_inSecureInputState = NO;
    }
}

- (void)_setDrawingAreaSize:(NSSize)size
{
    if (!_data->_page->drawingArea())
        return;
    
    _data->_page->drawingArea()->setSize(IntSize(size), IntSize(_data->_resizeScrollOffset));
    _data->_resizeScrollOffset = NSZeroSize;
}

- (void)_didChangeScrollbarsForMainFrame
{
    [self _updateGrowBoxForWindowFrameChange];
}

#if ENABLE(FULLSCREEN_API)
- (WKFullScreenWindowController*)fullScreenWindowController
{
    if (!_data->_fullScreenWindowController) {
        _data->_fullScreenWindowController.adoptNS([[WKFullScreenWindowController alloc] init]);
        [_data->_fullScreenWindowController.get() setWebView:self];
    }
    return _data->_fullScreenWindowController.get();
}

- (void)closeFullScreenWindowController
{
    if (!_data->_fullScreenWindowController)
        return;
    [_data->_fullScreenWindowController.get() close];
    _data->_fullScreenWindowController = nullptr;
}
#endif

- (bool)_executeSavedCommandBySelector:(SEL)selector
{
    // The sink does two things: 1) Tells us if the responder went unhandled, and
    // 2) prevents any NSBeep; we don't ever want to beep here.
    RetainPtr<WKResponderChainSink> sink(AdoptNS, [[WKResponderChainSink alloc] initWithResponderChain:self]);
    [super doCommandBySelector:selector];
    [sink.get() detach];
    return ![sink.get() didReceiveUnhandledCommand];
}

- (void)_cacheWindowBottomCornerRect
{
#if !defined(BUILDING_ON_SNOW_LEOPARD)
    // FIXME: We should remove this code when <rdar://problem/9362085> is resolved. 
    NSWindow *window = [self window];
    if (!window)
        return;

    _data->_windowBottomCornerIntersectionRect = [window _intersectBottomCornersWithRect:[self convertRect:[self visibleRect] toView:nil]];
    if (!NSIsEmptyRect(_data->_windowBottomCornerIntersectionRect))
        [self setNeedsDisplayInRect:[self convertRect:_data->_windowBottomCornerIntersectionRect fromView:nil]];
#endif
}

@end

@implementation WKView (Private)

- (void)disableFrameSizeUpdates
{
    _frameSizeUpdatesDisabledCount++;
}

- (void)enableFrameSizeUpdates
{
    if (!_frameSizeUpdatesDisabledCount)
        return;
    
    if (!(--_frameSizeUpdatesDisabledCount))
        [self _setDrawingAreaSize:[self frame].size];
}

- (BOOL)frameSizeUpdatesDisabled
{
    return _frameSizeUpdatesDisabledCount > 0;
}

- (void)performDictionaryLookupAtCurrentMouseLocation
{
    NSPoint thePoint = [NSEvent mouseLocation];
    thePoint = [[self window] convertScreenToBase:thePoint];
    thePoint = [self convertPoint:thePoint fromView:nil];

    _data->_page->performDictionaryLookupAtLocation(FloatPoint(thePoint.x, thePoint.y));
}

- (NSInteger)spellCheckerDocumentTag
{
    if (!_data->_hasSpellCheckerDocumentTag) {
        _data->_spellCheckerDocumentTag = [NSSpellChecker uniqueSpellDocumentTag];
        _data->_hasSpellCheckerDocumentTag = YES;
    }
    return _data->_spellCheckerDocumentTag;
}

- (void)handleCorrectionPanelResult:(NSString*)result
{
    _data->_page->handleCorrectionPanelResult(result);
}

@end

@implementation WKResponderChainSink

- (id)initWithResponderChain:(NSResponder *)chain
{
    self = [super init];
    if (!self)
        return nil;
    _lastResponderInChain = chain;
    while (NSResponder *next = [_lastResponderInChain nextResponder])
        _lastResponderInChain = next;
    [_lastResponderInChain setNextResponder:self];
    return self;
}

- (void)detach
{
    // This assumes that the responder chain was either unmodified since
    // -initWithResponderChain: was called, or was modified in such a way
    // that _lastResponderInChain is still in the chain, and self was not
    // moved earlier in the chain than _lastResponderInChain.
    NSResponder *responderBeforeSelf = _lastResponderInChain;    
    NSResponder *next = [responderBeforeSelf nextResponder];
    for (; next && next != self; next = [next nextResponder])
        responderBeforeSelf = next;
    
    // Nothing to be done if we are no longer in the responder chain.
    if (next != self)
        return;
    
    [responderBeforeSelf setNextResponder:[self nextResponder]];
    _lastResponderInChain = nil;
}

- (bool)didReceiveUnhandledCommand
{
    return _didReceiveUnhandledCommand;
}

- (void)noResponderFor:(SEL)selector
{
    _didReceiveUnhandledCommand = true;
}

- (void)doCommandBySelector:(SEL)selector
{
    _didReceiveUnhandledCommand = true;
}

- (BOOL)tryToPerform:(SEL)action with:(id)object
{
    _didReceiveUnhandledCommand = true;
    return YES;
}

@end
