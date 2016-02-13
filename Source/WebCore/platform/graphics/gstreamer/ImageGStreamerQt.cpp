/*
    Copyright (C) 2010 Igalia S.L
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#include "config.h"
#include "ImageGStreamer.h"

#if ENABLE(VIDEO) && USE(GSTREAMER)

#include "GStreamerUtilities.h"

#include <gst/gst.h>
#include <gst/video/gstvideometa.h>


using namespace std;
using namespace WebCore;

ImageGStreamer::ImageGStreamer(GstSample* sample)
{
    GstCaps* caps = gst_sample_get_caps(sample);
    GstVideoInfo videoInfo;
    gst_video_info_init(&videoInfo);
    if (!gst_video_info_from_caps(&videoInfo, caps))
        return;
    // FIXME: Check for https://bugreports.qt.io/browse/QTBUG-44245

    // Right now the TextureMapper only supports chromas with one plane
    ASSERT(GST_VIDEO_INFO_N_PLANES(&videoInfo) == 1);

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!gst_video_frame_map(&m_videoFrame, &videoInfo, buffer, GST_MAP_READ))
        return;

    unsigned char* bufferData = reinterpret_cast<unsigned char*>(GST_VIDEO_FRAME_PLANE_DATA(&m_videoFrame, 0));

    QImage::Format imageFormat;
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    imageFormat = (GST_VIDEO_FRAME_FORMAT(&m_videoFrame) == GST_VIDEO_FORMAT_BGRA) ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
#else
    imageFormat = (GST_VIDEO_FRAME_FORMAT(&m_videoFrame) == GST_VIDEO_FORMAT_ARGB) ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
#endif

    int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&m_videoFrame, 0);
    int width = GST_VIDEO_FRAME_WIDTH(&m_videoFrame);
    int height = GST_VIDEO_FRAME_HEIGHT(&m_videoFrame);

    // FIXME: ask about stride
    QImage image(bufferData, width, height, stride, imageFormat);

    QPixmap *surface = new QPixmap(QPixmap::fromImage(qMove(image), Qt::NoFormatConversion));
    m_image = BitmapImage::create(surface);

    if (GstVideoCropMeta* cropMeta = gst_buffer_get_video_crop_meta(buffer))
        setCropRect(FloatRect(cropMeta->x, cropMeta->y, cropMeta->width, cropMeta->height));
}

ImageGStreamer::~ImageGStreamer()
{
    if (m_image)
        m_image = nullptr;

    // We keep the buffer memory mapped until the image is destroyed because the internal
    // QImage/QPixmap was created using the buffer data directly.
    gst_video_frame_unmap(&m_videoFrame);
}
#endif // USE(GSTREAMER)
