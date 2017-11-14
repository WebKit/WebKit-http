/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2009, 2010, 2015, 2016 Igalia S.L
 * Copyright (C) 2015, 2016 Metrological Group B.V.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "MediaPlayerPrivateGStreamerBase.h"

#if ENABLE(VIDEO) && USE(GSTREAMER)

#include "GStreamerUtilities.h"
#include "GraphicsContext.h"
#include "GraphicsTypes.h"
#include "ImageGStreamer.h"
#include "ImageOrientation.h"
#include "IntRect.h"
#include "Logging.h"
#include "MediaPlayer.h"
#include "NotImplemented.h"
#include "SharedBuffer.h"
#include "VideoSinkGStreamer.h"
#include "WebKitWebSourceGStreamer.h"
#include <wtf/glib/GMutexLocker.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/text/CString.h>
#include <wtf/MathExtras.h>
#include <wtf/UUID.h>

#include <gst/audio/streamvolume.h>
#include <gst/video/gstvideometa.h>

#if ENABLE(ENCRYPTED_MEDIA)
#include "CDMInstance.h"
#include "GStreamerEMEUtilities.h"
#include "SharedBuffer.h"
#include "WebKitClearKeyDecryptorGStreamer.h"
#if USE(OPENCDM)
#include "CDMOpenCDM.h"
#include "WebKitOpenCDMPlayReadyDecryptorGStreamer.h"
#include "WebKitOpenCDMWidevineDecryptorGStreamer.h"
#endif
#endif

#if USE(GSTREAMER_GL)
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GST_GL_CAPS_FORMAT "{ BGRx, BGRA }"
#define TEXTURE_MAPPER_COLOR_CONVERT_FLAG TextureMapperGL::ShouldConvertTextureBGRAToRGBA
#define TEXTURE_COPIER_COLOR_CONVERT_FLAG VideoTextureCopierGStreamer::ColorConversion::ConvertBGRAToRGBA
#else
#define GST_GL_CAPS_FORMAT "{ xRGB, ARGB }"
#define TEXTURE_MAPPER_COLOR_CONVERT_FLAG TextureMapperGL::ShouldConvertTextureARGBToRGBA
#define TEXTURE_COPIER_COLOR_CONVERT_FLAG VideoTextureCopierGStreamer::ColorConversion::ConvertARGBToRGBA
#endif

#include <gst/app/gstappsink.h>

#if USE(LIBEPOXY)
// Include the <epoxy/gl.h> header before <gst/gl/gl.h>.
#include <epoxy/gl.h>
#define __gl2_h_

#include <gst/gl/gstglconfig.h>
#undef GST_GL_HAVE_GLSYNC
#define GST_GL_HAVE_GLSYNC 1
#endif

#define GST_USE_UNSTABLE_API
#include <gst/gl/gl.h>
#undef GST_USE_UNSTABLE_API

#include "GLContext.h"
#if USE(GLX)
#include "GLContextGLX.h"
#include <gst/gl/x11/gstgldisplay_x11.h>
#endif

#if USE(EGL)
#include "GLContextEGL.h"
#include <gst/gl/egl/gstgldisplay_egl.h>
#endif

#if PLATFORM(X11)
#include "PlatformDisplayX11.h"
#endif

#if PLATFORM(WAYLAND)
#include "PlatformDisplayWayland.h"
#elif PLATFORM(WPE)
#include "PlatformDisplayWPE.h"
#endif

// gstglapi.h may include eglplatform.h and it includes X.h, which
// defines None, breaking MediaPlayer::None enum
#if PLATFORM(X11) && GST_GL_HAVE_PLATFORM_EGL
#undef None
#endif // PLATFORM(X11) && GST_GL_HAVE_PLATFORM_EGL
#include "VideoTextureCopierGStreamer.h"
#endif // USE(GSTREAMER_GL)

#if USE(TEXTURE_MAPPER_GL)
#include "BitmapTextureGL.h"
#include "BitmapTexturePool.h"
#include "TextureMapperContextAttributes.h"
#include "TextureMapperGL.h"
#include "TextureMapperPlatformLayerBuffer.h"
#include "TextureMapperPlatformLayerProxy.h"
#endif

#define WL_EGL_PLATFORM
#include <EGL/egl.h>

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
#include "WebKitClearKeyDecryptorGStreamer.h"
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
#include "GStreamerEMEUtilities.h"
#include <runtime/JSCInlines.h>
#include <runtime/TypedArrayInlines.h>
#include <runtime/Uint8Array.h>
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
#include "CDMPRSessionGStreamer.h"
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA)
#if USE(OPENCDM)
#include "CDMPrivateOpenCDM.h"
#include "CDMSessionOpenCDM.h"
#include "WebKitOpenCDMPlayReadyDecryptorGStreamer.h"
#include "WebKitOpenCDMWidevineDecryptorGStreamer.h"
#endif // USE(OPENCDM)
#if USE(PLAYREADY)
#include "PlayreadySession.h"
#endif
#include "WebKitPlayReadyDecryptorGStreamer.h"
#endif

#if USE(CAIRO) && ENABLE(ACCELERATED_2D_CANVAS)
#include <cairo-gl.h>
#endif

GST_DEBUG_CATEGORY(webkit_media_player_debug);
#define GST_CAT_DEFAULT webkit_media_player_debug

using namespace std;

namespace WebCore {

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
// Fake UUID (well, actually not, just a special value) to break m_protectionCondition.waitFor() in handleSyncMessage().
static const String ABORT_ENCRYPTION_SETUP_PROTECTION_SYSTEM_UUID = "abort";
#endif

#if ENABLE(NATIVE_AUDIO)
static const GstStreamVolumeFormat volumeFormat = GST_STREAM_VOLUME_FORMAT_LINEAR;
#else
static const GstStreamVolumeFormat volumeFormat = GST_STREAM_VOLUME_FORMAT_CUBIC;
#endif


void registerWebKitGStreamerElements()
{
    if (!webkitGstCheckVersion(1, 6, 1))
        return;

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    GRefPtr<GstElementFactory> clearKeyDecryptorFactory = gst_element_factory_find("webkitclearkey");
    if (!clearKeyDecryptorFactory)
        gst_element_register(nullptr, "webkitclearkey", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_MEDIA_CK_DECRYPT);
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
    GRefPtr<GstElementFactory> clearKeyDecryptorFactory = gst_element_factory_find("webkitclearkey");
    if (!clearKeyDecryptorFactory)
        gst_element_register(0, "webkitclearkey", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_MEDIA_CK_DECRYPT);
#endif

#if (ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)) && USE(PLAYREADY)
    GRefPtr<GstElementFactory> playReadyDecryptorFactory = gst_element_factory_find("webkitplayreadydec");
    if (!playReadyDecryptorFactory)
        gst_element_register(0, "webkitplayreadydec", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_MEDIA_PLAYREADY_DECRYPT);
#endif

#if (ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)) && USE(OPENCDM)
    GRefPtr<GstElementFactory> widevineDecryptorFactory = gst_element_factory_find("webkitopencdmwidevine");
    if (!widevineDecryptorFactory)
        gst_element_register(0, "webkitopencdmwidevine", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_OPENCDM_WIDEVINE_DECRYPT);
    GRefPtr<GstElementFactory> playReadyDecryptorFactory = gst_element_factory_find("webkitplayreadydec");
    if (!playReadyDecryptorFactory)
        gst_element_register(0, "webkitplayreadydec", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_OPENCDM_PLAYREADY_DECRYPT);
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    GRefPtr<GstElementFactory> clearKeyDecryptorFactory = adoptGRef(gst_element_factory_find("webkitclearkey"));
    if (!clearKeyDecryptorFactory)
        gst_element_register(nullptr, "webkitclearkey", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_MEDIA_CK_DECRYPT);

#if USE(OPENCDM)
    GRefPtr<GstElementFactory> widevineDecryptorFactory = adoptGRef(gst_element_factory_find("webkitopencdmwidevine"));
    if (!widevineDecryptorFactory)
        gst_element_register(0, "webkitopencdmwidevine", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_OPENCDM_WIDEVINE_DECRYPT);
    GRefPtr<GstElementFactory> playReadyDecryptorFactory = adoptGRef(gst_element_factory_find("webkitplayreadydec"));
    if (!playReadyDecryptorFactory)
        gst_element_register(0, "webkitplayreadydec", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_OPENCDM_PLAYREADY_DECRYPT);
#endif
#endif
}

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
static const char* keySystemUuidToId(const char*);
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
static const char* keySystemIdToUuid(const char*);
#endif

bool MediaPlayerPrivateGStreamerBase::initializeGStreamerAndRegisterWebKitElements()
{
    if (!initializeGStreamer())
        return false;

    registerWebKitGStreamerElements();

    static bool gstDebugEnabled = false;
    if (!gstDebugEnabled) {
        GST_DEBUG_CATEGORY_INIT(webkit_media_player_debug, "webkitmediaplayer", 0, "WebKit media player");
        gstDebugEnabled = true;
    }

#if USE(GSTREAMER_WEBKIT_HTTP_SRC)
    GRefPtr<GstElementFactory> srcFactory = adoptGRef(gst_element_factory_find("webkitwebsrc"));
    if (!srcFactory)
        gst_element_register(0, "webkitwebsrc", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_WEB_SRC);
#endif

    return true;
}

static int greatestCommonDivisor(int a, int b)
{
    while (b) {
        int temp = a;
        a = b;
        b = temp % b;
    }

    return ABS(a);
}

#if USE(TEXTURE_MAPPER_GL)
static inline TextureMapperGL::Flags texMapFlagFromOrientation(const ImageOrientation& orientation)
{
    switch (orientation) {
    case DefaultImageOrientation:
        return 0;
    case OriginRightTop:
        return TextureMapperGL::ShouldRotateTexture90;
    case OriginBottomRight:
        return TextureMapperGL::ShouldRotateTexture180;
    case OriginLeftBottom:
        return TextureMapperGL::ShouldRotateTexture270;
    default:
        ASSERT_NOT_REACHED();
    }

    return 0;
}
#endif

#if USE(GSTREAMER_GL)
class GstVideoFrameHolder : public TextureMapperPlatformLayerBuffer::UnmanagedBufferDataHolder {
public:
    explicit GstVideoFrameHolder(GstSample* sample, TextureMapperGL::Flags flags)
    {
        GstVideoInfo videoInfo;
        if (UNLIKELY(!getSampleVideoInfo(sample, videoInfo)))
            return;

        m_size = IntSize(GST_VIDEO_INFO_WIDTH(&videoInfo), GST_VIDEO_INFO_HEIGHT(&videoInfo));
        m_flags = flags | (GST_VIDEO_INFO_HAS_ALPHA(&videoInfo) ? TextureMapperGL::ShouldBlend : 0) | TEXTURE_MAPPER_COLOR_CONVERT_FLAG;

        GstBuffer* buffer = gst_sample_get_buffer(sample);
        if (UNLIKELY(!gst_video_frame_map(&m_videoFrame, &videoInfo, buffer, static_cast<GstMapFlags>(GST_MAP_READ | GST_MAP_GL))))
            return;

        m_textureID = *reinterpret_cast<GLuint*>(m_videoFrame.data[0]);
        m_isValid = true;
    }

    virtual ~GstVideoFrameHolder()
    {
        if (UNLIKELY(!m_isValid))
            return;

        gst_video_frame_unmap(&m_videoFrame);
    }

    const IntSize& size() const { return m_size; }
    TextureMapperGL::Flags flags() const { return m_flags; }
    GLuint textureID() const { return m_textureID; }
    GC3Dint internalFormat() const { return m_internalFormat; }
    bool isValid() const { return m_isValid; }

private:
    GstVideoFrame m_videoFrame;
    IntSize m_size;
    TextureMapperGL::Flags m_flags;
    GLuint m_textureID;
    GC3Dint m_internalFormat;
    bool m_isValid { false };
};
#endif // USE(GSTREAMER_GL)

MediaPlayerPrivateGStreamerBase::MediaPlayerPrivateGStreamerBase(MediaPlayer* player)
    : m_notifier(MainThreadNotifier<MainThreadNotification>::create())
    , m_player(player)
    , m_fpsSink(nullptr)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_networkState(MediaPlayer::Empty)
    , m_isEndReached(false)
    , m_usingFallbackVideoSink(false)
    , m_drawTimer(RunLoop::main(), this, &MediaPlayerPrivateGStreamerBase::repaint)
{
    g_mutex_init(&m_sampleMutex);
#if USE(COORDINATED_GRAPHICS_THREADED)
    m_platformLayerProxy = adoptRef(new TextureMapperPlatformLayerProxy());
#endif

#if USE(HOLE_PUNCH_GSTREAMER)
#if USE(COORDINATED_GRAPHICS_THREADED)
    LockHolder locker(m_platformLayerProxy->lock());
    m_platformLayerProxy->pushNextBuffer(std::make_unique<TextureMapperPlatformLayerBuffer>(0, m_size, TextureMapperGL::ShouldOverwriteRect, GraphicsContext3D::DONT_CARE));
#endif
#endif

}

MediaPlayerPrivateGStreamerBase::~MediaPlayerPrivateGStreamerBase()
{
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA)
    m_protectionCondition.notifyAll();
#endif
    m_notifier->invalidate();

#if USE(GSTREAMER_GL) || USE(COORDINATED_GRAPHICS_THREADED)
    m_drawTimer.stop();
    {
        LockHolder locker(m_drawMutex);
        m_drawCondition.notifyOne();
    }
#endif

    if (m_videoSink) {
        g_signal_handlers_disconnect_matched(m_videoSink.get(), G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
#if USE(GSTREAMER_GL)
        if (GST_IS_BIN(m_videoSink.get())) {
            GRefPtr<GstElement> appsink = adoptGRef(gst_bin_get_by_name(GST_BIN_CAST(m_videoSink.get()), "webkit-gl-video-sink"));
            g_signal_handlers_disconnect_by_data(appsink.get(), this);
        }
#endif
    }

    if (m_volumeElement)
        g_signal_handlers_disconnect_matched(m_volumeElement.get(), G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    m_cdmSession = nullptr;
#endif

#if USE(TEXTURE_MAPPER_GL)
    if (client())
        client()->platformLayerWillBeDestroyed();
#endif

    // This will release the GStreamer thread from m_drawCondition if AC is disabled.
    cancelRepaint();

    // The change to GST_STATE_NULL state is always synchronous. So after this gets executed we don't need to worry
    // about handlers running in the GStreamer thread.
    if (m_pipeline)
        gst_element_set_state(m_pipeline.get(), GST_STATE_NULL);

    g_mutex_clear(&m_sampleMutex);

    m_player = nullptr;
}

void MediaPlayerPrivateGStreamerBase::setPipeline(GstElement* pipeline)
{
    m_pipeline = pipeline;
#if USE(HOLE_PUNCH_GSTREAMER)
    updateVideoRectangle();
#endif
}

void MediaPlayerPrivateGStreamerBase::clearSamples()
{
#if USE(COORDINATED_GRAPHICS_THREADED)
    // Disconnect handlers to ensure that new samples aren't going to arrive
    // before the pipeline destruction
    if (m_repaintRequestedHandler) {
        g_signal_handler_disconnect(m_videoSink.get(), m_repaintRequestedHandler);
        m_repaintRequestedHandler = 0;
    }

    if (m_repaintCancelledHandler) {
        g_signal_handler_disconnect(m_videoSink.get(), m_repaintCancelledHandler);
        m_repaintCancelledHandler = 0;
    }

    if (m_drainHandler) {
        g_signal_handler_disconnect(m_videoSink.get(), m_drainHandler);
        m_drainHandler = 0;
    }
#endif

    WTF::GMutexLocker<GMutex> lock(m_sampleMutex);
    m_sample = nullptr;
}

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
static std::pair<Vector<GRefPtr<GstEvent>>, Vector<String>> extractEventsAndSystemsFromMessage(GstMessage* message)
{
    const GstStructure* structure = gst_message_get_structure(message);

    const GValue* streamEncryptionAllowedSystemsValue = gst_structure_get_value(structure, "stream-encryption-systems");
    ASSERT(streamEncryptionAllowedSystemsValue && G_VALUE_HOLDS(streamEncryptionAllowedSystemsValue, G_TYPE_STRV));
    const char** streamEncryptionAllowedSystems = reinterpret_cast<const char**>(g_value_get_boxed(streamEncryptionAllowedSystemsValue));
    ASSERT(streamEncryptionAllowedSystems);
    Vector<String> streamEncryptionAllowedSystemsVector;
    unsigned i;
    for (i = 0; streamEncryptionAllowedSystems[i]; ++i)
        streamEncryptionAllowedSystemsVector.append(streamEncryptionAllowedSystems[i]);

    const GValue* streamEncryptionEventsList = gst_structure_get_value(structure, "stream-encryption-events");
    ASSERT(streamEncryptionEventsList && GST_VALUE_HOLDS_LIST(streamEncryptionEventsList));
    unsigned streamEncryptionEventsListSize = gst_value_list_get_size(streamEncryptionEventsList);
    Vector<GRefPtr<GstEvent>> streamEncryptionEventsVector;
    for (i = 0; i < streamEncryptionEventsListSize; ++i)
        streamEncryptionEventsVector.append(GRefPtr<GstEvent>(static_cast<GstEvent*>(g_value_get_boxed(gst_value_list_get_value(streamEncryptionEventsList, i)))));

    return std::make_pair(streamEncryptionEventsVector, streamEncryptionAllowedSystemsVector);
}
#endif

#if ENABLE(ENCRYPTED_MEDIA)
static std::pair<Vector<GRefPtr<GstEvent>>, Vector<String>> extractEventsAndSystemsFromMessage(GstMessage* message)
{
    const GstStructure* structure = gst_message_get_structure(message);

    const GValue* streamEncryptionAllowedSystemsValue = gst_structure_get_value(structure, "stream-encryption-systems");
    ASSERT(streamEncryptionAllowedSystemsValue && G_VALUE_HOLDS(streamEncryptionAllowedSystemsValue, G_TYPE_STRV));
    const char** streamEncryptionAllowedSystems = reinterpret_cast<const char**>(g_value_get_boxed(streamEncryptionAllowedSystemsValue));
    ASSERT(streamEncryptionAllowedSystems);
    Vector<String> streamEncryptionAllowedSystemsVector;
    unsigned i;
    for (i = 0; streamEncryptionAllowedSystems[i]; ++i)
        streamEncryptionAllowedSystemsVector.append(streamEncryptionAllowedSystems[i]);

    const GValue* streamEncryptionEventsList = gst_structure_get_value(structure, "stream-encryption-events");
    ASSERT(streamEncryptionEventsList && GST_VALUE_HOLDS_LIST(streamEncryptionEventsList));
    unsigned streamEncryptionEventsListSize = gst_value_list_get_size(streamEncryptionEventsList);
    Vector<GRefPtr<GstEvent>> streamEncryptionEventsVector;
    for (i = 0; i < streamEncryptionEventsListSize; ++i)
        streamEncryptionEventsVector.append(GRefPtr<GstEvent>(static_cast<GstEvent*>(g_value_get_boxed(gst_value_list_get_value(streamEncryptionEventsList, i)))));

    return std::make_pair(streamEncryptionEventsVector, streamEncryptionAllowedSystemsVector);
}
#endif

bool MediaPlayerPrivateGStreamerBase::handleSyncMessage(GstMessage* message)
{
    UNUSED_PARAM(message);
    if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_NEED_CONTEXT)
        return false;

    const gchar* contextType;
    gst_message_parse_context_type(message, &contextType);

#if USE(GSTREAMER_GL)
    GRefPtr<GstContext> elementContext = adoptGRef(requestGLContext(contextType, this));
    if (elementContext) {
        gst_element_set_context(GST_ELEMENT(message->src), elementContext.get());
        return true;
    }
#endif // USE(GSTREAMER_GL)

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
    if (!g_strcmp0(contextType, "drm-preferred-decryption-system-id")) {
        if (isMainThread()) {
            GST_ERROR("can't handle drm-preferred-decryption-system-id need context message in the main thread");
            ASSERT_NOT_REACHED();
            return false;
        }
        GST_DEBUG("handling drm-preferred-decryption-system-id need context message");
        LockHolder lock(m_protectionMutex);
        std::pair<Vector<GRefPtr<GstEvent>>, Vector<String>> streamEncryptionInformation = extractEventsAndSystemsFromMessage(message);
        GST_TRACE("found %" G_GSIZE_FORMAT " protection events", streamEncryptionInformation.first.size());
        Vector<uint8_t> concatenatedInitDataChunks;
        unsigned concatenatedInitDataChunksNumber = 0;
        const char* eventKeySystemId = nullptr;
#if USE(PLAYREADY)
        PlayreadySession* prSession = nullptr;
#endif

        for (auto& event : streamEncryptionInformation.first) {
            GST_TRACE("handling protection event %u", GST_EVENT_SEQNUM(event.get()));
            GstBuffer* data = nullptr;
            gst_event_parse_protection(event.get(), &eventKeySystemId, &data, nullptr);

            // Here we receive the DRM init data from the pipeline: we will emit
            // the needkey event with that data and the browser might create a
            // CDMSession from this event handler. If such a session was created
            // We will emit the message event from the session to provide the
            // DRM challenge to the browser and wait for an update. If on the
            // contrary no session was created we won't wait and let the pipeline
            // error out by itself.
            GstMapInfo mapInfo;
            if (!gst_buffer_map(data, &mapInfo, GST_MAP_READ)) {
                GST_WARNING("cannot map %s protection data", eventKeySystemId);
                break;
            }

#if USE(PLAYREADY)
            if (webkit_media_playready_decrypt_is_playready_key_system_id(eventKeySystemId)) {
                Vector<uint8_t> initDataVector;
                initDataVector.append(reinterpret_cast<uint8_t*>(mapInfo.data), mapInfo.size);
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
                bool prSessionAlreadyExisted = false;
                LockHolder prSessionsLocker(m_prSessionsMutex);
                prSession = prSessionByInitData(initDataVector, true);
                if (prSession)
                    prSessionAlreadyExisted = true;
                else {
                    // Preventively creating a new session now, when we still know what pipeline caused the event
                    prSession = createPlayreadySession(initDataVector,
                        getPipeline(GST_ELEMENT(message->src)), true);
                }
                prSessionsLocker.unlockEarly();
                if (!prSessionAlreadyExisted) {
                    LockHolder locker(prSession->mutex());
                    if (prSession->keyRequested() || prSession->ready()) {
                        GST_DEBUG("playready key requested already");
                        if (prSession->ready()) {
                            GST_DEBUG("playready key already negotiated");
                            emitPlayReadySession(prSession);
                        }
                        if (streamEncryptionInformation.second.contains(eventKeySystemId)) {
                            GST_TRACE("considering init data handled for %s", eventKeySystemId);
                            m_handledProtectionEvents.add(GST_EVENT_SEQNUM(event.get()));
                        }
                        return false;
                    }
                }
#elif ENABLE(LEGACY_ENCRYPTED_MEDIA)
                prSession = this->prSession();
                if (prSession && (prSession->keyRequested() || prSession->ready())) {
                    GST_DEBUG("playready key requested already");
                    if (prSession->ready()) {
                        GST_DEBUG("playready key already negotiated");
                        emitPlayReadySession(prSession);
                    }
                    if (streamEncryptionInformation.second.contains(eventKeySystemId)) {
                        GST_TRACE("considering init data handled for %s", eventKeySystemId);
                        m_handledProtectionEvents.add(GST_EVENT_SEQNUM(event.get()));
                    }
                    return false;
                }
#endif
            }
#endif

#if USE(OPENCDM)
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
            LockHolder locker(m_cdmSessionMutex);
#endif
            if (m_cdmSession && (m_cdmSession->keyRequested() || m_cdmSession->ready())) {
                GST_DEBUG("ocdm key requested already");
                if (m_cdmSession->ready()) {
                    GST_DEBUG("ocdm key already negotiated");
                    emitOpenCDMSession();
                }
                return false;
            }
#endif

            GST_TRACE("appending init data for %s of size %u", eventKeySystemId, mapInfo.size);
            GST_MEMDUMP("init data", reinterpret_cast<const unsigned char *>(mapInfo.data), mapInfo.size);
            concatenatedInitDataChunks.append(mapInfo.data, mapInfo.size);
            ++concatenatedInitDataChunksNumber;
            if (streamEncryptionInformation.second.contains(eventKeySystemId)) {
                GST_TRACE("considering init data handled for %s", eventKeySystemId);
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
                Vector<uint8_t> initDataVector;
                initDataVector.append(reinterpret_cast<uint8_t*>(mapInfo.data), mapInfo.size);
                m_initDatas.add(String(eventKeySystemId), WTFMove(initDataVector));
#endif
                m_handledProtectionEvents.add(GST_EVENT_SEQNUM(event.get()));
            }
            gst_buffer_unmap(data, &mapInfo);
        }

        if (!concatenatedInitDataChunksNumber)
            return false;

        if (concatenatedInitDataChunksNumber > 1)
            eventKeySystemId = "";

        String sessionId(createCanonicalUUIDString());
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) && USE(PLAYREADY)
        if (prSession)
            sessionId = prSession->sessionId();
#endif

        WeakPtr<MediaPlayerPrivateGStreamerBase> weakThis = m_weakPtrFactory.createWeakPtr(*this);

        RunLoop::main().dispatch([weakThis, eventKeySystemId, sessionId, initData = WTFMove(concatenatedInitDataChunks)] {
            if (!weakThis) {
                GST_DEBUG("the player private has been destroyed, returning");
                return;
            }

            GST_DEBUG("scheduling keyNeeded event for %s with concatenated init datas size of %" G_GSIZE_FORMAT, eventKeySystemId, initData.size());
            GST_MEMDUMP("init datas", initData.data(), initData.size());

            // FIXME: Provide a somehow valid sessionId.
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
            weakThis->needKey(keySystemUuidToId(eventKeySystemId), "sessionId", initData.data(), initData.size());
#elif ENABLE(LEGACY_ENCRYPTED_MEDIA)
            RefPtr<Uint8Array> initDataArray = Uint8Array::create(initData.data(), initData.size());
            weakThis->needKey(initDataArray);
#else
            ASSERT_NOT_REACHED();
#endif
        });

        GST_INFO("waiting for a key request to arrive");
        m_protectionCondition.waitFor(m_protectionMutex, Seconds(4), [this] {
            return !this->m_lastGenerateKeyRequestKeySystemUuid.isEmpty();
        });
        if (!m_lastGenerateKeyRequestKeySystemUuid.isEmpty() && m_lastGenerateKeyRequestKeySystemUuid != ABORT_ENCRYPTION_SETUP_PROTECTION_SYSTEM_UUID) {
            GST_INFO("got a key request, continuing with %s on %s", m_lastGenerateKeyRequestKeySystemUuid.utf8().data(), GST_MESSAGE_SRC_NAME(message));

            GRefPtr<GstContext> context = adoptGRef(gst_context_new("drm-preferred-decryption-system-id", FALSE));
            GstStructure* contextStructure = gst_context_writable_structure(context.get());
            gst_structure_set(contextStructure, "decryption-system-id", G_TYPE_STRING, m_lastGenerateKeyRequestKeySystemUuid.utf8().data(), nullptr);
            gst_element_set_context(GST_ELEMENT(GST_MESSAGE_SRC(message)), context.get());
        } else
            GST_WARNING("did not get a proper key request");

        return true;
    }
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)

#if ENABLE(ENCRYPTED_MEDIA)
    if (!g_strcmp0(contextType, "drm-preferred-decryption-system-id")) {
        if (isMainThread()) {
            GST_ERROR("can't handle drm-preferred-decryption-system-id need context message in the main thread");
            ASSERT_NOT_REACHED();
            return false;
        }
        GST_DEBUG("handling drm-preferred-decryption-system-id need context message");
        LockHolder lock(m_protectionMutex);
        std::pair<Vector<GRefPtr<GstEvent>>, Vector<String>> streamEncryptionInformation = extractEventsAndSystemsFromMessage(message);
        GST_TRACE("found %" G_GSIZE_FORMAT " protection events", streamEncryptionInformation.first.size());
        Vector<uint8_t> concatenatedInitDataChunks;
        unsigned concatenatedInitDataChunksNumber = 0;
        String eventKeySystemIdString;

        for (auto& event : streamEncryptionInformation.first) {
            GST_TRACE("handling protection event %u", GST_EVENT_SEQNUM(event.get()));
            const char* eventKeySystemId = nullptr;
            GstBuffer* data = nullptr;
            gst_event_parse_protection(event.get(), &eventKeySystemId, &data, nullptr);

            GstMapInfo mapInfo;
            if (!gst_buffer_map(data, &mapInfo, GST_MAP_READ)) {
                GST_WARNING("cannot map %s protection data", eventKeySystemId);
                break;
            }
            GST_TRACE("appending init data for %s of size %" G_GSIZE_FORMAT, eventKeySystemId, mapInfo.size);
            GST_MEMDUMP("init data", reinterpret_cast<const unsigned char *>(mapInfo.data), mapInfo.size);
            concatenatedInitDataChunks.append(mapInfo.data, mapInfo.size);
            ++concatenatedInitDataChunksNumber;
            eventKeySystemIdString = eventKeySystemId;
            if (streamEncryptionInformation.second.contains(eventKeySystemId)) {
                GST_TRACE("considering init data handled for %s", eventKeySystemId);
                m_handledProtectionEvents.add(GST_EVENT_SEQNUM(event.get()));
            }
            gst_buffer_unmap(data, &mapInfo);
        }

        if (!concatenatedInitDataChunksNumber)
            return false;

        if (concatenatedInitDataChunksNumber > 1)
            eventKeySystemIdString = emptyString();

        RunLoop::main().dispatch([weakThis = m_weakPtrFactory.createWeakPtr(*this), eventKeySystemIdString, initData = WTFMove(concatenatedInitDataChunks)] {
            if (!weakThis)
                return;

            GST_DEBUG("scheduling initializationDataEncountered event for %s with concatenated init datas size of %" G_GSIZE_FORMAT, eventKeySystemIdString.utf8().data(), initData.size());
            GST_MEMDUMP("init datas", initData.data(), initData.size());
            weakThis->m_player->initializationDataEncountered(ASCIILiteral("cenc"), ArrayBuffer::create(initData.data(), initData.size()));
        });

        GST_INFO("waiting for a CDM instance");
        m_protectionCondition.waitFor(m_protectionMutex, Seconds(4), [this] {
            return this->m_cdmInstance;
        });
        if (m_cdmInstance && !m_cdmInstance->keySystem().isEmpty()) {
            const char* preferredKeySystemUuid = GStreamerEMEUtilities::keySystemToUuid(m_cdmInstance->keySystem());
            GST_INFO("working with %s, continuing with %s on %s", m_cdmInstance->keySystem().utf8().data(), preferredKeySystemUuid, GST_MESSAGE_SRC_NAME(message));

            GRefPtr<GstContext> context = adoptGRef(gst_context_new("drm-preferred-decryption-system-id", FALSE));
            GstStructure* contextStructure = gst_context_writable_structure(context.get());
            gst_structure_set(contextStructure, "decryption-system-id", G_TYPE_STRING, preferredKeySystemUuid, nullptr);
            gst_element_set_context(GST_ELEMENT(GST_MESSAGE_SRC(message)), context.get());
        } else
            GST_WARNING("no proper CDM instance attached");

        return true;
    }
#endif // ENABLE(ENCRYPTED_MEDIA)

    return false;
}

#if USE(GSTREAMER_GL)
GstContext* MediaPlayerPrivateGStreamerBase::requestGLContext(const gchar* contextType, MediaPlayerPrivateGStreamerBase* player)
{
    if (!player->ensureGstGLContext())
        return nullptr;

    if (!g_strcmp0(contextType, GST_GL_DISPLAY_CONTEXT_TYPE)) {
        GstContext* displayContext = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
        gst_context_set_gl_display(displayContext, player->gstGLDisplay());
        return displayContext;
    }

    if (!g_strcmp0(contextType, "gst.gl.app_context")) {
        GstContext* appContext = gst_context_new("gst.gl.app_context", TRUE);
        GstStructure* structure = gst_context_writable_structure(appContext);
#if GST_CHECK_VERSION(1, 11, 0)
        gst_structure_set(structure, "context", GST_TYPE_GL_CONTEXT, player->gstGLContext(), nullptr);
#else
        gst_structure_set(structure, "context", GST_GL_TYPE_CONTEXT, player->gstGLContext(), nullptr);
#endif
        return appContext;
    }

    return nullptr;
}

bool MediaPlayerPrivateGStreamerBase::ensureGstGLContext()
{
    if (m_glContext)
        return true;

    auto& sharedDisplay = PlatformDisplay::sharedDisplayForCompositing();
    if (!m_glDisplay) {
#if PLATFORM(X11)
#if USE(GLX)
        if (is<PlatformDisplayX11>(sharedDisplay))
            m_glDisplay = GST_GL_DISPLAY(gst_gl_display_x11_new_with_display(downcast<PlatformDisplayX11>(sharedDisplay).native()));
#elif USE(EGL)
        if (is<PlatformDisplayX11>(sharedDisplay))
            m_glDisplay = GST_GL_DISPLAY(gst_gl_display_egl_new_with_egl_display(downcast<PlatformDisplayX11>(sharedDisplay).eglDisplay()));
#endif
#endif

#if PLATFORM(WAYLAND)
        if (is<PlatformDisplayWayland>(sharedDisplay))
            m_glDisplay = GST_GL_DISPLAY(gst_gl_display_egl_new_with_egl_display(downcast<PlatformDisplayWayland>(sharedDisplay).eglDisplay()));
#endif

#if PLATFORM(WPE)
        ASSERT(is<PlatformDisplayWPE>(sharedDisplay));
        m_glDisplay = GST_GL_DISPLAY(gst_gl_display_egl_new_with_egl_display(downcast<PlatformDisplayWPE>(sharedDisplay).eglDisplay()));
#endif

        ASSERT(m_glDisplay);
    }

    GLContext* webkitContext = sharedDisplay.sharingGLContext();
    // EGL and GLX are mutually exclusive, no need for ifdefs here.
    GstGLPlatform glPlatform = webkitContext->isEGLContext() ? GST_GL_PLATFORM_EGL : GST_GL_PLATFORM_GLX;

#if USE(OPENGL_ES_2)
    GstGLAPI glAPI = GST_GL_API_GLES2;
#elif USE(OPENGL)
    GstGLAPI glAPI = GST_GL_API_OPENGL;
#else
    ASSERT_NOT_REACHED();
#endif

    PlatformGraphicsContext3D contextHandle = webkitContext->platformContext();
    if (!contextHandle)
        return false;

    m_glContext = gst_gl_context_new_wrapped(m_glDisplay.get(), reinterpret_cast<guintptr>(contextHandle), glPlatform, glAPI);

    return true;
}
#endif // USE(GSTREAMER_GL)

// Returns the size of the video
FloatSize MediaPlayerPrivateGStreamerBase::naturalSize() const
{
    if (!hasVideo())
        return { };

    if (!m_videoSize.isEmpty())
        return m_videoSize;

    WTF::GMutexLocker<GMutex> lock(m_sampleMutex);

    GRefPtr<GstCaps> caps;
    // We may not have enough data available for the video sink yet.
    if (!GST_IS_SAMPLE(m_sample.get()))
        return { };

    if (GST_IS_SAMPLE(m_sample.get()) && !caps)
        caps = gst_sample_get_caps(m_sample.get());

    if (!caps) {
        GRefPtr<GstPad> videoSinkPad = adoptGRef(gst_element_get_static_pad(m_videoSink.get(), "sink"));
        if (videoSinkPad)
            caps = gst_pad_get_current_caps(videoSinkPad.get());
    }

    if (!caps)
        return { };

    // TODO: handle possible clean aperture data. See
    // https://bugzilla.gnome.org/show_bug.cgi?id=596571
    // TODO: handle possible transformation matrix. See
    // https://bugzilla.gnome.org/show_bug.cgi?id=596326

    // Get the video PAR and original size, if this fails the
    // video-sink has likely not yet negotiated its caps.
    int pixelAspectRatioNumerator, pixelAspectRatioDenominator, stride;
    IntSize originalSize;
    GstVideoFormat format;
    if (!getVideoSizeAndFormatFromCaps(caps.get(), originalSize, format, pixelAspectRatioNumerator, pixelAspectRatioDenominator, stride))
        return { };

    // Sanity check for the unlikely, but reproducible case when getVideoSizeAndFormatFromCaps returns incorrect values.
    if (!originalSize.width() || !originalSize.height() || !pixelAspectRatioNumerator || !pixelAspectRatioNumerator) {
        GST_DEBUG("getVideoSizeAndFormatFromCaps returned an invalid info, returning an empty size");
        return { };
    }

    // Sanity check for the unlikely, but reproducible case when getVideoSizeAndFormatFromCaps returns incorrect values
    if ((originalSize.width() == 0) || (originalSize.height() == 0)
       || (pixelAspectRatioNumerator == 0) || (pixelAspectRatioNumerator == 0)) {
        GST_DEBUG("getVideoSizeAndFormatFromCaps returned an invalid info, returning an empty size");
        return FloatSize();
    }

#if USE(TEXTURE_MAPPER_GL)
    // When using accelerated compositing, if the video is tagged as rotated 90 or 270 degrees, swap width and height.
    if (m_renderingCanBeAccelerated) {
        if (m_videoSourceOrientation.usesWidthAsHeight())
            originalSize = originalSize.transposedSize();
    }
#endif

    GST_DEBUG("Original video size: %dx%d", originalSize.width(), originalSize.height());
    GST_DEBUG("Pixel aspect ratio: %d/%d", pixelAspectRatioNumerator, pixelAspectRatioDenominator);

    // Calculate DAR based on PAR and video size.
    int displayWidth = originalSize.width() * pixelAspectRatioNumerator;
    int displayHeight = originalSize.height() * pixelAspectRatioDenominator;

    // Divide display width and height by their GCD to avoid possible overflows.
    int displayAspectRatioGCD = greatestCommonDivisor(displayWidth, displayHeight);
    displayWidth /= displayAspectRatioGCD;
    displayHeight /= displayAspectRatioGCD;

    // Apply DAR to original video size. This is the same behavior as in xvimagesink's setcaps function.
    guint64 width = 0, height = 0;
    if (!(originalSize.height() % displayHeight)) {
        GST_DEBUG("Keeping video original height");
        width = gst_util_uint64_scale_int(originalSize.height(), displayWidth, displayHeight);
        height = static_cast<guint64>(originalSize.height());
    } else if (!(originalSize.width() % displayWidth)) {
        GST_DEBUG("Keeping video original width");
        height = gst_util_uint64_scale_int(originalSize.width(), displayHeight, displayWidth);
        width = static_cast<guint64>(originalSize.width());
    } else {
        GST_DEBUG("Approximating while keeping original video height");
        width = gst_util_uint64_scale_int(originalSize.height(), displayWidth, displayHeight);
        height = static_cast<guint64>(originalSize.height());
    }

    GST_DEBUG("Natural size: %" G_GUINT64_FORMAT "x%" G_GUINT64_FORMAT, width, height);
    m_videoSize = FloatSize(static_cast<int>(width), static_cast<int>(height));
    return m_videoSize;
}

void MediaPlayerPrivateGStreamerBase::setVolume(float volume)
{
    if (!m_volumeElement)
        return;

    GST_DEBUG("Setting volume: %f", volume);
    gst_stream_volume_set_volume(m_volumeElement.get(), volumeFormat, static_cast<double>(volume));
}

float MediaPlayerPrivateGStreamerBase::volume() const
{
    if (!m_volumeElement)
        return 0;

    return gst_stream_volume_get_volume(m_volumeElement.get(), volumeFormat);
}


void MediaPlayerPrivateGStreamerBase::notifyPlayerOfVolumeChange()
{
    if (!m_player || !m_volumeElement)
        return;
    double volume;
    volume = gst_stream_volume_get_volume(m_volumeElement.get(), volumeFormat);
    // get_volume() can return values superior to 1.0 if the user
    // applies software user gain via third party application (GNOME
    // volume control for instance).
    volume = CLAMP(volume, 0.0, 1.0);
    m_player->volumeChanged(static_cast<float>(volume));
}

void MediaPlayerPrivateGStreamerBase::volumeChangedCallback(MediaPlayerPrivateGStreamerBase* player)
{
#if PLATFORM(WPE)
    // This is called when m_volumeElement receives the notify::volume signal.
    GST_DEBUG("Volume changed to: %f", player->volume());
#endif

    player->m_notifier->notify(MainThreadNotification::VolumeChanged, [player] { player->notifyPlayerOfVolumeChange(); });
}

MediaPlayer::NetworkState MediaPlayerPrivateGStreamerBase::networkState() const
{
    return m_networkState;
}

MediaPlayer::ReadyState MediaPlayerPrivateGStreamerBase::readyState() const
{
    return m_readyState;
}

void MediaPlayerPrivateGStreamerBase::sizeChanged()
{
    notImplemented();
}

void MediaPlayerPrivateGStreamerBase::setMuted(bool muted)
{
    if (!m_volumeElement)
        return;

    g_object_set(m_volumeElement.get(), "mute", muted, nullptr);
}

bool MediaPlayerPrivateGStreamerBase::muted() const
{
    if (!m_volumeElement)
        return false;

    gboolean muted;
    g_object_get(m_volumeElement.get(), "mute", &muted, nullptr);
    return muted;
}

void MediaPlayerPrivateGStreamerBase::notifyPlayerOfMute()
{
    if (!m_player || !m_volumeElement)
        return;

    gboolean muted;
    g_object_get(m_volumeElement.get(), "mute", &muted, nullptr);
    m_player->muteChanged(static_cast<bool>(muted));
}

void MediaPlayerPrivateGStreamerBase::muteChangedCallback(MediaPlayerPrivateGStreamerBase* player)
{
    // This is called when m_volumeElement receives the notify::mute signal.
    player->m_notifier->notify(MainThreadNotification::MuteChanged, [player] { player->notifyPlayerOfMute(); });
}

void MediaPlayerPrivateGStreamerBase::acceleratedRenderingStateChanged()
{
    m_renderingCanBeAccelerated = m_player && m_player->client().mediaPlayerAcceleratedCompositingEnabled();
}

#if USE(TEXTURE_MAPPER_GL)
void MediaPlayerPrivateGStreamerBase::updateTexture(BitmapTextureGL& texture, GstVideoInfo& videoInfo)
{
    GstBuffer* buffer = gst_sample_get_buffer(m_sample.get());

    GstVideoGLTextureUploadMeta* meta;
    if ((meta = gst_buffer_get_video_gl_texture_upload_meta(buffer))) {
        if (meta->n_textures == 1) { // BRGx & BGRA formats use only one texture.
            guint ids[4] = { texture.id(), 0, 0, 0 };

            if (gst_video_gl_texture_upload_meta_upload(meta, ids))
                return;
        }
    }

    // Right now the TextureMapper only supports chromas with one plane
    ASSERT(GST_VIDEO_INFO_N_PLANES(&videoInfo) == 1);

    GstVideoFrame videoFrame;
    if (!gst_video_frame_map(&videoFrame, &videoInfo, buffer, GST_MAP_READ))
        return;

    int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&videoFrame, 0);
    const void* srcData = GST_VIDEO_FRAME_PLANE_DATA(&videoFrame, 0);
    texture.updateContents(srcData, WebCore::IntRect(0, 0, GST_VIDEO_INFO_WIDTH(&videoInfo), GST_VIDEO_INFO_HEIGHT(&videoInfo)), WebCore::IntPoint(0, 0), stride, BitmapTexture::UpdateCannotModifyOriginalImageData);
    gst_video_frame_unmap(&videoFrame);
}

RefPtr<TextureMapperPlatformLayerProxy> MediaPlayerPrivateGStreamerBase::proxy() const
{
    return m_platformLayerProxy.copyRef();
}

void MediaPlayerPrivateGStreamerBase::swapBuffersIfNeeded()
{
}

void MediaPlayerPrivateGStreamerBase::pushTextureToCompositor()
{
#if !USE(GSTREAMER_GL)
    class ConditionNotifier {
    public:
        ConditionNotifier(Lock& lock, Condition& condition)
            : m_locker(lock), m_condition(condition)
        {
        }
        ~ConditionNotifier()
        {
            m_condition.notifyOne();
        }
    private:
        LockHolder m_locker;
        Condition& m_condition;
    };
    ConditionNotifier notifier(m_drawMutex, m_drawCondition);
#endif

    WTF::GMutexLocker<GMutex> lock(m_sampleMutex);
    if (!GST_IS_SAMPLE(m_sample.get()))
        return;

    LockHolder holder(m_platformLayerProxy->lock());

    if (!m_platformLayerProxy->isActive())
        return;

#if USE(GSTREAMER_GL)
    std::unique_ptr<GstVideoFrameHolder> frameHolder = std::make_unique<GstVideoFrameHolder>(m_sample.get(), texMapFlagFromOrientation(m_videoSourceOrientation));
    if (UNLIKELY(!frameHolder->isValid()))
        return;

    std::unique_ptr<TextureMapperPlatformLayerBuffer> layerBuffer = std::make_unique<TextureMapperPlatformLayerBuffer>(frameHolder->textureID(), frameHolder->size(), frameHolder->flags(), GraphicsContext3D::RGBA);
    layerBuffer->setUnmanagedBufferDataHolder(WTFMove(frameHolder));
    m_platformLayerProxy->pushNextBuffer(WTFMove(layerBuffer));
#else
    GstVideoInfo videoInfo;
    if (UNLIKELY(!getSampleVideoInfo(m_sample.get(), videoInfo)))
        return;

    IntSize size = IntSize(GST_VIDEO_INFO_WIDTH(&videoInfo), GST_VIDEO_INFO_HEIGHT(&videoInfo));
    std::unique_ptr<TextureMapperPlatformLayerBuffer> buffer = m_platformLayerProxy->getAvailableBuffer(size, GraphicsContext3D::DONT_CARE);
    if (UNLIKELY(!buffer)) {
        if (UNLIKELY(!m_context3D))
            m_context3D = GraphicsContext3D::create(GraphicsContext3DAttributes(), nullptr, GraphicsContext3D::RenderToCurrentGLContext);

        TextureMapperContextAttributes contextAttributes;
        contextAttributes.initialize();

        auto texture = BitmapTextureGL::create(contextAttributes);
        texture->reset(size, GST_VIDEO_INFO_HAS_ALPHA(&videoInfo) ? BitmapTexture::SupportsAlpha : BitmapTexture::NoFlag);
        buffer = std::make_unique<TextureMapperPlatformLayerBuffer>(WTFMove(texture));
    }
    updateTexture(buffer->textureGL(), videoInfo);
    buffer->setExtraFlags(texMapFlagFromOrientation(m_videoSourceOrientation) | (GST_VIDEO_INFO_HAS_ALPHA(&videoInfo) ? TextureMapperGL::ShouldBlend : 0));
    m_platformLayerProxy->pushNextBuffer(WTFMove(buffer));
#endif // USE(GSTREAMER_GL)
}
#endif // USE(TEXTURE_MAPPER_GL)

void MediaPlayerPrivateGStreamerBase::repaint()
{
    ASSERT(m_sample);
    ASSERT(isMainThread());

    m_player->repaint();

    LockHolder lock(m_drawMutex);
    m_drawCondition.notifyOne();
}

void MediaPlayerPrivateGStreamerBase::triggerRepaint(GstSample* sample)
{
    bool triggerResize;
    {
        WTF::GMutexLocker<GMutex> lock(m_sampleMutex);
        triggerResize = !m_sample;
        m_sample = sample;
    }

    if (triggerResize) {
        GST_DEBUG("First sample reached the sink, triggering video dimensions update");
        m_notifier->notify(MainThreadNotification::SizeChanged, [this] { m_player->sizeChanged(); });
    }

    if (!m_renderingCanBeAccelerated) {
        LockHolder locker(m_drawMutex);
        m_drawTimer.startOneShot(0_s);
        m_drawCondition.wait(m_drawMutex);
        return;
    }

#if USE(TEXTURE_MAPPER_GL)
#if USE(GSTREAMER_GL)
    pushTextureToCompositor();
#else
    {
        LockHolder lock(m_drawMutex);
        if (!m_platformLayerProxy->scheduleUpdateOnCompositorThread([this] { this->pushTextureToCompositor(); }))
            return;
        m_drawCondition.wait(m_drawMutex);
    }
#endif
#endif // USE(TEXTURE_MAPPER_GL)
}

#if !USE(HOLE_PUNCH_GSTREAMER)
void MediaPlayerPrivateGStreamerBase::repaintCallback(MediaPlayerPrivateGStreamerBase* player, GstSample* sample)
{
    player->triggerRepaint(sample);
}

void MediaPlayerPrivateGStreamerBase::repaintCancelledCallback(MediaPlayerPrivateGStreamerBase* player)
{
    player->cancelRepaint();
}
#endif

void MediaPlayerPrivateGStreamerBase::cancelRepaint()
{
    if (!m_renderingCanBeAccelerated) {
        m_drawTimer.stop();
        LockHolder locker(m_drawMutex);
        m_drawCondition.notifyOne();
    }
}

#if USE(GSTREAMER_GL)
GstFlowReturn MediaPlayerPrivateGStreamerBase::newSampleCallback(GstElement* sink, MediaPlayerPrivateGStreamerBase* player)
{
    GRefPtr<GstSample> sample = adoptGRef(gst_app_sink_pull_sample(GST_APP_SINK(sink)));
    player->triggerRepaint(sample.get());
    return GST_FLOW_OK;
}

GstFlowReturn MediaPlayerPrivateGStreamerBase::newPrerollCallback(GstElement* sink, MediaPlayerPrivateGStreamerBase* player)
{
    GRefPtr<GstSample> sample = adoptGRef(gst_app_sink_pull_preroll(GST_APP_SINK(sink)));
    player->triggerRepaint(sample.get());
    return GST_FLOW_OK;
}

void MediaPlayerPrivateGStreamerBase::clearCurrentBuffer()
{
    WTF::GMutexLocker<GMutex> lock(m_sampleMutex);

    // Replace by a new sample having only the caps, so this dummy sample is still useful to get the dimensions.
    // This prevents resizing problems when the video changes its quality and a DRAIN is performed.
    const GstStructure* info = gst_sample_get_info(m_sample.get());
    m_sample = adoptGRef(gst_sample_new(nullptr, gst_sample_get_caps(m_sample.get()),
        gst_sample_get_segment(m_sample.get()), info ? gst_structure_copy(info) : nullptr));

    {
        LockHolder locker(m_platformLayerProxy->lock());

        if (m_platformLayerProxy->isActive())
            m_platformLayerProxy->dropCurrentBufferWhilePreservingTexture();
    }
}
#endif

void MediaPlayerPrivateGStreamerBase::setSize(const IntSize& size)
{
    if (size == m_size)
        return;

    GST_INFO("Setting size to %dx%d", size.width(), size.height());
    m_size = size;

#if USE(HOLE_PUNCH_GSTREAMER)
    updateVideoRectangle();
#endif
}

void MediaPlayerPrivateGStreamerBase::setPosition(const IntPoint& position)
{
    if (position == m_position)
        return;

    m_position = position;

#if USE(HOLE_PUNCH_GSTREAMER)
    updateVideoRectangle();
#endif
}

#if USE(HOLE_PUNCH_GSTREAMER)
void MediaPlayerPrivateGStreamerBase::updateVideoRectangle()
{
    if (!m_pipeline)
        return;

    GRefPtr<GstElement> sinkElement;
    g_object_get(m_pipeline.get(), "video-sink", &sinkElement.outPtr(), nullptr);
    if (!sinkElement)
        return;

    GST_INFO("Setting video sink size and position to x:%d y:%d, width=%d, height=%d", m_position.x(), m_position.y(), m_size.width(), m_size.height());

    GUniquePtr<gchar> rectString(g_strdup_printf("%d,%d,%d,%d", m_position.x(), m_position.y(), m_size.width(),m_size.height()));
    g_object_set(sinkElement.get(), "rectangle", rectString.get(), nullptr);
}
#endif

void MediaPlayerPrivateGStreamerBase::paint(GraphicsContext& context, const FloatRect& rect)
{
    if (context.paintingDisabled())
        return;

    if (!m_player->visible())
        return;

    WTF::GMutexLocker<GMutex> lock(m_sampleMutex);
    if (!GST_IS_SAMPLE(m_sample.get()))
        return;

    ImagePaintingOptions paintingOptions(CompositeCopy);
    if (m_renderingCanBeAccelerated)
        paintingOptions.m_orientationDescription.setImageOrientationEnum(m_videoSourceOrientation);

    RefPtr<ImageGStreamer> gstImage = ImageGStreamer::createImage(m_sample.get());
    if (!gstImage)
        return;

    if (Image* image = reinterpret_cast<Image*>(gstImage->image()))
        context.drawImage(*image, rect, gstImage->rect(), paintingOptions);
}

#if USE(GSTREAMER_GL)
bool MediaPlayerPrivateGStreamerBase::copyVideoTextureToPlatformTexture(GraphicsContext3D* context, Platform3DObject outputTexture, GC3Denum outputTarget, GC3Dint level, GC3Denum internalFormat, GC3Denum format, GC3Denum type, bool premultiplyAlpha, bool flipY)
{
    UNUSED_PARAM(context);

    if (m_usingFallbackVideoSink)
        return false;

    if (premultiplyAlpha)
        return false;

    WTF::GMutexLocker<GMutex> lock(m_sampleMutex);

    GstVideoInfo videoInfo;
    if (!getSampleVideoInfo(m_sample.get(), videoInfo))
        return false;

    GstBuffer* buffer = gst_sample_get_buffer(m_sample.get());
    GstVideoFrame videoFrame;
    if (!gst_video_frame_map(&videoFrame, &videoInfo, buffer, static_cast<GstMapFlags>(GST_MAP_READ | GST_MAP_GL)))
        return false;

    IntSize size(GST_VIDEO_INFO_WIDTH(&videoInfo), GST_VIDEO_INFO_HEIGHT(&videoInfo));
    if (m_videoSourceOrientation.usesWidthAsHeight())
        size = size.transposedSize();
    unsigned textureID = *reinterpret_cast<unsigned*>(videoFrame.data[0]);

    if (!m_videoTextureCopier)
        m_videoTextureCopier = std::make_unique<VideoTextureCopierGStreamer>(TEXTURE_COPIER_COLOR_CONVERT_FLAG);

    bool copied = m_videoTextureCopier->copyVideoTextureToPlatformTexture(textureID, size, outputTexture, outputTarget, level, internalFormat, format, type, flipY, m_videoSourceOrientation);

    gst_video_frame_unmap(&videoFrame);

    return copied;
}

NativeImagePtr MediaPlayerPrivateGStreamerBase::nativeImageForCurrentTime()
{
#if USE(CAIRO) && ENABLE(ACCELERATED_2D_CANVAS)
    if (m_usingFallbackVideoSink)
        return nullptr;

    WTF::GMutexLocker<GMutex> lock(m_sampleMutex);

    GstVideoInfo videoInfo;
    if (!getSampleVideoInfo(m_sample.get(), videoInfo))
        return nullptr;

    GstBuffer* buffer = gst_sample_get_buffer(m_sample.get());
    GstVideoFrame videoFrame;
    if (!gst_video_frame_map(&videoFrame, &videoInfo, buffer, static_cast<GstMapFlags>(GST_MAP_READ | GST_MAP_GL)))
        return nullptr;

    IntSize size(GST_VIDEO_INFO_WIDTH(&videoInfo), GST_VIDEO_INFO_HEIGHT(&videoInfo));
    if (m_videoSourceOrientation.usesWidthAsHeight())
        size = size.transposedSize();

    GLContext* context = PlatformDisplay::sharedDisplayForCompositing().sharingGLContext();
    context->makeContextCurrent();

    if (!m_videoTextureCopier)
        m_videoTextureCopier = std::make_unique<VideoTextureCopierGStreamer>(TEXTURE_COPIER_COLOR_CONVERT_FLAG);

    unsigned textureID = *reinterpret_cast<unsigned*>(videoFrame.data[0]);
    bool copied = m_videoTextureCopier->copyVideoTextureToPlatformTexture(textureID, size, 0, GraphicsContext3D::TEXTURE_2D, 0, GraphicsContext3D::RGBA, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, false, m_videoSourceOrientation);
    gst_video_frame_unmap(&videoFrame);

    if (!copied)
        return nullptr;

    return adoptRef(cairo_gl_surface_create_for_texture(context->cairoDevice(), CAIRO_CONTENT_COLOR_ALPHA, m_videoTextureCopier->resultTexture(), size.width(), size.height()));
#else
    return nullptr;
#endif
}
#endif // USE(GSTREAMER_GL)

void MediaPlayerPrivateGStreamerBase::setVideoSourceOrientation(const ImageOrientation& orientation)
{
    if (m_videoSourceOrientation == orientation)
        return;

    m_videoSourceOrientation = orientation;
}

bool MediaPlayerPrivateGStreamerBase::supportsFullscreen() const
{
    return true;
}

PlatformMedia MediaPlayerPrivateGStreamerBase::platformMedia() const
{
    return NoPlatformMedia;
}

MediaPlayer::MovieLoadType MediaPlayerPrivateGStreamerBase::movieLoadType() const
{
    if (m_readyState == MediaPlayer::HaveNothing)
        return MediaPlayer::Unknown;

    if (isLiveStream())
        return MediaPlayer::LiveStream;

    return MediaPlayer::Download;
}

#if USE(GSTREAMER_GL)
GstElement* MediaPlayerPrivateGStreamerBase::createGLAppSink()
{
    if (!webkitGstCheckVersion(1, 8, 0))
        return nullptr;

    GstElement* appsink = gst_element_factory_make("appsink", "webkit-gl-video-sink");
    if (!appsink)
        return nullptr;

    g_object_set(appsink, "enable-last-sample", FALSE, "emit-signals", TRUE, "max-buffers", 1, nullptr);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(newSampleCallback), this);
    g_signal_connect(appsink, "new-preroll", G_CALLBACK(newPrerollCallback), this);

    GRefPtr<GstPad> pad = adoptGRef(gst_element_get_static_pad(appsink, "sink"));
    gst_pad_add_probe (pad.get(), GST_PAD_PROBE_TYPE_EVENT_FLUSH, [] (GstPad*, GstPadProbeInfo* info,  gpointer userData) -> GstPadProbeReturn {
        if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_EVENT (info)) != GST_EVENT_FLUSH_START)
            return GST_PAD_PROBE_OK;

        auto* player = static_cast<MediaPlayerPrivateGStreamerBase*>(userData);
        player->clearCurrentBuffer();
        return GST_PAD_PROBE_OK;
    }, this, nullptr);

    return appsink;
}

gboolean appSinkSinkQuery(GstPad* pad, GstObject* parent, GstQuery* query)
{
    gboolean result = FALSE;
    auto* player = static_cast<MediaPlayerPrivateGStreamerBase*>(g_object_get_data(G_OBJECT(parent), "player"));

    switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DRAIN: {
        player->clearCurrentBuffer();
        result = TRUE;
        break;
    }
    default:
        result = gst_pad_query_default(pad, parent, query);
        break;
    }

    return result;
}

GstElement* MediaPlayerPrivateGStreamerBase::createVideoSinkGL()
{
    // FIXME: Currently it's not possible to get the video frames and caps using this approach until
    // the pipeline gets into playing state. Due to this, trying to grab a frame and painting it by some
    // other mean (canvas or webgl) before playing state can result in a crash.
    // This is being handled in https://bugs.webkit.org/show_bug.cgi?id=159460.
    if (!webkitGstCheckVersion(1, 8, 0))
        return nullptr;

    gboolean result = TRUE;
    GstElement* videoSink = gst_bin_new("webkitvideosinkbin");
    GstElement* upload = gst_element_factory_make("glupload", nullptr);
    GstElement* colorconvert = gst_element_factory_make("glcolorconvert", nullptr);
    GstElement* appsink = createGLAppSink();

    if (!appsink || !upload || !colorconvert) {
        GST_WARNING("Failed to create GstGL elements");
        gst_object_unref(videoSink);

        if (upload)
            gst_object_unref(upload);
        if (colorconvert)
            gst_object_unref(colorconvert);
        if (appsink)
            gst_object_unref(appsink);

        return nullptr;
    }

    gst_bin_add_many(GST_BIN(videoSink), upload, colorconvert, appsink, nullptr);

    GRefPtr<GstCaps> caps = adoptGRef(gst_caps_from_string("video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), format = (string) " GST_GL_CAPS_FORMAT));

    result &= gst_element_link_pads(upload, "src", colorconvert, "sink");
    result &= gst_element_link_pads_filtered(colorconvert, "src", appsink, "sink", caps.get());

    GRefPtr<GstPad> pad = adoptGRef(gst_element_get_static_pad(upload, "sink"));
    gst_element_add_pad(videoSink, gst_ghost_pad_new("sink", pad.get()));

    pad = adoptGRef(gst_element_get_static_pad(appsink, "sink"));
    gst_pad_add_probe (pad.get(), GST_PAD_PROBE_TYPE_EVENT_FLUSH,
        [] (GstPad*, GstPadProbeInfo* info,  gpointer userData) -> GstPadProbeReturn {
            if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_EVENT (info)) != GST_EVENT_FLUSH_START)
                return GST_PAD_PROBE_OK;

            auto* player = static_cast<MediaPlayerPrivateGStreamerBase*>(userData);
            player->clearCurrentBuffer();
            return GST_PAD_PROBE_OK;
        }, this, nullptr);

    g_object_set_data(G_OBJECT(appsink), "player", (gpointer) this);
    gst_pad_set_query_function(pad.get(), appSinkSinkQuery);

    if (!result) {
        GST_WARNING("Failed to link GstGL elements");
        gst_object_unref(videoSink);
        videoSink = nullptr;
    }
    return videoSink;
}
#endif // USE(GSTREAMER_GL)

#if !USE(HOLE_PUNCH_GSTREAMER)
GstElement* MediaPlayerPrivateGStreamerBase::createVideoSink()
{
    acceleratedRenderingStateChanged();

#if USE(GSTREAMER_GL)
    if (m_renderingCanBeAccelerated)
        m_videoSink = createVideoSinkGL();
#endif

    if (!m_videoSink) {
        m_usingFallbackVideoSink = true;
        m_videoSink = webkitVideoSinkNew();
        m_repaintRequestedHandler = g_signal_connect_swapped(m_videoSink.get(), "repaint-requested", G_CALLBACK(repaintCallback), this);
        m_repaintCancelledHandler = g_signal_connect_swapped(m_videoSink.get(), "repaint-cancelled", G_CALLBACK(repaintCancelledCallback), this);
    }

    GstElement* videoSink = nullptr;
    m_fpsSink = gst_element_factory_make("fpsdisplaysink", "sink");
    if (m_fpsSink) {
        g_object_set(m_fpsSink.get(), "silent", TRUE , nullptr);

        // Turn off text overlay unless logging is enabled.
#if LOG_DISABLED
        g_object_set(m_fpsSink.get(), "text-overlay", FALSE , nullptr);
#else
        if (!isLogChannelEnabled("Media"))
            g_object_set(m_fpsSink.get(), "text-overlay", FALSE , nullptr);
#endif // LOG_DISABLED

        if (g_object_class_find_property(G_OBJECT_GET_CLASS(m_fpsSink.get()), "video-sink")) {
            g_object_set(m_fpsSink.get(), "video-sink", m_videoSink.get(), nullptr);
            videoSink = m_fpsSink.get();
        } else
            m_fpsSink = nullptr;
    }

    if (!m_fpsSink)
        videoSink = m_videoSink.get();

    ASSERT(videoSink);

    return videoSink;
}
#endif

void MediaPlayerPrivateGStreamerBase::setStreamVolumeElement(GstStreamVolume* volume)
{
    ASSERT(!m_volumeElement);
    m_volumeElement = volume;

    // We don't set the initial volume because we trust the sink to keep it for us. See
    // https://bugs.webkit.org/show_bug.cgi?id=118974 for more information.
    if (!m_player->platformVolumeConfigurationRequired()) {
        GST_DEBUG("Setting stream volume to %f", m_player->volume());
        setVolume(m_player->volume());
    } else
        GST_DEBUG("Not setting stream volume, trusting system one");

    GST_DEBUG("Setting stream muted %d",  m_player->muted());
    g_object_set(m_volumeElement.get(), "mute", m_player->muted(), nullptr);

    g_signal_connect_swapped(m_volumeElement.get(), "notify::volume", G_CALLBACK(volumeChangedCallback), this);
    g_signal_connect_swapped(m_volumeElement.get(), "notify::mute", G_CALLBACK(muteChangedCallback), this);
}

unsigned MediaPlayerPrivateGStreamerBase::decodedFrameCount() const
{
    guint64 decodedFrames = 0;
    if (m_fpsSink)
        g_object_get(m_fpsSink.get(), "frames-rendered", &decodedFrames, nullptr);
    return static_cast<unsigned>(decodedFrames);
}

unsigned MediaPlayerPrivateGStreamerBase::droppedFrameCount() const
{
    guint64 framesDropped = 0;
    if (m_fpsSink)
        g_object_get(m_fpsSink.get(), "frames-dropped", &framesDropped, nullptr);
    return static_cast<unsigned>(framesDropped);
}

unsigned MediaPlayerPrivateGStreamerBase::audioDecodedByteCount() const
{
    GstQuery* query = gst_query_new_position(GST_FORMAT_BYTES);
    gint64 position = 0;

    if (audioSink() && gst_element_query(audioSink(), query))
        gst_query_parse_position(query, 0, &position);

    gst_query_unref(query);
    return static_cast<unsigned>(position);
}

unsigned MediaPlayerPrivateGStreamerBase::videoDecodedByteCount() const
{
    GstQuery* query = gst_query_new_position(GST_FORMAT_BYTES);
    gint64 position = 0;

    if (gst_element_query(m_videoSink.get(), query))
        gst_query_parse_position(query, 0, &position);

    gst_query_unref(query);
    return static_cast<unsigned>(position);
}

#if USE(PLAYREADY)
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
WebCore::PlayreadySession* MediaPlayerPrivateGStreamerBase::createPlayreadySession(const Vector<uint8_t> &initDataVector, GstElement* pipeline, bool alreadyLocked)
{
    LockHolder locker(alreadyLocked ? nullptr : &m_prSessionsMutex);
    PlayreadySession* result;
    std::unique_ptr<PlayreadySession> uniquePrSession = std::make_unique<PlayreadySession>(initDataVector, pipeline);
    result = uniquePrSession.get();
    m_prSessions.append(std::move(uniquePrSession));
    return result;
}

WebCore::PlayreadySession* MediaPlayerPrivateGStreamerBase::prSessionByInitData(const Vector<uint8_t>& initData, bool alreadyLocked) const
{
    LockHolder locker(alreadyLocked ? nullptr : &m_prSessionsMutex);
    for (auto& prSession : m_prSessions)
        if (prSession->initData() == initData)
            return prSession.get();
    return nullptr;
}

WebCore::PlayreadySession* MediaPlayerPrivateGStreamerBase::prSessionBySessionId(const String& sessionId, bool alreadyLocked) const
{
    LockHolder locker(alreadyLocked ? nullptr : &m_prSessionsMutex);
    for (auto& prSession : m_prSessions)
        if (prSession->sessionId() == sessionId)
            return prSession.get();
    return nullptr;
}
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
PlayreadySession* MediaPlayerPrivateGStreamerBase::prSession() const
{
    PlayreadySession* session = nullptr;
    if (m_cdmSession) {
        CDMPRSessionGStreamer* cdmSession = static_cast<CDMPRSessionGStreamer*>(m_cdmSession);
        session = static_cast<PlayreadySession*>(cdmSession);
    }
    return session;
}
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
void MediaPlayerPrivateGStreamerBase::emitPlayReadySession(PlayreadySession* session)
{
    if (!session->ready())
        return;

    bool eventHandled = gst_element_send_event(m_pipeline.get(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
        gst_structure_new("playready-session", "session", G_TYPE_POINTER, session, nullptr)));
    GST_TRACE("emitted PR session on pipeline, event handled %s", eventHandled ? "yes" : "no");
}
#endif
#endif // USE(PLAYREADY)

#if USE(OPENCDM)
CDMSessionOpenCDM* MediaPlayerPrivateGStreamerBase::openCDMSession()
{
    if (!m_cdmSession)
        return nullptr;

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    return static_cast<CDMSessionOpenCDM*>(m_cdmSession);
#else
    return static_cast<CDMSessionOpenCDM*>(m_cdmSession.get());
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA)
}

void MediaPlayerPrivateGStreamerBase::emitOpenCDMSession()
{
    CDMSessionOpenCDM* cdmSession = openCDMSession();
    if (!cdmSession)
        return;

    const String& sessionId = cdmSession->sessionId();
    if (sessionId.isEmpty())
        return;

    bool eventHandled = gst_element_send_event(m_pipeline.get(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
        gst_structure_new("drm-session", "session", G_TYPE_STRING, sessionId.utf8().data(), nullptr)));
    GST_TRACE("emitted OpenCDM session on pipeline, event handled %s", eventHandled ? "yes" : "no");
}

void MediaPlayerPrivateGStreamerBase::resetOpenCDMSession()
{
    if (!m_cdmSession)
        return;
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    m_cdmSession = nullptr;
#else
    m_cdmSession.reset();
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA)
}
#endif // USE(OPENCDM)

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
MediaPlayer::MediaKeyException MediaPlayerPrivateGStreamerBase::addKey(const String& keySystem, const unsigned char* keyData, unsigned keyLength, const unsigned char* /* initData */, unsigned /* initDataLength */ , const String& sessionID)
{
    GST_DEBUG("addKey system: %s, length: %u, session: %s", keySystem.utf8().data(), keyLength, sessionID.utf8().data());

#if USE(PLAYREADY)
    if (equalIgnoringASCIICase(keySystem, PLAYREADY_PROTECTION_SYSTEM_ID)
        || equalIgnoringASCIICase(keySystem, PLAYREADY_YT_PROTECTION_SYSTEM_ID)) {
        RefPtr<Uint8Array> key = Uint8Array::create(keyData, keyLength);
        RefPtr<Uint8Array> nextMessage;
        unsigned short errorCode;
        uint32_t systemCode;
        PlayreadySession* prSession = prSessionBySessionId(sessionID);
        bool result = prSession->playreadyProcessKey(key.get(), nextMessage, errorCode, systemCode);

        if (errorCode || !result) {
            GST_ERROR("Error processing key: errorCode: %u, result: %d", errorCode, result);
            return MediaPlayer::InvalidPlayerState;
        }

        // XXX: use nextMessage here and send a new keyMessage is ack is needed?
        emitPlayReadySession(prSession);

        m_player->keyAdded(keySystem, sessionID);

        return MediaPlayer::NoError;
    }
#endif
#if USE(OPENCDM)
    if (CDMPrivateOpenCDM::supportsKeySystem(keySystem)) {
        RefPtr<Uint8Array> key = Uint8Array::create(keyData, keyLength);
        RefPtr<Uint8Array> nextMessage;
        unsigned short errorCode = 0;
        uint32_t systemCode;
        bool result = m_cdmSession->update(key.get(), nextMessage, errorCode, systemCode);
        if (errorCode || !result) {
            GST_ERROR("Error processing key: errorCode: %u, result: %d", errorCode, result);
            return MediaPlayer::InvalidPlayerState;
        }
        emitOpenCDMSession();
        m_player->keyAdded(keySystem, sessionID);
        return MediaPlayer::NoError;
    }
#endif
    if (equalIgnoringASCIICase(keySystem, "org.w3.clearkey")) {
        GstBuffer* buffer = gst_buffer_new_wrapped(g_memdup(keyData, keyLength), keyLength);
        dispatchDecryptionKey(buffer);
        gst_buffer_unref(buffer);

        m_player->keyAdded(keySystem, sessionID);

        return MediaPlayer::NoError;
    }

    return MediaPlayer::KeySystemNotSupported;
}

void MediaPlayerPrivateGStreamerBase::trimInitData(String keySystemUuid, const unsigned char*& initDataPtr, unsigned &initDataLength)
{
    if (initDataLength < 8 || keySystemUuid.length() < 16)
        return;

    // "pssh" box format (simplified) as described by ISO/IEC 14496-12:2012(E) and ISO/IEC 23001-7.
    // - Atom length (4 bytes).
    // - Atom name ("pssh", 4 bytes).
    // - Version (1 byte).
    // - Flags (3 bytes).
    // - Encryption system id (16 bytes).
    // - ...

    const unsigned char* chunkBase = initDataPtr;

    // Big/little-endian independent way to convert 4 bytes into a 32-bit word.
    uint32_t chunkSize = 0x1000000 * chunkBase[0] + 0x10000 * chunkBase[1] + 0x100 * chunkBase[2] + chunkBase[3];

    while (chunkBase + chunkSize < initDataPtr + initDataLength) {
        StringBuilder parsedKeySystemBuilder;
        for (unsigned char i = 0; i < 16; i++) {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                parsedKeySystemBuilder.append("-");
            parsedKeySystemBuilder.append(String::format("%02hhx", chunkBase[12+i]));
        }

        String parsedKeySystem = parsedKeySystemBuilder.toString();

        if (chunkBase[4] != 'p' || chunkBase[5] != 's' || chunkBase[6] != 's' || chunkBase[7] != 'h') {
            GST_DEBUG("pssh not found");
            return;
        }

        if (parsedKeySystem == keySystemUuid) {
            initDataPtr = chunkBase;
            initDataLength = chunkSize;
            return;
        }

        chunkBase += chunkSize;
        chunkSize = 0x1000000 * chunkBase[0] + 0x10000 * chunkBase[1] + 0x100 * chunkBase[2] + chunkBase[3];
    }

}

MediaPlayer::MediaKeyException MediaPlayerPrivateGStreamerBase::generateKeyRequest(const String& keySystem, const unsigned char* initDataPtr, unsigned initDataLength, const String& customData)
{
    receivedGenerateKeyRequest(keySystem);
    GST_DEBUG("generating key request for system: %s, init data size %u", keySystem.utf8().data(), initDataLength);
    GST_MEMDUMP("init data", initDataPtr, initDataLength);
#if USE(PLAYREADY)
    if (equalIgnoringASCIICase(keySystem, PLAYREADY_PROTECTION_SYSTEM_ID)
        || equalIgnoringASCIICase(keySystem, PLAYREADY_YT_PROTECTION_SYSTEM_ID)) {
        // For now we do not know if all protection systems should drop the pssh box, but during
        // testing of PR, we found that it is mandatory (found using the EME certification tests)
        // so for PR we remove the pssh and size, only ship the actual initdata.
        trimInitData(keySystemIdToUuid(keySystem.utf8().data()), initDataPtr, initDataLength);

        // At this point, the initData is comparable to the one reaching handleProtectionEvent(),
        // so it can be used to find prSession.
        Vector<uint8_t> initDataVector;
        initDataVector.append(reinterpret_cast<const uint8_t*>(initDataPtr), initDataLength);
        LockHolder prSessionsLocker(m_prSessionsMutex);
        PlayreadySession* prSession = prSessionByInitData(initDataVector, true);
        if (!prSession)
            GST_ERROR("prSession should already have been created when handling the protection events");
        prSessionsLocker.unlockEarly();

        LockHolder locker(prSession->mutex());
        if (prSession->ready()) {
            emitPlayReadySession(prSession);
            return MediaPlayer::NoError;
        }
        if (prSession->keyRequested()) {
            GST_DEBUG("previous key request already ongoing");
            return MediaPlayer::NoError;
        }

        // there can be only 1 pssh, so skip this one (fixed lemgth)
        // Data: <4 bytes total length><4 bytes FCC><4 bytes length ex this><Given Bytes in last length field><16 bytes GUID><4 bytes length ex this><Given Bytes in last length field>
        // https://www.w3.org/TR/2014/WD-encrypted-media-20140828/cenc-format.html
        unsigned int boxLength = 0;
        if (initDataPtr[4] == 'p' && initDataPtr[5] == 's' && initDataPtr[6] == 's' && initDataPtr[7] == 'h')  {
            boxLength = 4 + 4 + 16 +
                        4 + (((initDataPtr[8] << 24) | (initDataPtr[9] << 16) |  (initDataPtr[10] << 8) | (initDataPtr[11])) * 16) +
                        4;
        }
        GST_TRACE("current init data size %u, substracted %u", initDataLength, boxLength);
        GST_MEMDUMP ("init data", &(initDataPtr[boxLength]), (initDataLength - boxLength));

        unsigned short errorCode;
        uint32_t systemCode;
        RefPtr<Uint8Array> initData = Uint8Array::create(&(initDataPtr[boxLength]), initDataLength-boxLength);
        String destinationURL;
        RefPtr<Uint8Array> result = prSession->playreadyGenerateKeyRequest(initData.get(), customData, destinationURL, errorCode, systemCode);
        if (errorCode) {
            GST_ERROR("the key request wasn't properly generated");
            return MediaPlayer::InvalidPlayerState;
        }

        if (prSession->ready()) {
            emitPlayReadySession(prSession);
            return MediaPlayer::NoError;
        }
        URL url(URL(), destinationURL);
        GST_TRACE("playready generateKeyRequest result size %u, sessionId: %s", result->length(), prSession->sessionId().utf8().data());
        GST_MEMDUMP("result", result->data(), result->length());
        m_player->keyMessage(keySystem, prSession->sessionId(), result->data(), result->length(), url);
        return MediaPlayer::NoError;
    }
#endif
#if USE(OPENCDM)
    if (CDMPrivateOpenCDM::supportsKeySystem(keySystem)) {
        LockHolder locker(m_cdmSessionMutex);
        if (!m_cdmSession)
            m_cdmSession = CDMPrivateOpenCDM::createSession(nullptr, this);
        if (m_cdmSession->ready()) {
            emitOpenCDMSession();
            return MediaPlayer::NoError;
        }

        trimInitData(keySystemIdToUuid(keySystem.utf8().data()), initDataPtr, initDataLength);
        String mimeType;
        if (equalIgnoringASCIICase(keySystem, WIDEVINE_PROTECTION_SYSTEM_ID))
            mimeType = "video/mp4";
        else if (equalIgnoringASCIICase(keySystem, PLAYREADY_PROTECTION_SYSTEM_ID)
            || equalIgnoringASCIICase(keySystem, PLAYREADY_YT_PROTECTION_SYSTEM_ID))
            mimeType = "video/x-h264";

        unsigned short errorCode = 0;
        uint32_t systemCode;
        RefPtr<Uint8Array> initData = Uint8Array::create(initDataPtr, initDataLength);
        String destinationURL;
        RefPtr<Uint8Array> result = m_cdmSession->generateKeyRequest(mimeType, initData.get(), destinationURL, errorCode, systemCode);
        if (errorCode) {
            GST_ERROR("the key request wasn't properly generated");
            return MediaPlayer::InvalidPlayerState;
        }

        if (m_cdmSession->ready()) {
            emitOpenCDMSession();
            return MediaPlayer::NoError;
        }
        if (!result)
            return MediaPlayer::NoError;
        URL url(URL(), destinationURL);
        GST_TRACE("OpenCDM generateKeyRequest result size %u", result->length());
        GST_MEMDUMP("result", result->data(), result->length());
        m_player->keyMessage(keySystem, m_cdmSession->sessionId(), result->data(), result->length(), url);
        return MediaPlayer::NoError;
    }
#endif
    if (equalIgnoringASCIICase(keySystem, "org.w3.clearkey")) {
        trimInitData(keySystemIdToUuid(keySystem.utf8().data()), initDataPtr, initDataLength);
        GST_TRACE("current init data size %u", initDataLength);
        GST_MEMDUMP("init data", initDataPtr, initDataLength);
        m_player->keyMessage(keySystem, createCanonicalUUIDString(), initDataPtr, initDataLength, URL());
        return MediaPlayer::NoError;
    }
    return MediaPlayer::KeySystemNotSupported;
}

MediaPlayer::MediaKeyException MediaPlayerPrivateGStreamerBase::cancelKeyRequest(const String& /* keySystem */ , const String& /* sessionID */)
{
    GST_DEBUG("cancelKeyRequest");
    return MediaPlayer::KeySystemNotSupported;
}

void MediaPlayerPrivateGStreamerBase::needKey(const String& keySystem, const String& sessionId, const unsigned char* initData, unsigned initDataLength)
{
    if (!m_player->keyNeeded(keySystem, sessionId, initData, initDataLength))
        GST_DEBUG("no event handler for key needed");
}
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
void MediaPlayerPrivateGStreamerBase::needKey(RefPtr<Uint8Array> initData)
{
    if (!m_player->keyNeeded(initData.get()))
        GST_INFO("no event handler for key needed");
}

void MediaPlayerPrivateGStreamerBase::setCDMSession(CDMSession* session)
{
    GST_DEBUG("setting CDM session to %p", session);
    m_cdmSession = session;
}

void MediaPlayerPrivateGStreamerBase::keyAdded()
{
#if USE(PLAYREADY)
    // FIXME: This won't work properly when using more than one session at the same time.
    emitPlayReadySession(prSession());
#endif

#if USE(OPENCDM)
    if (m_cdmSession)
        emitOpenCDMSession();
#endif // USE(OPENCDM)
}

std::unique_ptr<CDMSession> MediaPlayerPrivateGStreamerBase::createSession(const String& keySystem, CDMSessionClient* client)
{
    if (!supportsKeySystem(keySystem, emptyString()))
        return nullptr;

    GST_DEBUG("creating key session for %s", keySystem.utf8().data());
#if USE(PLAYREADY)
    if (equalIgnoringASCIICase(keySystem, PLAYREADY_PROTECTION_SYSTEM_ID)
        || equalIgnoringASCIICase(keySystem, PLAYREADY_YT_PROTECTION_SYSTEM_ID))
        return std::make_unique<CDMPRSessionGStreamer>(client, this);
#endif

#if USE(OPENCDM)
    if (CDMPrivateOpenCDM::supportsKeySystem(keySystem))
        return CDMPrivateOpenCDM::createSession(client, this);
#endif // USE(OPENCDM)
    return nullptr;
}
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA)

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)
void MediaPlayerPrivateGStreamerBase::dispatchDecryptionKey(GstBuffer* buffer)
{
    gst_element_send_event(m_pipeline.get(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
        gst_structure_new("drm-cipher", "key", GST_TYPE_BUFFER, buffer, nullptr)));
}

void MediaPlayerPrivateGStreamerBase::handleProtectionEvent(GstEvent* event, GstElement* element)
{
#if (!USE(PLAYREADY) || !ENABLE(LEGACY_ENCRYPTED_MEDIA_V1))
    UNUSED_PARAM(element);
#endif

    const gchar* eventKeySystemId = nullptr;
    GstBuffer* data = nullptr;
    gst_event_parse_protection(event, &eventKeySystemId, &data, nullptr);

    GstMapInfo mapInfo;
    if (!gst_buffer_map(data, &mapInfo, GST_MAP_READ)) {
        GST_WARNING("cannot map %s protection data", eventKeySystemId);
        return;
    }

    GST_MEMDUMP("init datas", mapInfo.data, mapInfo.size);

    if (m_handledProtectionEvents.contains(GST_EVENT_SEQNUM(event))) {
        GST_DEBUG("event %u already handled", GST_EVENT_SEQNUM(event));
        m_handledProtectionEvents.remove(GST_EVENT_SEQNUM(event));
        return;
    }

#if USE(PLAYREADY)
    PlayreadySession* prSession = nullptr;
    if (webkit_media_playready_decrypt_is_playready_key_system_id(eventKeySystemId)) {
        Vector<uint8_t> initDataVector;
        initDataVector.append(reinterpret_cast<uint8_t*>(mapInfo.data), mapInfo.size);
        bool prSessionAlreadyExisted = false;

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
        LockHolder prSessionsLocker(m_prSessionsMutex);
        prSession = prSessionByInitData(initDataVector, true);
        if (prSession)
            prSessionAlreadyExisted = true;
        else
            prSession = createPlayreadySession(initDataVector, getPipeline(element), true);
        prSessionsLocker.unlockEarly();
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
        prSession = this->prSession();
        if (prSession)
            prSessionAlreadyExisted = true;
#endif

        if (prSessionAlreadyExisted) {
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
            LockHolder locker(prSession->mutex());
#endif
            if (prSession->keyRequested() || prSession->ready()) {
                if (prSession->ready())
                    emitPlayReadySession(prSession);
                return;
            }
        }
    }
#endif // USE(PLAYREADY)

#if USE(OPENCDM)
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
    LockHolder locker(m_cdmSessionMutex);
#endif
    if (m_cdmSession && (m_cdmSession->keyRequested() || m_cdmSession->ready())) {
        GST_DEBUG("ocdm key requested already");
        if (m_cdmSession->ready()) {
            GST_DEBUG("ocdm key already negotiated");
            emitOpenCDMSession();
        }
        return;
    }
#endif

    GST_DEBUG("scheduling keyNeeded event for %s with init data size of %u", eventKeySystemId, mapInfo.size);
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
#if USE(PLAYREADY)
    String sessionId = prSession->sessionId();
#else
    String sessionId = createCanonicalUUIDString();
#endif
    needKey(keySystemUuidToId(eventKeySystemId), sessionId, mapInfo.data, mapInfo.size);
#elif ENABLE(LEGACY_ENCRYPTED_MEDIA)
    RefPtr<Uint8Array> initDataArray = Uint8Array::create(mapInfo.data, mapInfo.size);
    needKey(initDataArray);
#else
    ASSERT_NOT_REACHED();
#endif
    gst_buffer_unmap(data, &mapInfo);
}

void MediaPlayerPrivateGStreamerBase::receivedGenerateKeyRequest(const String& keySystem)
{
    GST_DEBUG("received generate key request for %s", keySystem.utf8().data());
    m_lastGenerateKeyRequestKeySystemUuid = keySystemIdToUuid(keySystem.utf8().data());
    m_protectionCondition.notifyOne();
}

void MediaPlayerPrivateGStreamerBase::abortEncryptionSetup()
{
    GST_DEBUG("aborting wait for generateKeyRequest() during encryption setup");
    this->m_lastGenerateKeyRequestKeySystemUuid = ABORT_ENCRYPTION_SETUP_PROTECTION_SYSTEM_UUID;
    m_protectionCondition.notifyOne();
}

static const char* keySystemIdToUuid(const char* id)
{
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
    if (!g_ascii_strcasecmp(id, GStreamerEMEUtilities::s_ClearKeyKeySystem))
        return GStreamerEMEUtilities::s_ClearKeyUUID;
#endif

#if USE(PLAYREADY) || USE(OPENCDM)
    if (!g_ascii_strcasecmp(id, GStreamerEMEUtilities::s_PlayReadyKeySystems[0])
        || !g_ascii_strcasecmp(id, GStreamerEMEUtilities::s_PlayReadyKeySystems[1]))
        return GStreamerEMEUtilities::s_PlayReadyUUID;
#endif

#if USE(OPENCDM)
    if (!g_ascii_strcasecmp(id, GStreamerEMEUtilities::s_WidevineKeySystem))
        return GStreamerEMEUtilities::s_WidevineUUID;
#endif

    return { };
}
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
static const char* keySystemUuidToId(const char* uuid)
{
    if (!g_ascii_strcasecmp(uuid, GStreamerEMEUtilities::s_ClearKeyUUID))
        return GStreamerEMEUtilities::s_ClearKeyKeySystem;

#if USE(PLAYREADY) || USE(OPENCDM)
    if (!g_ascii_strcasecmp(uuid, GStreamerEMEUtilities::s_PlayReadyUUID))
        return GStreamerEMEUtilities::s_PlayReadyKeySystems[0];
#endif

#if USE(OPENCDM)
    if (!g_ascii_strcasecmp(uuid, GStreamerEMEUtilities::s_WidevineUUID))
        return GStreamerEMEUtilities::s_WidevineKeySystem;
#endif

    return { };
}
#endif

#if ENABLE(ENCRYPTED_MEDIA)
void MediaPlayerPrivateGStreamerBase::cdmInstanceAttached(const CDMInstance& instance)
{
    ASSERT(!m_cdmInstance);
    m_cdmInstance = &instance;
    GST_DEBUG("CDM instance %p set", m_cdmInstance.get());
    m_protectionCondition.notifyAll();
}

void MediaPlayerPrivateGStreamerBase::cdmInstanceDetached(const CDMInstance& instance)
{
#ifdef NDEBUG
    UNUSED_PARAM(instance);
#endif
    ASSERT(m_cdmInstance.get() == &instance);
    GST_DEBUG("detaching CDM instance %p", m_cdmInstance.get());
    m_cdmInstance = nullptr;
    m_protectionCondition.notifyAll();
}

void MediaPlayerPrivateGStreamerBase::attemptToDecryptWithInstance(const CDMInstance& instance)
{
    ASSERT(m_cdmInstance.get() == &instance);
    GST_TRACE("instance %p, current stored %p", &instance, m_cdmInstance.get());
    attemptToDecryptWithLocalInstance();
}

void MediaPlayerPrivateGStreamerBase::attemptToDecryptWithLocalInstance()
{
#if USE(OPENCDM)
    if (is<CDMInstanceOpenCDM>(*m_cdmInstance)) {
        GST_DEBUG("handling OpenCDM %s keys", m_cdmInstance->keySystem().utf8().data());
        auto& cdmInstanceOpenCDM = downcast<CDMInstanceOpenCDM>(*m_cdmInstance);
        String sessionId = cdmInstanceOpenCDM.getCurrentSessionId();
        ASSERT(!sessionId.isEmpty());
        dispatchDecryptionSession(sessionId);
    }
#endif
}

void MediaPlayerPrivateGStreamerBase::dispatchDecryptionKey(GstBuffer* buffer)
{
    bool eventHandled = gst_element_send_event(m_pipeline.get(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
        gst_structure_new("drm-cipher", "key", GST_TYPE_BUFFER, buffer, nullptr)));
    m_needToResendCredentials = m_handledProtectionEvents.size() > 0;
    GST_TRACE("emitted decryption cipher key on pipeline, event handled %s, need to resend credentials %s", boolForPrinting(eventHandled), boolForPrinting(m_needToResendCredentials));
}

void MediaPlayerPrivateGStreamerBase::dispatchDecryptionSession(const String& sessionId)
{
    bool eventHandled = gst_element_send_event(m_pipeline.get(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
        gst_structure_new("drm-session", "session", G_TYPE_STRING, sessionId.utf8().data(), nullptr)));
    m_needToResendCredentials = m_handledProtectionEvents.size() > 0;
    GST_TRACE("emitted decryption session %s on pipeline, event handled %s, need to resend credentials %s", sessionId.utf8().data(), boolForPrinting(eventHandled), boolForPrinting(m_needToResendCredentials));
}

void MediaPlayerPrivateGStreamerBase::handleProtectionEvent(GstEvent* event)
{
    if (m_handledProtectionEvents.contains(GST_EVENT_SEQNUM(event))) {
        GST_DEBUG("event %u already handled", GST_EVENT_SEQNUM(event));
        m_handledProtectionEvents.remove(GST_EVENT_SEQNUM(event));
        if (m_needToResendCredentials) {
            GST_DEBUG("resending credentials");
            attemptToDecryptWithLocalInstance();
        }
        return;
    }

    const gchar* eventKeySystemId = nullptr;
    gst_event_parse_protection(event, &eventKeySystemId, nullptr, nullptr);
    GST_WARNING("FIXME: unhandled protection event for %s", eventKeySystemId);
    ASSERT_NOT_REACHED();
}
#endif

bool MediaPlayerPrivateGStreamerBase::supportsKeySystem(const String& keySystem, const String& mimeType)
{
    bool result = false;

#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
    if (equalIgnoringASCIICase(keySystem, "org.w3.clearkey"))
        result = true;
#endif

#if USE(PLAYREADY) && (ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA))
    if (equalIgnoringASCIICase(keySystem, PLAYREADY_PROTECTION_SYSTEM_ID)
        || equalIgnoringASCIICase(keySystem, PLAYREADY_YT_PROTECTION_SYSTEM_ID)) {
        result = true;
    }
#endif

#if (ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA)) && USE(OPENCDM)
    if (CDMPrivateOpenCDM::supportsKeySystemAndMimeType(keySystem, mimeType))
        result = true;
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    result = GStreamerEMEUtilities::isClearKeyKeySystem(keySystem);
#endif

    GST_DEBUG("checking for KeySystem support with %s and type %s: %s", keySystem.utf8().data(), mimeType.utf8().data(), boolForPrinting(result));
    return result;
}

MediaPlayer::SupportsType MediaPlayerPrivateGStreamerBase::extendedSupportsType(const MediaEngineSupportParameters& parameters, MediaPlayer::SupportsType result)
{
#if ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)
    // From: <http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html#dom-canplaytype>
    // In addition to the steps in the current specification, this method must run the following steps:

    // 1. Check whether the Key System is supported with the specified container and codec type(s) by following the steps for the first matching condition from the following list:
    //    If keySystem is null, continue to the next step.
    if (parameters.keySystem.isNull() || parameters.keySystem.isEmpty())
        return result;

    // If keySystem contains an unrecognized or unsupported Key System, return the empty string
    if (!supportsKeySystem(parameters.keySystem, String::format("%s; codecs=\"%s\"", parameters.type.containerType().utf8().data(), parameters.type.parameter(ContentType::codecsParameter()).utf8().data())))
        result = MediaPlayer::IsNotSupported;
#else
    UNUSED_PARAM(parameters);
#endif
    return result;
}

}

#endif // USE(GSTREAMER)
