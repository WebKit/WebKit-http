/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef Icon_h
#define Icon_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Forward.h>
#include <wtf/Vector.h>

#if PLATFORM(IOS)
#include "NativeImagePtr.h"
#include <CoreGraphics/CoreGraphics.h>
#include <wtf/RetainPtr.h>
#elif PLATFORM(MAC)
#include <wtf/RetainPtr.h>
OBJC_CLASS NSImage;
#elif PLATFORM(WIN)
typedef struct HICON__* HICON;
#elif PLATFORM(GTK)
typedef struct _GdkPixbuf GdkPixbuf;
#endif

namespace WebCore {

class GraphicsContext;
class IntRect;
    
class Icon : public RefCounted<Icon> {
public:
    static PassRefPtr<Icon> createIconForFiles(const Vector<String>& filenames);

    ~Icon();

    void paint(GraphicsContext*, const IntRect&);

#if PLATFORM(WIN)
    static PassRefPtr<Icon> create(HICON hIcon) { return adoptRef(new Icon(hIcon)); }
#endif

#if PLATFORM(IOS)
    // FIXME: Make this work for non-iOS ports and remove the PLATFORM(IOS)-guard.
    static PassRefPtr<Icon> createIconForImage(NativeImagePtr);
#endif

private:
#if PLATFORM(IOS)
    Icon(CGImageRef);
    RetainPtr<CGImageRef> m_cgImage;
#elif PLATFORM(MAC)
    Icon(NSImage*);
    RetainPtr<NSImage> m_nsImage;
#elif PLATFORM(WIN)
    Icon(HICON);
    HICON m_hIcon;
#elif PLATFORM(GTK)
    Icon();
    GdkPixbuf* m_icon;
#elif PLATFORM(EFL)
    Icon();
    Evas_Object* m_icon;
#endif
};

}

#endif
