/*
    Copyright (C) 2008, 2009 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2008 Holger Hans Peter Freyther

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef qwebpage_p_h
#define qwebpage_p_h

#include "QWebPageAdapter.h"

#include "qwebframe.h"
#include "qwebpage.h"

#include <QPointer>
#include <qevent.h>
#include <qgesture.h>
#include <qgraphicssceneevent.h>
#include <qgraphicswidget.h>
#include <qnetworkproxy.h>


namespace WebCore {
class ContextMenuClientQt;
class ContextMenuItem;
class ContextMenu;
class Document;
class EditorClientQt;
class Element;
class IntRect;
class Node;
class NodeList;
class Frame;
}

QT_BEGIN_NAMESPACE
class QUndoStack;
class QMenu;
class QBitArray;
QT_END_NAMESPACE

class QtPluginWidgetAdapter;
class QWebInspector;
class QWebFrameAdapter;
class UndoStepQt;

class QtViewportAttributesPrivate : public QSharedData {
public:
    QtViewportAttributesPrivate(QWebPage::ViewportAttributes* qq)
        : q(qq)
    { }

    QWebPage::ViewportAttributes* q;
};

class QWebPagePrivate : public QWebPageAdapter {
public:
    QWebPagePrivate(QWebPage*);
    ~QWebPagePrivate();

    static WebCore::Page* core(const QWebPage*);

    // Adapter implementation
    void show() override;
    void setFocus() override;
    void unfocus() override;
    void setWindowRect(const QRect &) override;
    QSize viewportSize() const override;
    QWebPageAdapter* createWindow(bool /*dialog*/) override;
    QObject* handle() override { return q; }
    void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID) override;
    void javaScriptAlert(QWebFrameAdapter*, const QString& msg) override;
    bool javaScriptConfirm(QWebFrameAdapter*, const QString& msg) override;
    bool javaScriptPrompt(QWebFrameAdapter*, const QString& msg, const QString& defaultValue, QString* result) override;
    bool shouldInterruptJavaScript() override;
    void printRequested(QWebFrameAdapter*) override;
    void databaseQuotaExceeded(QWebFrameAdapter*, const QString& databaseName) override;
    void applicationCacheQuotaExceeded(QWebSecurityOrigin*, quint64 defaultOriginQuota, quint64 totalSpaceNeeded) override;
    void setToolTip(const QString&) override;
#if USE(QT_MULTIMEDIA)
    QWebFullScreenVideoHandler* createFullScreenVideoHandler() override;
#endif
    QWebFrameAdapter* mainFrameAdapter() override;
    QStringList chooseFiles(QWebFrameAdapter*, bool allowMultiple, const QStringList& suggestedFileNames) override;
    QColor colorSelectionRequested(const QColor& selectedColor) override;
    std::unique_ptr<QWebSelectMethod> createSelectPopup() override;
    QRect viewRectRelativeToWindow() override;
    void geolocationPermissionRequested(QWebFrameAdapter*) override;
    void geolocationPermissionRequestCancelled(QWebFrameAdapter*) override;
    void notificationsPermissionRequested(QWebFrameAdapter*) override;
    void notificationsPermissionRequestCancelled(QWebFrameAdapter*) override;

    void respondToChangedContents() override;
    void respondToChangedSelection() override;
    void microFocusChanged() override;
    void triggerCopyAction() override;
    void triggerActionForKeyEvent(QKeyEvent*) override;
    void clearUndoStack() override;
    bool canUndo() const override;
    bool canRedo() const override;
    void undo() override;
    void redo() override;
    void createUndoStep(QSharedPointer<UndoStepQt>) override;
    const char* editorCommandForKeyEvent(QKeyEvent*) override;

    void updateNavigationActions() override;

    QObject* inspectorHandle() override;
    void setInspectorFrontend(QObject*) override;
    void setInspectorWindowTitle(const QString&) override;
    void createWebInspector(QObject** inspectorView, QWebPageAdapter** inspectorPage) override;
    QStringList menuActionsAsText() override;
    void emitViewportChangeRequested() override;
    bool acceptNavigationRequest(QWebFrameAdapter*, const QNetworkRequest&, int type) override;
    void emitRestoreFrameStateRequested(QWebFrameAdapter*) override;
    void emitSaveFrameStateRequested(QWebFrameAdapter*, QWebHistoryItem*) override;
    void emitDownloadRequested(const QNetworkRequest&) override;
    void emitFrameCreated(QWebFrameAdapter*) override;
    QString userAgentForUrl(const QUrl &url) const override { return q->userAgentForUrl(url); }
    bool supportsErrorPageExtension() const override { return q->supportsExtension(QWebPage::ErrorPageExtension); }
    bool errorPageExtension(ErrorPageOption *, ErrorPageReturn *) override;
    QtPluginWidgetAdapter* createPlugin(const QString &, const QUrl &, const QStringList &, const QStringList &) override;
    QtPluginWidgetAdapter* adapterForWidget(QObject *) const override;
    bool requestSoftwareInputPanel() const override;
    void createAndSetCurrentContextMenu(const QList<MenuItemDescription>&, QBitArray*) override;
    bool handleScrollbarContextMenuEvent(QContextMenuEvent*, bool, ScrollDirection*, ScrollGranularity*) override;


    void createMainFrame();

    void _q_webActionTriggered(bool checked);
    void updateAction(QWebPage::WebAction);
    void updateEditorActions();

    void timerEvent(QTimerEvent*);

#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(const QPoint& globalPos);
#endif
    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent*);

    template<class T> void dragEnterEvent(T*);
    template<class T> void dragMoveEvent(T*);
    template<class T> void dropEvent(T*);

    void shortcutOverrideEvent(QKeyEvent*);
    void leaveEvent(QEvent*);

    bool gestureEvent(QGestureEvent*);


    void setInspector(QWebInspector*);
    QWebInspector* getOrCreateInspector();

#ifndef QT_NO_SHORTCUT
    static QWebPage::WebAction editorActionForKeyEvent(QKeyEvent*);
#endif
    static const char* editorCommandForWebActions(QWebPage::WebAction);

    QWebPage *q;
    QPointer<QWebFrame> mainFrame;

#ifndef QT_NO_UNDOSTACK
    QUndoStack *undoStack;
#endif

    QPointer<QWidget> view;

    QWebPage::LinkDelegationPolicy linkPolicy;

    QSize m_viewportSize;
    QSize fixedLayoutSize;

    QWebHitTestResult hitTestResult;
#ifndef QT_NO_CONTEXTMENU
    QPointer<QMenu> currentContextMenu;
#endif
    QPalette palette;
    bool useFixedLayout;

    QAction *actions[QWebPage::WebActionCount];

    QWidget* inspectorFrontend;
    QWebInspector* inspector;
    bool inspectorIsInternalOnly; // True if created through the Inspect context menu action
    Qt::DropAction m_lastDropAction;
};

#endif
