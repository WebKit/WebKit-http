/*
 *  Copyright (C) 2009, 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *  Copyright (C) 2013 Collabora Ltd.
 *  Copyright (C) 2013 Orange
 *  Copyright (C) 2014 Sebastian Dröge <sebastian@centricular.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef WebKitMediaSourceGStreamer_h
#define WebKitMediaSourceGStreamer_h
#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)

#include "MediaPlayer.h"
#include "MediaSource.h"
#include "SourceBufferPrivateClient.h"
#include "SourceBufferPrivate.h"

#include "GRefPtrGStreamer.h"

#include <gst/gst.h>

namespace WebCore {

class MediaPlayerPrivateGStreamerMSE;

enum MediaSourceStreamTypeGStreamer { Invalid, Unknown, Audio, Video, Text };

}

G_BEGIN_DECLS

#define WEBKIT_TYPE_MEDIA_SRC            (webkit_media_src_get_type ())
#define WEBKIT_MEDIA_SRC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBKIT_TYPE_MEDIA_SRC, WebKitMediaSrc))
#define WEBKIT_MEDIA_SRC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WEBKIT_TYPE_MEDIA_SRC, WebKitMediaSrcClass))
#define WEBKIT_IS_MEDIA_SRC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBKIT_TYPE_MEDIA_SRC))
#define WEBKIT_IS_MEDIA_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WEBKIT_TYPE_MEDIA_SRC))

typedef struct _WebKitMediaSrc        WebKitMediaSrc;
typedef struct _WebKitMediaSrcClass   WebKitMediaSrcClass;
typedef struct _WebKitMediaSrcPrivate WebKitMediaSrcPrivate;

struct _WebKitMediaSrc {
    GstBin parent;

    WebKitMediaSrcPrivate *priv;
};

struct _WebKitMediaSrcClass {
    GstBinClass parentClass;

    /* notify app that number of audio/video/text streams changed */
    void (*video_changed) (WebKitMediaSrc* webKitMediaSrc);
    void (*audio_changed) (WebKitMediaSrc* webKitMediaSrc);
    void (*text_changed) (WebKitMediaSrc* webKitMediaSrc);
};

GType webkit_media_src_get_type(void);

void webkit_media_src_set_mediaplayerprivate(WebKitMediaSrc* src, WebCore::MediaPlayerPrivateGStreamerMSE* player);

void webkit_media_src_prepare_seek(WebKitMediaSrc*, const MediaTime&);

G_END_DECLS

namespace WTF {
template<> GRefPtr<WebKitMediaSrc> adoptGRef(WebKitMediaSrc* ptr);
template<> WebKitMediaSrc* refGPtr<WebKitMediaSrc>(WebKitMediaSrc* ptr);
template<> void derefGPtr<WebKitMediaSrc>(WebKitMediaSrc* ptr);
};

namespace WebCore {

class ContentType;
class SourceBufferPrivateGStreamer;
class MediaSourceGStreamer;

class PlaybackPipeline: public RefCounted<PlaybackPipeline> {
    public:
        static PassRefPtr<PlaybackPipeline> create();
        virtual ~PlaybackPipeline();

        void setWebKitMediaSrc(WebKitMediaSrc*);
        WebKitMediaSrc* webKitMediaSrc();

        MediaSourcePrivate::AddStatus addSourceBuffer(RefPtr<SourceBufferPrivateGStreamer>);
        void removeSourceBuffer(RefPtr<SourceBufferPrivateGStreamer>);
        void attachTrack(RefPtr<SourceBufferPrivateGStreamer>, RefPtr<TrackPrivateBase>, GstCaps*);
        void reattachTrack(RefPtr<SourceBufferPrivateGStreamer>, RefPtr<TrackPrivateBase>, GstCaps*);
        void notifyDurationChanged();

        // From MediaSourceGStreamer
        void markEndOfStream(MediaSourcePrivate::EndOfStreamStatus);

        // From SourceBufferPrivateGStreamer
        void flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample> >);
        void enqueueSample(PassRefPtr<MediaSample>);

        GstElement* pipeline();
    private:
        PlaybackPipeline();
        GRefPtr<WebKitMediaSrc> m_webKitMediaSrc;
};

};

#endif // USE(GSTREAMER)
#endif
