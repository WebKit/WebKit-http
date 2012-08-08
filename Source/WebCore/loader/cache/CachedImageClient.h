/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

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

#ifndef CachedImageClient_h
#define CachedImageClient_h

#include "CachedResourceClient.h"

namespace WebCore {

class CachedImage;
class IntRect;

class CachedImageClient : public CachedResourceClient {
public:
    virtual ~CachedImageClient() { }
    static CachedResourceClientType expectedType() { return ImageType; }
    virtual CachedResourceClientType resourceClientType() const { return expectedType(); }

    // Called whenever a frame of an image changes, either because we got more data from the network or
    // because we are animating. If not null, the IntRect is the changed rect of the image.
    virtual void imageChanged(CachedImage*, const IntRect* = 0) { }

    // Called to find out if this client wants to actually display the image. Used to tell when we
    // can halt animation. Content nodes that hold image refs for example would not render the image,
    // but RenderImages would (assuming they have visibility: visible and their render tree isn't hidden
    // e.g., in the b/f cache or in a background tab).
    virtual bool willRenderImage(CachedImage*) { return false; }
};

}

#endif
