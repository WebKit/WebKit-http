/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "qwkpage.h"
#include "qwkpage_p.h"

#include "qwkpreferences_p.h"

#include "ClientImpl.h"
#include "DragData.h"
#include "DrawingAreaProxyImpl.h"
#include "qgraphicswkview.h"
#include "qwkcontext.h"
#include "qwkcontext_p.h"
#include "qwkhistory.h"
#include "qwkhistory_p.h"
#include "FindIndicator.h"
#include "LocalizedStrings.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "NotImplemented.h"
#include "Region.h"
#include "TiledDrawingAreaProxy.h"
#include "WebContext.h"
#include "WebContextMenuProxyQt.h"
#include "WebEditCommandProxy.h"
#include "WebEventFactoryQt.h"
#include "WebPopupMenuProxyQt.h"
#include "WebUndoCommandQt.h"
#include "WKStringQt.h"
#include "WKURLQt.h"
#include <QAction>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QStyle>
#include <QTouchEvent>
#include <QUndoStack>
#include <QtDebug>
#include <WebCore/Cursor.h>
#include <WebCore/FloatRect.h>
#include <WebCore/NotImplemented.h>
#include <WebKit2/WKFrame.h>
#include <WebKit2/WKPageGroup.h>
#include <WebKit2/WKRetainPtr.h>

using namespace WebKit;
using namespace WebCore;

static WebCore::ContextMenuAction contextMenuActionForWebAction(QWKPage::WebAction action)
{
    switch (action) {
    case QWKPage::OpenLink:
        return WebCore::ContextMenuItemTagOpenLink;
    case QWKPage::OpenLinkInNewWindow:
        return WebCore::ContextMenuItemTagOpenLinkInNewWindow;
    case QWKPage::CopyLinkToClipboard:
        return WebCore::ContextMenuItemTagCopyLinkToClipboard;
    case QWKPage::OpenImageInNewWindow:
        return WebCore::ContextMenuItemTagOpenImageInNewWindow;
    case QWKPage::Cut:
        return WebCore::ContextMenuItemTagCut;
    case QWKPage::Copy:
        return WebCore::ContextMenuItemTagCopy;
    case QWKPage::Paste:
        return WebCore::ContextMenuItemTagPaste;
    case QWKPage::SelectAll:
        return WebCore::ContextMenuItemTagSelectAll;
    default:
        ASSERT(false);
        break;
    }
    return WebCore::ContextMenuItemTagNoAction;
}

static inline DragOperation dropActionToDragOperation(Qt::DropActions actions)
{
    unsigned result = 0;
    if (actions & Qt::CopyAction)
        result |= DragOperationCopy;
    if (actions & Qt::MoveAction)
        result |= (DragOperationMove | DragOperationGeneric);
    if (actions & Qt::LinkAction)
        result |= DragOperationLink;
    if (result == (DragOperationCopy | DragOperationMove | DragOperationGeneric | DragOperationLink))
        result = DragOperationEvery;
    return (DragOperation)result;
}

static inline Qt::DropAction dragOperationToDropAction(unsigned dragOperation)
{
    Qt::DropAction result = Qt::IgnoreAction;
    if (dragOperation & DragOperationCopy)
        result = Qt::CopyAction;
    else if (dragOperation & DragOperationMove)
        result = Qt::MoveAction;
    else if (dragOperation & DragOperationGeneric)
        result = Qt::MoveAction;
    else if (dragOperation & DragOperationLink)
        result = Qt::LinkAction;
    return result;
}

static inline Qt::DropActions dragOperationToDropActions(unsigned dragOperations)
{
    Qt::DropActions result = Qt::IgnoreAction;
    if (dragOperations & DragOperationCopy)
        result |= Qt::CopyAction;
    if (dragOperations & DragOperationMove)
        result |= Qt::MoveAction;
    if (dragOperations & DragOperationGeneric)
        result |= Qt::MoveAction;
    if (dragOperations & DragOperationLink)
        result |= Qt::LinkAction;
    return result;
}

QWKPagePrivate::QWKPagePrivate(QWKPage* qq, QWKContext* c, WKPageGroupRef pageGroupRef)
    : q(qq)
    , view(0)
    , context(c)
    , preferences(0)
    , createNewPageFn(0)
    , backingStoreType(QGraphicsWKView::Simple)
    , isConnectedToEngine(true)
#ifndef QT_NO_UNDOSTACK
    , undoStack(adoptPtr(new QUndoStack(qq)))
#endif
{
    memset(actions, 0, sizeof(actions));
    page = context->d->context->createWebPage(this, toImpl(pageGroupRef));
    history = QWKHistoryPrivate::createHistory(q, page->backForwardList());
}

QWKPagePrivate::~QWKPagePrivate()
{
    page->close();
    delete history;
}

void QWKPagePrivate::init(QGraphicsItem* view, QGraphicsWKView::BackingStoreType backingStoreType)
{
    this->view = view;
    this->backingStoreType = backingStoreType;
    page->initializeWebPage();
}

void QWKPagePrivate::setCursor(const WebCore::Cursor& cursor)
{
#ifndef QT_NO_CURSOR
    emit q->cursorChanged(*cursor.platformCursor());
#endif
}

void QWKPagePrivate::setViewportArguments(const ViewportArguments& args)
{
    viewportArguments = args;
    emit q->viewportChangeRequested();
}

PassOwnPtr<DrawingAreaProxy> QWKPagePrivate::createDrawingAreaProxy()
{
    // FIXME: We should avoid this cast by decoupling the view from the page.
    QGraphicsWKView* wkView = static_cast<QGraphicsWKView*>(view);

#if ENABLE(TILED_BACKING_STORE)
    if (backingStoreType == QGraphicsWKView::Tiled)
        return TiledDrawingAreaProxy::create(wkView, page.get());
#endif
    return DrawingAreaProxyImpl::create(page.get());
}

void QWKPagePrivate::setViewNeedsDisplay(const WebCore::IntRect& rect)
{
    view->update(QRect(rect));
}

void QWKPagePrivate::displayView()
{
    // FIXME: Implement.
}

void QWKPagePrivate::scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset)
{
    // FIXME: Implement.
}

WebCore::IntSize QWKPagePrivate::viewSize()
{
    return view ? WebCore::IntSize(view->boundingRect().size().toSize()) : WebCore::IntSize();
}

bool QWKPagePrivate::isViewWindowActive()
{
    return view && view->isActive();
}

bool QWKPagePrivate::isViewFocused()
{
    return view && view->hasFocus();
}

bool QWKPagePrivate::isViewVisible()
{
    return view && view->isVisible();
}

bool QWKPagePrivate::isViewInWindow()
{
    // FIXME: Implement.
    return true;
}

void QWKPagePrivate::enterAcceleratedCompositingMode(const LayerTreeContext&)
{
    // FIXME: Implement.
}

void QWKPagePrivate::exitAcceleratedCompositingMode()
{
    // FIXME: Implement.
}

void QWKPagePrivate::pageDidRequestScroll(const IntPoint& point)
{
    emit q->scrollRequested(point.x(), point.y());
}

void QWKPagePrivate::didChangeContentsSize(const IntSize& newSize)
{
    emit q->contentsSizeChanged(QSize(newSize));
}

void QWKPagePrivate::toolTipChanged(const String&, const String& newTooltip)
{
    emit q->toolTipChanged(QString(newTooltip));
}

void QWKPagePrivate::registerEditCommand(PassRefPtr<WebEditCommandProxy> command, WebPageProxy::UndoOrRedo undoOrRedo)
{
#ifndef QT_NO_UNDOSTACK
    if (undoOrRedo == WebPageProxy::Undo) {
        const WebUndoCommandQt* webUndoCommand = static_cast<const WebUndoCommandQt*>(undoStack->command(undoStack->index()));
        if (webUndoCommand && webUndoCommand->inUndoRedo())
            return;
        undoStack->push(new WebUndoCommandQt(command));
    }
#endif
}

void QWKPagePrivate::clearAllEditCommands()
{
#ifndef QT_NO_UNDOSTACK
    undoStack->clear();
#endif
}

bool QWKPagePrivate::canUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
#ifdef QT_NO_UNDOSTACK
    return false;
#else
    if (undoOrRedo == WebPageProxy::Undo)
        return undoStack->canUndo();
    return undoStack->canRedo();
#endif
}

void QWKPagePrivate::executeUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
#ifndef QT_NO_UNDOSTACK
    if (undoOrRedo == WebPageProxy::Undo)
        undoStack->undo();
    else
        undoStack->redo();
#endif
}

FloatRect QWKPagePrivate::convertToDeviceSpace(const FloatRect& rect)
{
    return rect;
}

IntPoint QWKPagePrivate::screenToWindow(const IntPoint& point)
{
    return point;
}

IntRect QWKPagePrivate::windowToScreen(const IntRect& rect)
{
    return rect;
}

FloatRect QWKPagePrivate::convertToUserSpace(const FloatRect& rect)
{
    return rect;
}

void QWKPagePrivate::selectionChanged(bool, bool, bool, bool)
{
}

void QWKPagePrivate::doneWithKeyEvent(const NativeWebKeyboardEvent&, bool)
{
}

PassRefPtr<WebPopupMenuProxy> QWKPagePrivate::createPopupMenuProxy(WebPageProxy*)
{
    return WebPopupMenuProxyQt::create();
}

PassRefPtr<WebContextMenuProxy> QWKPagePrivate::createContextMenuProxy(WebPageProxy*)
{
    return WebContextMenuProxyQt::create(q);
}

void QWKPagePrivate::setFindIndicator(PassRefPtr<FindIndicator>, bool fadeOut)
{
}

void QWKPagePrivate::didCommitLoadForMainFrame(bool useCustomRepresentation)
{
}

void QWKPagePrivate::didFinishLoadingDataForCustomRepresentation(const String& suggestedFilename, const CoreIPC::DataReference&)
{
}

void QWKPagePrivate::flashBackingStoreUpdates(const Vector<IntRect>&)
{
    notImplemented();
}

QWidget* QWKPagePrivate::ownerWidget()
{
    if (!view || !view->scene() || view->scene()->views().isEmpty())
        return 0;

    QList<QGraphicsView*> views = view->scene()->views();
    if (views.size() > 1)
        qWarning("Cannot support multiple views");

    return views.at(0);
}

void QWKPagePrivate::paint(QPainter* painter, QRect area)
{
    if (page->isValid() && page->drawingArea()) {
        if (page->drawingArea()->type() == DrawingAreaTypeImpl) {
            // FIXME: Do something with the unpainted region?
            WebKit::Region unpaintedRegion;
            static_cast<DrawingAreaProxyImpl*>(page->drawingArea())->paint(painter, area, unpaintedRegion);
        } else
            page->drawingArea()->paint(IntRect(area), painter);
    } else
        painter->fillRect(area, Qt::white);
}

void QWKPagePrivate::keyPressEvent(QKeyEvent* ev)
{
    page->handleKeyboardEvent(NativeWebKeyboardEvent(ev));
}

void QWKPagePrivate::keyReleaseEvent(QKeyEvent* ev)
{
    page->handleKeyboardEvent(NativeWebKeyboardEvent(ev));
}

void QWKPagePrivate::mouseMoveEvent(QGraphicsSceneMouseEvent* ev)
{
    // For some reason mouse press results in mouse hover (which is
    // converted to mouse move for WebKit). We ignore these hover
    // events by comparing lastPos with newPos.
    // NOTE: lastPos from the event always comes empty, so we work
    // around that here.
    static QPointF lastPos = QPointF();
    if (lastPos == ev->pos())
        return;
    lastPos = ev->pos();

    page->handleMouseEvent(NativeWebMouseEvent(ev, 0));
}

void QWKPagePrivate::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    if (tripleClickTimer.isActive() && (ev->pos() - tripleClick).manhattanLength() < QApplication::startDragDistance()) {
        page->handleMouseEvent(NativeWebMouseEvent(ev, 3));
        return;
    }

    page->handleMouseEvent(NativeWebMouseEvent(ev, 1));
}

void QWKPagePrivate::mouseReleaseEvent(QGraphicsSceneMouseEvent* ev)
{
    page->handleMouseEvent(NativeWebMouseEvent(ev, 0));
}

void QWKPagePrivate::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* ev)
{
    page->handleMouseEvent(NativeWebMouseEvent(ev, 2));

    tripleClickTimer.start(QApplication::doubleClickInterval(), q);
    tripleClick = ev->pos().toPoint();
}

void QWKPagePrivate::wheelEvent(QGraphicsSceneWheelEvent* ev)
{
    page->handleWheelEvent(NativeWebWheelEvent(ev));
}

void QWKPagePrivate::updateAction(QWKPage::WebAction action)
{
#ifdef QT_NO_ACTION
    Q_UNUSED(action)
#else
    QAction* a = actions[action];
    if (!a)
        return;

    RefPtr<WebKit::WebFrameProxy> mainFrame = page->mainFrame();
    if (!mainFrame)
        return;

    bool enabled = a->isEnabled();
    bool checked = a->isChecked();

    switch (action) {
    case QWKPage::Back:
        enabled = page->canGoBack();
        break;
    case QWKPage::Forward:
        enabled = page->canGoForward();
        break;
    case QWKPage::Stop:
        enabled = !(WebFrameProxy::LoadStateFinished == mainFrame->loadState());
        break;
    case QWKPage::Reload:
        enabled = (WebFrameProxy::LoadStateFinished == mainFrame->loadState());
        break;
    default:
        break;
    }

    a->setEnabled(enabled);

    if (a->isCheckable())
        a->setChecked(checked);
#endif // QT_NO_ACTION
}

void QWKPagePrivate::updateNavigationActions()
{
    updateAction(QWKPage::Back);
    updateAction(QWKPage::Forward);
    updateAction(QWKPage::Stop);
    updateAction(QWKPage::Reload);
}

#ifndef QT_NO_ACTION
void QWKPagePrivate::_q_webActionTriggered(bool checked)
{
    QAction* a = qobject_cast<QAction*>(q->sender());
    if (!a)
        return;
    QWKPage::WebAction action = static_cast<QWKPage::WebAction>(a->data().toInt());
    q->triggerAction(action, checked);
}
#endif // QT_NO_ACTION

void QWKPagePrivate::touchEvent(QTouchEvent* event)
{
#if ENABLE(TOUCH_EVENTS)
    WebTouchEvent touchEvent = WebEventFactory::createWebTouchEvent(event);
    page->handleTouchEvent(touchEvent);
#else
    event->ignore();
#endif
}

void QWKPagePrivate::dragEnterEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    page->resetDragOperation();
    DragData dragData(ev->mimeData(), ev->pos().toPoint(), QCursor::pos(), dropActionToDragOperation(ev->possibleActions()));
    page->dragEntered(&dragData);
    ev->acceptProposedAction();
#else
    Q_UNUSED(ev)
#endif
}

void QWKPagePrivate::dragLeaveEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    DragData dragData(0, IntPoint(), QCursor::pos(), DragOperationNone);
    page->dragExited(&dragData);
    page->resetDragOperation();
#else
    Q_UNUSED(ev)
#endif
}

void QWKPagePrivate::dragMoveEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    DragData dragData(ev->mimeData(), ev->pos().toPoint(), QCursor::pos(), dropActionToDragOperation(ev->possibleActions()));
    page->dragUpdated(&dragData);
    ev->setDropAction(dragOperationToDropAction(page->dragOperation()));
    if (page->dragOperation() != DragOperationNone)
        ev->accept();
#else
    Q_UNUSED(ev)
#endif
}

void QWKPagePrivate::dropEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    DragData dragData(ev->mimeData(), ev->pos().toPoint(), QCursor::pos(), dropActionToDragOperation(ev->possibleActions()));
    SandboxExtension::Handle handle;
    page->performDrag(&dragData, String(), handle);
    ev->setDropAction(dragOperationToDropAction(page->dragOperation()));
    ev->accept();
#else
    Q_UNUSED(ev)
#endif
}

void QWKPagePrivate::didRelaunchProcess()
{
    QGraphicsWKView* wkView = static_cast<QGraphicsWKView*>(view);
    if (wkView)
        q->setViewportSize(wkView->size().toSize());

    isConnectedToEngine = true;
    emit q->engineConnectionChanged(true);
}

void QWKPagePrivate::processDidCrash()
{
    isConnectedToEngine = false;
    emit q->engineConnectionChanged(false);
}

QWKPage::QWKPage(QWKContext* context, WKPageGroupRef pageGroupRef)
    : d(new QWKPagePrivate(this, context, pageGroupRef))
{
    init();
}

QWKPage::QWKPage(QWKContext* context)
    : d(new QWKPagePrivate(this, context))
{
    init();
}

void QWKPage::init()
{
    WKPageLoaderClient loadClient = {
        0,      /* version */
        this,   /* clientInfo */
        qt_wk_didStartProvisionalLoadForFrame,
        qt_wk_didReceiveServerRedirectForProvisionalLoadForFrame,
        qt_wk_didFailProvisionalLoadWithErrorForFrame,
        qt_wk_didCommitLoadForFrame,
        qt_wk_didFinishDocumentLoadForFrame,
        qt_wk_didFinishLoadForFrame,
        qt_wk_didFailLoadWithErrorForFrame,
        qt_wk_didSameDocumentNavigationForFrame,
        qt_wk_didReceiveTitleForFrame,
        qt_wk_didFirstLayoutForFrame,
        qt_wk_didFirstVisuallyNonEmptyLayoutForFrame,
        qt_wk_didRemoveFrameFromHierarchy,
        0, /* didDisplayInsecureContentForFrame */
        0, /* didRunInsecureContentForFrame */
        0, /* canAuthenticateAgainstProtectionSpaceInFrame */
        0, /* didReceiveAuthenticationChallengeInFrame */
        qt_wk_didStartProgress,
        qt_wk_didChangeProgress,
        qt_wk_didFinishProgress,
        qt_wk_didBecomeUnresponsive,
        qt_wk_didBecomeResponsive,
        0,  /* processDidCrash */
        0,  /* didChangeBackForwardList */
        0,  /* shouldGoToBackForwardListItem */
        0   /* didFailToInitializePlugin */
    };
    WKPageSetPageLoaderClient(pageRef(), &loadClient);

    WKPageUIClient uiClient = {
        0,      /* version */
        this,   /* clientInfo */
        qt_wk_createNewPage,
        qt_wk_showPage,
        qt_wk_close,
        qt_wk_takeFocus,
        0,  /* focus */
        0,  /* unfocus */
        qt_wk_runJavaScriptAlert,
        0,  /* runJavaScriptConfirm */
        0,  /* runJavaScriptPrompt */
        qt_wk_setStatusText,
        0,  /* mouseDidMoveOverElement */
        0,  /* missingPluginButtonClicked */
        0,  /* didNotHandleKeyEvent */
        0,  /* didNotHandleWheelEvent */
        0,  /* toolbarsAreVisible */
        0,  /* setToolbarsAreVisible */
        0,  /* menuBarIsVisible */
        0,  /* setMenuBarIsVisible */
        0,  /* statusBarIsVisible */
        0,  /* setStatusBarIsVisible */
        0,  /* isResizable */
        0,  /* setIsResizable */
        0,  /* getWindowFrame */
        0,  /* setWindowFrame */
        0,  /* runBeforeUnloadConfirmPanel */
        0,  /* didDraw */
        0,  /* pageDidScroll */
        0,  /* exceededDatabaseQuota */
        0,  /* runOpenPanel */
        0,  /* decidePolicyForGeolocationPermissionRequest */
        0,  /* headerHeight */
        0,  /* footerHeight */
        0,  /* drawHeader */
        0,  /* drawFooter */
        0,  /* printFrame */
        0,  /* runModal */
        0,  /* didCompleteRubberBandForMainFrame */
        0,  /* saveDataToFileInDownloadsFolder */
        0,  /* shouldInterruptJavaScript */
    };
    WKPageSetPageUIClient(pageRef(), &uiClient);
}

QWKPage::~QWKPage()
{
    delete d;
}

QWKPage::ViewportAttributes::ViewportAttributes()
    : d(0)
    , m_initialScaleFactor(-1.0)
    , m_minimumScaleFactor(-1.0)
    , m_maximumScaleFactor(-1.0)
    , m_devicePixelRatio(-1.0)
    , m_isUserScalable(true)
    , m_isValid(false)
{

}

QWKPage::ViewportAttributes::ViewportAttributes(const QWKPage::ViewportAttributes& other)
    : d(other.d)
    , m_initialScaleFactor(other.m_initialScaleFactor)
    , m_minimumScaleFactor(other.m_minimumScaleFactor)
    , m_maximumScaleFactor(other.m_maximumScaleFactor)
    , m_devicePixelRatio(other.m_devicePixelRatio)
    , m_isUserScalable(other.m_isUserScalable)
    , m_isValid(other.m_isValid)
    , m_size(other.m_size)
{

}

QWKPage::ViewportAttributes::~ViewportAttributes()
{

}

QWKPage::ViewportAttributes& QWKPage::ViewportAttributes::operator=(const QWKPage::ViewportAttributes& other)
{
    if (this != &other) {
        d = other.d;
        m_initialScaleFactor = other.m_initialScaleFactor;
        m_minimumScaleFactor = other.m_minimumScaleFactor;
        m_maximumScaleFactor = other.m_maximumScaleFactor;
        m_devicePixelRatio = other.m_devicePixelRatio;
        m_isUserScalable = other.m_isUserScalable;
        m_isValid = other.m_isValid;
        m_size = other.m_size;
    }

    return *this;
}

QWKPage::ViewportAttributes QWKPage::viewportAttributesForSize(const QSize& availableSize) const
{
    static int desktopWidth = 980;
    static int deviceDPI = 160;

    ViewportAttributes result;

     if (availableSize.isEmpty())
         return result; // Returns an invalid instance.

    // FIXME: Add a way to get these data via the platform plugin and fall back
    // to the size of the view.
    int deviceWidth = 480;
    int deviceHeight = 864;

    WebCore::ViewportAttributes conf = WebCore::computeViewportAttributes(d->viewportArguments, desktopWidth, deviceWidth, deviceHeight, deviceDPI, availableSize);

    result.m_isValid = true;
    result.m_size = conf.layoutSize;
    result.m_initialScaleFactor = conf.initialScale;
    result.m_minimumScaleFactor = conf.minimumScale;
    result.m_maximumScaleFactor = conf.maximumScale;
    result.m_devicePixelRatio = conf.devicePixelRatio;
    result.m_isUserScalable = static_cast<bool>(conf.userScalable);

    return result;
}

void QWKPage::setActualVisibleContentsRect(const QRect& rect) const
{
#if ENABLE(TILED_BACKING_STORE)
    d->page->setActualVisibleContentRect(rect);
#endif
}

void QWKPage::timerEvent(QTimerEvent* ev)
{
    int timerId = ev->timerId();
    if (timerId == d->tripleClickTimer.timerId())
        d->tripleClickTimer.stop();
    else
        QObject::timerEvent(ev);
}

WKPageRef QWKPage::pageRef() const
{
    return toAPI(d->page.get());
}

QWKContext* QWKPage::context() const
{
    return d->context;
}

QWKPreferences* QWKPage::preferences() const
{
    if (!d->preferences) {
        WKPageGroupRef pageGroupRef = WKPageGetPageGroup(pageRef());
        d->preferences = QWKPreferencesPrivate::createPreferences(pageGroupRef);
    }

    return d->preferences;
}

void QWKPage::setCreateNewPageFunction(CreateNewPageFn function)
{
    d->createNewPageFn = function;
}

void QWKPage::setCustomUserAgent(const QString& userAgent)
{
    WKRetainPtr<WKStringRef> wkUserAgent(WKStringCreateWithQString(userAgent));
    WKPageSetCustomUserAgent(pageRef(), wkUserAgent.get());
}

QString QWKPage::customUserAgent() const
{
    return WKStringCopyQString(WKPageCopyCustomUserAgent(pageRef()));
}

void QWKPage::load(const QUrl& url)
{
    WKRetainPtr<WKURLRef> wkurl(WKURLCreateWithQUrl(url));
    WKPageLoadURL(pageRef(), wkurl.get());
}

void QWKPage::setUrl(const QUrl& url)
{
    load(url);
}

QUrl QWKPage::url() const
{
    WKRetainPtr<WKFrameRef> frame = WKPageGetMainFrame(pageRef());
    if (!frame)
        return QUrl();
    return WKURLCopyQUrl(WKFrameCopyURL(frame.get()));
}

QString QWKPage::title() const
{
    return WKStringCopyQString(WKPageCopyTitle(pageRef()));
}

void QWKPage::setViewportSize(const QSize& size)
{
    if (d->page->drawingArea())
        d->page->drawingArea()->setSize(IntSize(size), IntSize());
}

qreal QWKPage::textZoomFactor() const
{
    return WKPageGetTextZoomFactor(pageRef());
}

void QWKPage::setTextZoomFactor(qreal zoomFactor)
{
    WKPageSetTextZoomFactor(pageRef(), zoomFactor);
}

qreal QWKPage::pageZoomFactor() const
{
    return WKPageGetPageZoomFactor(pageRef());
}

void QWKPage::setPageZoomFactor(qreal zoomFactor)
{
    WKPageSetPageZoomFactor(pageRef(), zoomFactor);
}

void QWKPage::setPageAndTextZoomFactors(qreal pageZoomFactor, qreal textZoomFactor)
{
    WKPageSetPageAndTextZoomFactors(pageRef(), pageZoomFactor, textZoomFactor);
}

QWKHistory* QWKPage::history() const
{
    return d->history;
}

void QWKPage::setResizesToContentsUsingLayoutSize(const QSize& targetLayoutSize)
{
#if ENABLE(TILED_BACKING_STORE)
    d->page->setResizesToContentsUsingLayoutSize(targetLayoutSize);
#endif
}

#ifndef QT_NO_ACTION
void QWKPage::triggerAction(WebAction webAction, bool)
{
    switch (webAction) {
    case Back:
        d->page->goBack();
        return;
    case Forward:
        d->page->goForward();
        return;
    case Stop:
        d->page->stopLoading();
        return;
    case Reload:
        d->page->reload(/* reloadFromOrigin */ true);
        return;
    default:
        break;
    }

    QAction* qtAction = action(webAction);
    WebKit::WebContextMenuItemData menuItemData(ActionType, contextMenuActionForWebAction(webAction), qtAction->text(), qtAction->isEnabled(), qtAction->isChecked());
    d->page->contextMenuItemSelected(menuItemData);
}
#endif // QT_NO_ACTION

#ifndef QT_NO_ACTION
QAction* QWKPage::action(WebAction action) const
{
    if (action == QWKPage::NoWebAction || action >= WebActionCount)
        return 0;

    if (d->actions[action])
        return d->actions[action];

    QString text;
    QIcon icon;
    QStyle* style = qobject_cast<QApplication*>(QCoreApplication::instance())->style();
    bool checkable = false;

    switch (action) {
    case OpenLink:
        text = contextMenuItemTagOpenLink();
        break;
    case OpenLinkInNewWindow:
        text = contextMenuItemTagOpenLinkInNewWindow();
        break;
    case CopyLinkToClipboard:
        text = contextMenuItemTagCopyLinkToClipboard();
        break;
    case OpenImageInNewWindow:
        text = contextMenuItemTagOpenImageInNewWindow();
        break;
    case Back:
        text = contextMenuItemTagGoBack();
        icon = style->standardIcon(QStyle::SP_ArrowBack);
        break;
    case Forward:
        text = contextMenuItemTagGoForward();
        icon = style->standardIcon(QStyle::SP_ArrowForward);
        break;
    case Stop:
        text = contextMenuItemTagStop();
        icon = style->standardIcon(QStyle::SP_BrowserStop);
        break;
    case Reload:
        text = contextMenuItemTagReload();
        icon = style->standardIcon(QStyle::SP_BrowserReload);
        break;
    case Cut:
        text = contextMenuItemTagCut();
        break;
    case Copy:
        text = contextMenuItemTagCopy();
        break;
    case Paste:
        text = contextMenuItemTagPaste();
        break;
    case SelectAll:
        text = contextMenuItemTagSelectAll();
        break;
#ifndef QT_NO_UNDOSTACK
    case Undo: {
        QAction* undoAction = d->undoStack->createUndoAction(d->q);
        d->actions[action] = undoAction;
        return undoAction;
    }
    case Redo: {
        QAction* redoAction = d->undoStack->createRedoAction(d->q);
        d->actions[action] = redoAction;
        return redoAction;
    }
#endif
    default:
        return 0;
        break;
    }

    if (text.isEmpty())
        return 0;

    QAction* a = new QAction(d->q);
    a->setText(text);
    a->setData(action);
    a->setCheckable(checkable);
    a->setIcon(icon);

    connect(a, SIGNAL(triggered(bool)), this, SLOT(_q_webActionTriggered(bool)));

    d->actions[action] = a;
    d->updateAction(action);
    return a;
}
#endif // QT_NO_ACTION

void QWKPage::findZoomableAreaForPoint(const QPoint& point)
{
    d->page->findZoomableAreaForPoint(point);
}

void QWKPagePrivate::didFindZoomableArea(const IntRect& area)
{
    emit q->zoomableAreaFound(QRect(area));
}

void QWKPagePrivate::startDrag(const WebCore::DragData& dragData, PassRefPtr<ShareableBitmap> dragImage)
{
    QWidget* widget = ownerWidget();
    if (!widget)
        return;

    QDrag* drag = new QDrag(widget);
    if (dragImage)
        drag->setPixmap(QPixmap::fromImage(dragImage->createQImage()));
    else if (dragData.platformData() && dragData.platformData()->hasImage())
        drag->setPixmap(qvariant_cast<QPixmap>(dragData.platformData()->imageData()));
    DragOperation dragOperationMask = dragData.draggingSourceOperationMask();
    drag->setMimeData(const_cast<QMimeData*>(dragData.platformData()));
    Qt::DropAction actualDropAction = drag->exec(dragOperationToDropActions(dragOperationMask));

    page->dragEnded(widget->mapFromGlobal(QCursor::pos()), QCursor::pos(), dropActionToDragOperation(actualDropAction));
}

WebCore::IntRect QWKPagePrivate::viewportVisibleRect() const
{
    QGraphicsWKView* wkView = static_cast<QGraphicsWKView*>(view);
    return enclosingIntRect(FloatRect(wkView->visibleRect()));
}

bool QWKPage::isConnectedToEngine() const
{
    return d->isConnectedToEngine;
}

#include "moc_qwkpage.cpp"
