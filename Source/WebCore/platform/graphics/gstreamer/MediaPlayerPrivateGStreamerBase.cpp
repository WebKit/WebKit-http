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

#include "GStreamerCommon.h"
#include "GraphicsContext.h"
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
#include "SharedBuffer.h"
#if USE(OPENCDM)
#include "CDMOpenCDM.h"
#include "WebKitOpenCDMDecryptorGStreamer.h"
#else
#include "WebKitClearKeyDecryptorGStreamer.h"
#endif
#endif

#if USE(GSTREAMER_GL)
#if PLATFORM(WPE)
#define GST_GL_CAPS_FORMAT "{ RGBA }"
#define TEXTURE_MAPPER_COLOR_CONVERT_FLAG static_cast<TextureMapperGL::Flag>(0);
#define TEXTURE_COPIER_COLOR_CONVERT_FLAG VideoTextureCopierGStreamer::ColorConversion::AlreadyRGBA
#elif G_BYTE_ORDER == G_LITTLE_ENDIAN)
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


#if USE(CAIRO) && ENABLE(ACCELERATED_2D_CANVAS)
#include <cairo-gl.h>
#endif

GST_DEBUG_CATEGORY(webkit_media_player_debug);
#define GST_CAT_DEFAULT webkit_media_player_debug

using namespace std;

namespace WebCore {

#if ENABLE(NATIVE_AUDIO)
static const GstStreamVolumeFormat volumeFormat = GST_STREAM_VOLUME_FORMAT_LINEAR;
#else
static const GstStreamVolumeFormat volumeFormat = GST_STREAM_VOLUME_FORMAT_CUBIC;
#endif

void MediaPlayerPrivateGStreamerBase::ensureWebKitGStreamerElements()
{
#if USE(GSTREAMER_WEBKIT_HTTP_SRC)
    GRefPtr<GstElementFactory> srcFactory = adoptGRef(gst_element_factory_find("webkitwebsrc"));
    if (!srcFactory)
        gst_element_register(0, "webkitwebsrc", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_WEB_SRC);
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    if (!webkitGstCheckVersion(1, 6, 1))
        return;

#if USE(OPENCDM)
    GRefPtr<GstElementFactory> decryptorFactory = adoptGRef(gst_element_factory_find("webkitopencdm"));
    if (!decryptorFactory)
        gst_element_register(0, "webkitopencdm", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_OPENCDM_DECRYPT);

#else
    GRefPtr<GstElementFactory> clearKeyDecryptorFactory = adoptGRef(gst_element_factory_find("webkitclearkey"));
    if (!clearKeyDecryptorFactory)
        gst_element_register(nullptr, "webkitclearkey", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_MEDIA_CK_DECRYPT);

#endif
#endif
}

bool MediaPlayerPrivateGStreamerBase::initializeGStreamer()
{
    if (!WebCore::initializeGStreamer())
        return false;

    static bool gstDebugEnabled = false;
    if (!gstDebugEnabled) {
        GST_DEBUG_CATEGORY_INIT(webkit_media_player_debug, "webkitmediaplayer", 0, "WebKit media player");
        gstDebugEnabled = true;
    }

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
        GstVideoMeta* meta = gst_buffer_get_video_meta(buffer);

        // If info and meta dimensions aren't the same, gst_video_frame_map() shows a CRITICAL error.
        // This can happen on prerolling (eg: on PLAYING->PAUSED) after a quality change, because the first frame (old quality) is returned.
        if (UNLIKELY(!(videoInfo.width == int(meta->width) && videoInfo.height == int(meta->height)
                    && gst_video_frame_map(&m_videoFrame, &videoInfo, buffer, static_cast<GstMapFlags>(GST_MAP_READ | GST_MAP_GL)))))
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
    m_platformLayerProxy->pushNextBuffer(std::make_unique<TextureMapperPlatformLayerBuffer>(0, m_size, TextureMapperGL::ShouldOverwriteRect, GL_DONT_CARE));
#endif
#endif
}

MediaPlayerPrivateGStreamerBase::~MediaPlayerPrivateGStreamerBase()
{
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

bool MediaPlayerPrivateGStreamerBase::handleSyncMessage(GstMessage* message)
{
    UNUSED_PARAM(message);
    if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_NEED_CONTEXT)
        return false;

    const gchar* contextType;
    gst_message_parse_context_type(message, &contextType);
    GST_DEBUG("Handling %s need-context message for %s", contextType, GST_MESSAGE_SRC_NAME(message));

#if USE(GSTREAMER_GL)
    GRefPtr<GstContext> elementContext = adoptGRef(requestGLContext(contextType));
    if (elementContext) {
        gst_element_set_context(GST_ELEMENT(message->src), elementContext.get());
        return true;
    }
#endif // USE(GSTREAMER_GL)

    return false;
}

#if USE(GSTREAMER_GL)
GstContext* MediaPlayerPrivateGStreamerBase::requestGLContext(const char* contextType)
{
    if (!ensureGstGLContext())
        return nullptr;

    if (!g_strcmp0(contextType, GST_GL_DISPLAY_CONTEXT_TYPE)) {
        GstContext* displayContext = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
        gst_context_set_gl_display(displayContext, gstGLDisplay());
        return displayContext;
    }

    if (!g_strcmp0(contextType, "gst.gl.app_context")) {
        GstContext* appContext = gst_context_new("gst.gl.app_context", TRUE);
        GstStructure* structure = gst_context_writable_structure(appContext);
#if GST_CHECK_VERSION(1, 11, 0)
        gst_structure_set(structure, "context", GST_TYPE_GL_CONTEXT, gstGLContext(), nullptr);
#else
        gst_structure_set(structure, "context", GST_GL_TYPE_CONTEXT, gstGLContext(), nullptr);
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
    if (GST_IS_SAMPLE(m_sample.get())) {
	// If there's a sample, get the caps from it.
        caps = gst_sample_get_caps(m_sample.get());
    } else {
        // If there's no sample, try to get the caps from the video sink.
        GRefPtr<GstElement> videoSink = m_videoSink;

        // m_videoSink can be null for auto-plugged sinks, but we can ask PlayBin.
        if (!videoSink)
          g_object_get(m_pipeline.get(), "video-sink", &videoSink.outPtr(), nullptr);
        if (!videoSink)
          return { };

        GRefPtr<GstPad> videoSinkPad = adoptGRef(gst_element_get_static_pad(videoSink.get(), "sink"));
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
#if USE(HOLE_PUNCH_GSTREAMER) && USE(COORDINATED_GRAPHICS_THREADED)
    LockHolder locker(m_platformLayerProxy->lock());
    m_platformLayerProxy->pushNextBuffer(std::make_unique<TextureMapperPlatformLayerBuffer>(0, m_size, TextureMapperGL::ShouldOverwriteRect, GL_DONT_CARE));
#endif
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
    std::unique_ptr<TextureMapperPlatformLayerBuffer> buffer = m_platformLayerProxy->getAvailableBuffer(size, GL_DONT_CARE);
    if (UNLIKELY(!buffer)) {
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
    GST_DEBUG("Flushing video sample");
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

void MediaPlayerPrivateGStreamerBase::ensureGLVideoSinkContext()
{
    if (!m_glDisplayElementContext)
        m_glDisplayElementContext = adoptGRef(requestGLContext(GST_GL_DISPLAY_CONTEXT_TYPE));

    if (m_glDisplayElementContext)
        gst_element_set_context(m_videoSink.get(), m_glDisplayElementContext.get());

    if (!m_glAppElementContext)
        m_glAppElementContext = adoptGRef(requestGLContext("gst.gl.app_context"));

    if (m_glAppElementContext)
        gst_element_set_context(m_videoSink.get(), m_glAppElementContext.get());
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

#if ENABLE(ENCRYPTED_MEDIA)
void MediaPlayerPrivateGStreamerBase::handleProtectionStructure(const GstStructure* structure)
{
    ASSERT(isMainThread());

    // FIXME: Inform that we are waiting for a key.
    GUniqueOutPtr<char> keySystemUUID;
    GRefPtr<GstBuffer> data;
    gst_structure_get(structure, "init-data", GST_TYPE_BUFFER, &data.outPtr(), "key-system-uuid", G_TYPE_STRING, &keySystemUUID.outPtr(), nullptr);

    GstMappedBuffer mappedInitData(data.get(), GST_MAP_READ);
    if (!mappedInitData) {
        GST_WARNING("cannot map protection data");
        return;
    }

    String keySystemUUIDString = keySystemUUID ? keySystemUUID.get() : "(unspecified)";
    InitData initData(mappedInitData.data(), mappedInitData.size());
    GST_TRACE("init data encountered for %s of size %" G_GSIZE_FORMAT " with MD5 %s", keySystemUUIDString.utf8().data(), mappedInitData.size(), GStreamerEMEUtilities::initDataMD5(initData).utf8().data());
    GST_MEMDUMP("init data", mappedInitData.data(), mappedInitData.size());

    RunLoop::main().dispatch([weakThis = m_weakPtrFactory.createWeakPtr(*this), keySystemUUID = keySystemUUIDString, initData] {
        if (!weakThis)
            return;

        GST_DEBUG("scheduling initializationDataEncountered event for %s with init data size of %" G_GSIZE_FORMAT, keySystemUUID.utf8().data(), initData.sizeInBytes());
        GST_TRACE("init data MD5 %s", GStreamerEMEUtilities::initDataMD5(initData).utf8().data());
        GST_MEMDUMP("init datas", reinterpret_cast<const uint8_t*>(initData.characters8()), initData.sizeInBytes());
        weakThis->m_player->initializationDataEncountered(ASCIILiteral("cenc"), ArrayBuffer::create(reinterpret_cast<const uint8_t*>(initData.characters8()), initData.sizeInBytes()));
    });
}

void MediaPlayerPrivateGStreamerBase::cdmInstanceAttached(const CDMInstance& instance)
{
    ASSERT(isMainThread());
    GST_DEBUG("CDM instance %p set", &instance);
    m_cdmInstance = &instance;
    dispatchLocalCDMInstance();
}

void MediaPlayerPrivateGStreamerBase::dispatchLocalCDMInstance()
{
    ASSERT(isMainThread());

    if (!m_cdmInstance) {
        GST_DEBUG("no CDM instance yet, not dispatching anything");
        return;
    }

    GST_DEBUG("CDM instance %p dispatched", m_cdmInstance.get());
    dispatchDecryptionStructure(GUniquePtr<GstStructure>(gst_structure_new("drm-cdm-instance-attached", "cdm-instance", G_TYPE_POINTER, m_cdmInstance.get(), nullptr)));
}

void MediaPlayerPrivateGStreamerBase::cdmInstanceDetached(const CDMInstance& instance)
{
    ASSERT(isMainThread());
    ASSERT(m_cdmInstance.get() == &instance);
    GST_DEBUG("detaching CDM instance %p, dispatching event", m_cdmInstance.get());
    m_cdmInstance = nullptr;
    dispatchDecryptionStructure(GUniquePtr<GstStructure>(gst_structure_new("drm-cdm-instance-detached", "cdm-instance", G_TYPE_POINTER, &instance, nullptr)));
}

void MediaPlayerPrivateGStreamerBase::attemptToDecryptWithInstance(const CDMInstance& instance)
{
    ASSERT(isMainThread());
    ASSERT(m_cdmInstance.get() == &instance);
    GST_TRACE("instance %p, current stored %p", &instance, m_cdmInstance.get());
    attemptToDecryptWithLocalInstance();
}

void MediaPlayerPrivateGStreamerBase::attemptToDecryptWithLocalInstance()
{
    GST_TRACE("dispatching attempt to decrypt with local instance");
    dispatchDecryptionStructure(GUniquePtr<GstStructure>(gst_structure_new_empty("drm-attempt-to-decrypt-with-local-instance")));
}

void MediaPlayerPrivateGStreamerBase::dispatchDecryptionKey(GstBuffer* buffer)
{
    bool eventHandled = gst_element_send_event(m_pipeline.get(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB,
        gst_structure_new("drm-cipher", "key", GST_TYPE_BUFFER, buffer, nullptr)));
    GST_TRACE("emitted decryption cipher key on pipeline, event handled %s, need to resend credentials %s", boolForPrinting(eventHandled));
}

void MediaPlayerPrivateGStreamerBase::dispatchDecryptionStructure(GUniquePtr<GstStructure>&& structure)
{
    bool eventHandled = gst_element_send_event(m_pipeline.get(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, structure.release()));
    GST_TRACE("emitted decryption structure on pipeline, event handled %s", boolForPrinting(eventHandled));
}
#endif

bool MediaPlayerPrivateGStreamerBase::supportsKeySystem(const String& keySystem, const String& mimeType)
{
    bool result = false;

#if ENABLE(ENCRYPTED_MEDIA) && !USE(OPENCDM)
    result = GStreamerEMEUtilities::isClearKeyKeySystem(keySystem);
#endif

    GST_DEBUG("checking for KeySystem support with %s and type %s: %s", keySystem.utf8().data(), mimeType.utf8().data(), boolForPrinting(result));
    return result;
}

MediaPlayer::SupportsType MediaPlayerPrivateGStreamerBase::extendedSupportsType(const MediaEngineSupportParameters& parameters, MediaPlayer::SupportsType result)
{
#if ENABLE(ENCRYPTED_MEDIA)
    if (result != MediaPlayer::IsNotSupported) {
        String cryptoblockformat = parameters.type.parameter("cryptoblockformat");
        if (!cryptoblockformat.isEmpty() && cryptoblockformat != "subsample")
            result = MediaPlayer::IsNotSupported;
    }
#else
    UNUSED_PARAM(parameters);
#endif
    return result;
}

}

#endif // USE(GSTREAMER)
