/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "ChromeClientQt.h"

#include "DataListSuggestionPickerQt.h"
#include "PopupMenuQt.h"
#include "QWebFrameAdapter.h"
#include "QWebPageAdapter.h"
#include "SearchPopupMenuQt.h"
#include "qwebfullscreenrequest.h"
#include "qwebkitplatformplugin.h"
#include "qwebsecurityorigin.h"
#include "qwebsecurityorigin_p.h"
#include "qwebsettings.h"

#include <QEventLoop>
#include <QWindow>
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/ColorChooser.h>
#include <WebCore/ColorChooserClient.h>
#include <WebCore/DatabaseTracker.h>
#include <WebCore/Document.h>
#include <WebCore/FileChooser.h>
#include <WebCore/FileIconLoader.h>
#include <WebCore/FullscreenManager.h>
#include <WebCore/HitTestResult.h>
#include <WebCore/Icon.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/QWebPageClient.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/WindowFeatures.h>
#include <wtf/WallTime.h>

#if USE(TILED_BACKING_STORE)
#include "TiledBackingStore.h"
#endif

#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
#include "FullScreenVideoQt.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLVideoElement.h"
#if USE(QT_MULTIMEDIA)
#include "MediaPlayerPrivateQt.h"
#endif
#endif

#if USE(TEXTURE_MAPPER)
#include "TextureMapperLayerClientQt.h"
#include <WebCore/GraphicsLayer.h>
#endif

namespace WebCore {

bool ChromeClientQt::dumpVisitedLinksCallbacks = false;

ChromeClientQt::ChromeClientQt(QWebPageAdapter* webPageAdapter)
    : m_webPage(webPageAdapter)
    , m_eventLoop(0)
#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
    , m_fullScreenVideo(0)
#endif
{
    toolBarsVisible = statusBarVisible = menuBarVisible = true;
}

ChromeClientQt::~ChromeClientQt()
{
    if (m_eventLoop)
        m_eventLoop->exit();

#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
    delete m_fullScreenVideo;
#endif
}

void ChromeClientQt::setWindowRect(const FloatRect& rect)
{
    if (!m_webPage)
        return;
    m_webPage->setWindowRect(QRect(qRound(rect.x()), qRound(rect.y()), qRound(rect.width()), qRound(rect.height())));
}

/*!
    windowRect represents the rect of the Window, including all interface elements
    like toolbars/scrollbars etc. It is used by the viewport meta tag as well as
    by the DOM Window object: outerHeight(), outerWidth(), screenX(), screenY().
*/
FloatRect ChromeClientQt::windowRect()
{
    if (!platformPageClient())
        return FloatRect();
    return platformPageClient()->windowRect();
}

bool ChromeClientQt::allowsAcceleratedCompositing() const
{
#if USE(TEXTURE_MAPPER)
    if (!platformPageClient())
        return false;
    return true;
#else
    return false;
#endif
}

FloatRect ChromeClientQt::pageRect()
{
    if (!m_webPage)
        return FloatRect();
    return FloatRect(QRectF(QPointF(0, 0), m_webPage->viewportSize()));
}

void ChromeClientQt::focus()
{
    if (!m_webPage)
        return;
    m_webPage->setFocus();
}


void ChromeClientQt::unfocus()
{
    if (!m_webPage)
        return;
    m_webPage->unfocus();
}

bool ChromeClientQt::canTakeFocus(FocusDirection)
{
    // This is called when cycling through links/focusable objects and we
    // reach the last focusable object. Then we want to claim that we can
    // take the focus to avoid wrapping.
    return true;
}

void ChromeClientQt::takeFocus(FocusDirection)
{
    // don't do anything. This is only called when cycling to links/focusable objects,
    // which in turn is called from focusNextPrevChild. We let focusNextPrevChild
    // call QWidget::focusNextPrevChild accordingly, so there is no need to do anything
    // here.
}


void ChromeClientQt::focusedElementChanged(Element* element)
{
    emit m_webPage->focusedElementChanged(QWebElement(element));
}

void ChromeClientQt::focusedFrameChanged(Frame*)
{
}

Page* ChromeClientQt::createWindow(Frame& frame, const FrameLoadRequest&, const WindowFeatures& features, const NavigationAction&)
{
#if ENABLE(FULLSCREEN_API)
    if (frame.document() && frame.document()->fullscreenManager().currentFullscreenElement())
        frame.document()->fullscreenManager().cancelFullscreen();
#else
    UNUSED_PARAM(frame);
#endif

    QWebPageAdapter* newPage = m_webPage->createWindow(features.dialog);
    if (!newPage)
        return 0;

    return newPage->page;
}

void ChromeClientQt::show()
{
    if (!m_webPage)
        return;
    m_webPage->show();
}


bool ChromeClientQt::canRunModal()
{
    return true;
}


void ChromeClientQt::runModal()
{
    m_eventLoop = new QEventLoop();
    QEventLoop* eventLoop = m_eventLoop;
    m_eventLoop->exec();
    delete eventLoop;
}


void ChromeClientQt::setToolbarsVisible(bool visible)
{
    toolBarsVisible = visible;
    QMetaObject::invokeMethod(m_webPage->handle(), "toolBarVisibilityChangeRequested", Q_ARG(bool, visible));
}


bool ChromeClientQt::toolbarsVisible()
{
    return toolBarsVisible;
}


void ChromeClientQt::setStatusbarVisible(bool visible)
{
    QMetaObject::invokeMethod(m_webPage->handle(), "statusBarVisibilityChangeRequested", Q_ARG(bool, visible));
    statusBarVisible = visible;
}


bool ChromeClientQt::statusbarVisible()
{
    return statusBarVisible;
}


void ChromeClientQt::setScrollbarsVisible(bool)
{
    notImplemented();
}


bool ChromeClientQt::scrollbarsVisible()
{
    notImplemented();
    return true;
}


void ChromeClientQt::setMenubarVisible(bool visible)
{
    menuBarVisible = visible;
    QMetaObject::invokeMethod(m_webPage->handle(), "menuBarVisibilityChangeRequested", Q_ARG(bool, visible));
}

bool ChromeClientQt::menubarVisible()
{
    return menuBarVisible;
}

void ChromeClientQt::setResizable(bool)
{
    notImplemented();
}

static inline QWebPageAdapter::MessageSource convertSource(MessageSource source)
{
    switch (source) {
    case MessageSource::XML:                return QWebPageAdapter::XmlMessageSource;
    case MessageSource::JS:                 return QWebPageAdapter::JSMessageSource;
    case MessageSource::Network:            return QWebPageAdapter::NetworkMessageSource;
    case MessageSource::ConsoleAPI:         return QWebPageAdapter::ConsoleAPIMessageSource;
    case MessageSource::Storage:            return QWebPageAdapter::StorageMessageSource;
    case MessageSource::AppCache:           return QWebPageAdapter::AppCacheMessageSource;
    case MessageSource::Rendering:          return QWebPageAdapter::RenderingMessageSource;
    case MessageSource::CSS:                return QWebPageAdapter::CSSMessageSource;
    case MessageSource::Security:           return QWebPageAdapter::SecurityMessageSource;
    case MessageSource::ContentBlocker:     return QWebPageAdapter::ContentBlockerMessageSource;
    case MessageSource::Other:              return QWebPageAdapter::OtherMessageSource;
    }
    Q_UNREACHABLE();
    return QWebPageAdapter::OtherMessageSource;
}

static inline QWebPageAdapter::MessageLevel convertLevel(MessageLevel level)
{
    switch (level) {
    case MessageLevel::Log:         return QWebPageAdapter::LogMessageLevel;
    case MessageLevel::Warning:     return QWebPageAdapter::WarningMessageLevel;
    case MessageLevel::Error:       return QWebPageAdapter::ErrorMessageLevel;
    case MessageLevel::Debug:       return QWebPageAdapter::DebugMessageLevel;
    case MessageLevel::Info:        return QWebPageAdapter::InfoMessageLevel;
    }
    Q_UNREACHABLE();
    return QWebPageAdapter::LogMessageLevel;
}

void ChromeClientQt::addMessageToConsole(MessageSource source, MessageLevel level, const String& message, unsigned lineNumber, unsigned columnNumber, const String& sourceID)
{
    QString x = message;
    QString y = sourceID;
    UNUSED_PARAM(columnNumber);
    m_webPage->consoleMessageReceived(convertSource(source), convertLevel(level), x, lineNumber, y);
}

void ChromeClientQt::chromeDestroyed()
{
    delete this;
}

bool ChromeClientQt::canRunBeforeUnloadConfirmPanel()
{
    return true;
}

bool ChromeClientQt::runBeforeUnloadConfirmPanel(const String& message, Frame& frame)
{
    return runJavaScriptConfirm(frame, message);
}

void ChromeClientQt::closeWindowSoon()
{
    m_webPage->page->setGroupName(String());
    m_webPage->page->mainFrame().loader().stopAllLoaders();
    QMetaObject::invokeMethod(m_webPage->handle(), "windowCloseRequested");
}

void ChromeClientQt::runJavaScriptAlert(Frame& f, const String& msg)
{
    m_webPage->javaScriptAlert(QWebFrameAdapter::kit(f), msg);
}

bool ChromeClientQt::runJavaScriptConfirm(Frame& f, const String& msg)
{
    return m_webPage->javaScriptConfirm(QWebFrameAdapter::kit(f), msg);
}

bool ChromeClientQt::runJavaScriptPrompt(Frame& f, const String& message, const String& defaultValue, String& result)
{
    QString x = result;
    QWebFrameAdapter* webFrame = QWebFrameAdapter::kit(f);
    bool rc = m_webPage->javaScriptPrompt(webFrame, message, defaultValue, &x);

    // Fix up a quirk in the QInputDialog class. If no input happened the string should be empty
    // but it is null. See https://bugs.webkit.org/show_bug.cgi?id=30914.
    if (rc && x.isNull())
        result = emptyString();
    else
        result = x;

    return rc;
}

void ChromeClientQt::setStatusbarText(const String& msg)
{
    QString x = msg;
    QMetaObject::invokeMethod(m_webPage->handle(), "statusBarMessage", Q_ARG(QString, x));
}

KeyboardUIMode ChromeClientQt::keyboardUIMode()
{
    return m_webPage->settings->testAttribute(QWebSettings::LinksIncludedInFocusChain)
        ? KeyboardAccessTabsToLinks : KeyboardAccessDefault;
}

void ChromeClientQt::invalidateRootView(const IntRect& windowRect)
{
#if USE(TILED_BACKING_STORE)
    if (platformPageClient()) {
        WebCore::TiledBackingStore* backingStore = m_webPage->mainFrameAdapter().frame->tiledBackingStore();
        if (!backingStore)
            return;
        backingStore->invalidate(windowRect);
    }
#else
    Q_UNUSED(windowRect);
#endif
}

void ChromeClientQt::invalidateContentsAndRootView(const IntRect& windowRect)
{
    // No double buffer, so only update the QWidget if content changed.
    if (platformPageClient()) {
        QRect rect(windowRect);
        rect = rect.intersected(QRect(QPoint(0, 0), m_webPage->viewportSize()));
        if (!rect.isEmpty())
            platformPageClient()->update(rect);
    }
    QMetaObject::invokeMethod(m_webPage->handle(), "repaintRequested", Qt::QueuedConnection, Q_ARG(QRect, windowRect));

    // FIXME: There is no "immediate" support for window painting. This should be done always whenever the flag
    // is set.
}

void ChromeClientQt::invalidateContentsForSlowScroll(const IntRect& windowRect)
{
    invalidateContentsAndRootView(windowRect);
}

void ChromeClientQt::scroll(const IntSize& delta, const IntRect& scrollViewRect, const IntRect&)
{
    if (platformPageClient())
        platformPageClient()->scroll(delta.width(), delta.height(), scrollViewRect);
    QMetaObject::invokeMethod(m_webPage->handle(), "scrollRequested", Q_ARG(int, delta.width()), Q_ARG(int, delta.height()), Q_ARG(QRect, scrollViewRect));
}

#if USE(COORDINATED_GRAPHICS)
void ChromeClientQt::delegatedScrollRequested(const IntPoint& point)
{

    const QPoint ofs = m_webPage->mainFrameAdapter().scrollPosition();
    IntSize currentPosition(ofs.x(), ofs.y());
    int x = point.x() - currentPosition.width();
    int y = point.y() - currentPosition.height();
    const QRect rect(QPoint(0, 0), m_webPage->viewportSize());
    QMetaObject::invokeMethod(m_webPage->handle(), "scrollRequested", Q_ARG(int, x), Q_ARG(int, y), Q_ARG(QRect, rect));
}
#endif

IntRect ChromeClientQt::rootViewToScreen(const IntRect& rect) const
{
    QWebPageClient* pageClient = platformPageClient();
    if (!pageClient)
        return rect;

    QWindow* ownerWindow = pageClient->ownerWindow();
    if (!ownerWindow)
        return rect;

    QRect screenRect(rect);
    screenRect.translate(ownerWindow->mapToGlobal(m_webPage->viewRectRelativeToWindow().topLeft()));

    return screenRect;
}

IntPoint ChromeClientQt::screenToRootView(const IntPoint& point) const
{
    QWebPageClient* pageClient = platformPageClient();
    if (!pageClient)
        return point;

    QWindow* ownerWindow = pageClient->ownerWindow();
    if (!ownerWindow)
        return point;

    return ownerWindow->mapFromGlobal(point) - m_webPage->viewRectRelativeToWindow().topLeft();
}

PlatformPageClient ChromeClientQt::platformPageClient() const
{
    return m_webPage->client.data();
}

void ChromeClientQt::contentsSizeChanged(Frame& frame, const IntSize& size) const
{
    if (frame.loader().networkingContext())
        QWebFrameAdapter::kit(frame)->contentsSizeDidChange(size);
}

void ChromeClientQt::mouseDidMoveOverElement(const HitTestResult& result, unsigned)
{
    TextDirection dir;
    if (result.absoluteLinkURL() != lastHoverURL
        || result.title(dir) != lastHoverTitle
        || result.textContent() != lastHoverContent) {
        lastHoverURL = result.absoluteLinkURL();
        lastHoverTitle = result.title(dir);
        lastHoverContent = result.textContent();
        QMetaObject::invokeMethod(m_webPage->handle(), "linkHovered", Q_ARG(QString, lastHoverURL.string()),
            Q_ARG(QString, lastHoverTitle), Q_ARG(QString, lastHoverContent));
    }
}

void ChromeClientQt::setToolTip(const String &tip, TextDirection)
{
    m_webPage->setToolTip(tip);
}

void ChromeClientQt::print(Frame& frame)
{
    emit m_webPage->printRequested(QWebFrameAdapter::kit(frame));
}

void ChromeClientQt::exceededDatabaseQuota(Frame& frame, const String& databaseName, DatabaseDetails)
{
    quint64 quota = QWebSettings::offlineStorageDefaultQuota();

    const auto& securityOriginData = frame.document()->securityOrigin().data();
    if (!DatabaseTracker::singleton().usage(securityOriginData))
        DatabaseTracker::singleton().setQuota(securityOriginData, quota);

    m_webPage->databaseQuotaExceeded(QWebFrameAdapter::kit(frame), databaseName);
}

void ChromeClientQt::reachedMaxAppCacheSize(int64_t)
{
    // FIXME: Free some space.
    notImplemented();
}

void ChromeClientQt::reachedApplicationCacheOriginQuota(SecurityOrigin& origin, int64_t totalSpaceNeeded)
{
#ifndef APPLICATION_CACHE_STORAGE_BROKEN
    int64_t quota;
    auto& applicationCacheStorage = ApplicationCacheStorage::singleton();
    quint64 defaultOriginQuota = applicationCacheStorage.defaultOriginQuota();

    QWebSecurityOriginPrivate* priv = new QWebSecurityOriginPrivate(origin);
    QWebSecurityOrigin* securityOrigin = new QWebSecurityOrigin(priv);

    if (!applicationCacheStorage.calculateQuotaForOrigin(origin, quota))
        applicationCacheStorage.storeUpdatedQuotaForOrigin(origin, defaultOriginQuota);

    m_webPage->applicationCacheQuotaExceeded(securityOrigin, defaultOriginQuota, static_cast<quint64>(totalSpaceNeeded));
#endif
}

#if ENABLE(INPUT_TYPE_COLOR)
std::unique_ptr<ColorChooser> ChromeClientQt::createColorChooser(ColorChooserClient& client, const Color& color)
{
    const QColor selectedColor = m_webPage->colorSelectionRequested(QColor(color));
    client.didChooseColor(selectedColor);
    client.didEndChooser();
    return nullptr;
}
#endif

#if ENABLE(DATALIST_ELEMENT)
std::unique_ptr<DataListSuggestionPicker> ChromeClientQt::createDataListSuggestionPicker(DataListSuggestionsClient&)
{
    // QTFIXME: Implement DataListSuggestionPickerQt
    return nullptr;
}
#endif

void ChromeClientQt::runOpenPanel(Frame& frame, FileChooser& fileChooser)
{
    QStringList suggestedFileNames;
    for (unsigned i = 0; i < fileChooser.settings().selectedFiles.size(); ++i)
        suggestedFileNames += fileChooser.settings().selectedFiles[i];

    const bool allowMultiple = fileChooser.settings().allowsMultipleFiles;

    QStringList result = m_webPage->chooseFiles(QWebFrameAdapter::kit(frame), allowMultiple, suggestedFileNames);
    if (!result.isEmpty()) {
        if (allowMultiple) {
            Vector<String> names;
            for (int i = 0; i < result.count(); ++i)
                names.append(result.at(i));
            fileChooser.chooseFiles(names);
        } else
            fileChooser.chooseFile(result.first());
    }
}

void ChromeClientQt::loadIconForFiles(const Vector<String>& filenames, FileIconLoader& loader)
{
    loader.iconLoaded(Icon::createIconForFiles(filenames));
}

void ChromeClientQt::setCursor(const Cursor& cursor)
{
#ifndef QT_NO_CURSOR
    QWebPageClient* pageClient = platformPageClient();
    if (!pageClient)
        return;
    pageClient->setCursor(*cursor.platformCursor());
#else
    UNUSED_PARAM(cursor);
#endif
}

void ChromeClientQt::attachRootGraphicsLayer(Frame& frame, GraphicsLayer* graphicsLayer)
{
#if USE(TEXTURE_MAPPER)
    if (!m_textureMapperLayerClient)
        m_textureMapperLayerClient = std::make_unique<TextureMapperLayerClientQt>(m_webPage->mainFrameAdapter());
    m_textureMapperLayerClient->setRootGraphicsLayer(graphicsLayer);
#endif
}

void ChromeClientQt::setNeedsOneShotDrawingSynchronization()
{
#if USE(TEXTURE_MAPPER)
    // we want the layers to synchronize next time we update the screen anyway
    if (m_textureMapperLayerClient)
        m_textureMapperLayerClient->markForSync(false);
#endif
}

void ChromeClientQt::scheduleCompositingLayerFlush()
{
#if USE(TEXTURE_MAPPER)
    // we want the layers to synchronize ASAP
    if (m_textureMapperLayerClient)
        m_textureMapperLayerClient->markForSync(true);
#endif
}

ChromeClient::CompositingTriggerFlags ChromeClientQt::allowedCompositingTriggers() const
{
    if (allowsAcceleratedCompositing())
        return ThreeDTransformTrigger | CanvasTrigger | AnimationTrigger | AnimatedOpacityTrigger;

    return 0;
}

#if USE(TILED_BACKING_STORE)
IntRect ChromeClientQt::visibleRectForTiledBackingStore() const
{
    if (!platformPageClient() || !m_webPage)
        return IntRect();

    if (!platformPageClient()->viewResizesToContentsEnabled()) {
        const QPoint ofs = m_webPage->mainFrameAdapter().scrollPosition();
        IntSize offset(ofs.x(), ofs.y());
        return QRect(QPoint(offset.width(), offset.height()), m_webPage->mainFrameAdapter().frameRect().size());
    }

    return enclosingIntRect(FloatRect(platformPageClient()->graphicsItemVisibleRect()));
}
#endif

void ChromeClientQt::isPlayingMediaDidChange(MediaProducer::MediaStateFlags state, uint64_t)
{
    if (state == m_mediaState)
        return;

    MediaProducer::MediaStateFlags oldState = m_mediaState;
    m_mediaState = state;

    if ((oldState & MediaProducer::IsPlayingAudio) == (m_mediaState & MediaProducer::IsPlayingAudio))
        return;

    m_webPage->recentlyAudibleChanged(m_mediaState & MediaProducer::IsPlayingAudio);
}

#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
FullScreenVideoQt* ChromeClientQt::fullScreenVideo()
{
    if (!m_fullScreenVideo)
        m_fullScreenVideo = new FullScreenVideoQt(this);
    return m_fullScreenVideo;
}

bool ChromeClientQt::supportsVideoFullscreen(HTMLMediaElementEnums::VideoFullscreenMode)
{
    return fullScreenVideo()->isValid();
}

bool ChromeClientQt::requiresFullscreenForVideoPlayback()
{
    return fullScreenVideo()->requiresFullScreenForVideoPlayback();
}

void ChromeClientQt::enterVideoFullscreenForVideoElement(HTMLVideoElement& videoElement, MediaPlayerEnums::VideoFullscreenMode)
{
    fullScreenVideo()->enterFullScreenForNode(&videoElement);
}

void ChromeClientQt::exitVideoFullscreenForVideoElement(HTMLVideoElement& videoElement)
{
    fullScreenVideo()->exitVideoFullscreen(&videoElement);
}
#endif

#if ENABLE(FULLSCREEN_API)
bool ChromeClientQt::supportsFullScreenForElement(const Element&, bool withKeyboard)
{
    return !withKeyboard;
}

void ChromeClientQt::enterFullScreenForElement(Element& element)
{
    m_webPage->fullScreenRequested(QWebFullScreenRequest::createEnterRequest(m_webPage, QWebElement(&element)));
}

void ChromeClientQt::exitFullScreenForElement(Element* element)
{
    m_webPage->fullScreenRequested(QWebFullScreenRequest::createExitRequest(m_webPage, QWebElement(element)));
}
#endif

std::unique_ptr<QWebSelectMethod> ChromeClientQt::createSelectPopup() const
{
    std::unique_ptr<QWebSelectMethod> result = m_platformPlugin.createSelectInputMethod();
    if (result)
        return result;

#if !defined(QT_NO_COMBOBOX)
    return m_webPage->createSelectPopup();
#else
    return nullptr;
#endif
}

void ChromeClientQt::dispatchViewportPropertiesDidChange(const ViewportArguments&) const
{
    m_webPage->emitViewportChangeRequested();
}

#if USE(QT_MULTIMEDIA)
QWebFullScreenVideoHandler* ChromeClientQt::createFullScreenVideoHandler()
{
    QWebFullScreenVideoHandler* handler = m_platformPlugin.createFullScreenVideoHandler().release();
    if (!handler)
        handler = m_webPage->createFullScreenVideoHandler();
    return handler;
}
#endif

bool ChromeClientQt::selectItemWritingDirectionIsNatural()
{
    return false;
}

bool ChromeClientQt::selectItemAlignmentFollowsMenuWritingDirection()
{
    return false;
}

RefPtr<PopupMenu> ChromeClientQt::createPopupMenu(PopupMenuClient& client) const
{
    return adoptRef(new PopupMenuQt(&client, this));
}

RefPtr<SearchPopupMenu> ChromeClientQt::createSearchPopupMenu(PopupMenuClient& client) const
{
    return adoptRef(new SearchPopupMenuQt(createPopupMenu(client)));
}

void ChromeClientQt::attachViewOverlayGraphicsLayer(WebCore::GraphicsLayer*)
{
}

IntPoint ChromeClientQt::accessibilityScreenToRootView(const IntPoint&) const
{
    return { };
}

IntRect ChromeClientQt::rootViewToAccessibilityScreen(const IntRect&) const
{
    return { };
}

void ChromeClientQt::didFinishLoadingImageForElement(HTMLImageElement&)
{
    // ?
}

void ChromeClientQt::intrinsicContentsSizeChanged(const IntSize&) const
{
}

RefPtr<Icon> ChromeClientQt::createIconForFiles(const Vector<WTF::String>& filenames)
{
    return Icon::createIconForFiles(filenames);
}

} // namespace WebCore
