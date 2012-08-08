/*
 * Copyright (C) 2012 Igalia, S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef GStreamerVersioning_h
#define GStreamerVersioning_h

#include <gst/gst.h>
#include <gst/video/video.h>

namespace WebCore {
class IntSize;
};

void webkitGstObjectRefSink(GstObject*);
GstCaps* webkitGstGetPadCaps(GstPad*);
bool getVideoSizeAndFormatFromCaps(GstCaps*, WebCore::IntSize&, GstVideoFormat&, int& pixelAspectRatioNumerator, int& pixelAspectRatioDenominator, int& stride);
GstBuffer* createGstBuffer(GstBuffer*);
void setGstElementClassMetadata(GstElementClass*, const char* name, const char* longName, const char* description, const char* author);
bool gstObjectIsFloating(GstObject*);
void notifyGstTagsOnPad(GstElement*, GstPad*, GstTagList*);
#endif // GStreamerVersioning_h
