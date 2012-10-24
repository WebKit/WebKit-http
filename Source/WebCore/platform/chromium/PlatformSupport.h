/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PlatformSupport_h
#define PlatformSupport_h

#if ENABLE(WEB_AUDIO)
#include "AudioBus.h"
#endif

#include "FileSystem.h"
#include "ImageSource.h"
#include "LinkHash.h"
#include "PluginData.h"

#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

typedef struct NPObject NPObject;
typedef struct _NPP NPP_t;
typedef NPP_t* NPP;

#if OS(WINDOWS)
typedef struct HFONT__* HFONT;
#endif

namespace WebCore {

class Color;
class Cursor;
class Document;
class Frame;
class GeolocationServiceBridge;
class GeolocationServiceChromium;
class GraphicsContext;
class Image;
class IDBFactoryBackendInterface;
class IntRect;
class KURL;
class SerializedScriptValue;
class Widget;

struct Cookie;
struct FontRenderStyle;

// PlatformSupport an interface to the embedding layer that lets the embedder
// supply implementations for Platform functionality that WebCore cannot access
// directly (for example, because WebCore is in a sandbox).

class PlatformSupport {
public:
    // Cookies ------------------------------------------------------------
    static void setCookies(const Document*, const KURL&, const String& value);
    static String cookies(const Document*, const KURL&);
    static String cookieRequestHeaderFieldValue(const Document*, const KURL&);
    static bool rawCookies(const Document*, const KURL&, Vector<Cookie>&);
    static void deleteCookie(const Document*, const KURL&, const String& cookieName);
    static bool cookiesEnabled(const Document*);

    // Font ---------------------------------------------------------------
#if OS(WINDOWS)
    static bool ensureFontLoaded(HFONT);
#endif

    // IndexedDB ----------------------------------------------------------
    static PassRefPtr<IDBFactoryBackendInterface> idbFactory();

    // Plugin -------------------------------------------------------------
    static bool plugins(bool refresh, Vector<PluginInfo>*);
    static NPObject* pluginScriptableObject(Widget*);

    // Theming ------------------------------------------------------------
#if OS(WINDOWS)
    static void paintButton(
        GraphicsContext*, int part, int state, int classicState, const IntRect&);
    static void paintMenuList(
        GraphicsContext*, int part, int state, int classicState, const IntRect&);
    static void paintScrollbarArrow(
        GraphicsContext*, int state, int classicState, const IntRect&);
    static void paintScrollbarThumb(
        GraphicsContext*, int part, int state, int classicState, const IntRect&);
    static void paintScrollbarTrack(
        GraphicsContext*, int part, int state, int classicState, const IntRect&, const IntRect& alignRect);
    static void paintSpinButton(
        GraphicsContext*, int part, int state, int classicState, const IntRect&);
    static void paintTextField(
        GraphicsContext*, int part, int state, int classicState, const IntRect&, const Color&, bool fillContentArea, bool drawEdges);
    static void paintTrackbar(
        GraphicsContext*, int part, int state, int classicState, const IntRect&);
    static void paintProgressBar(
        GraphicsContext*, const IntRect& barRect, const IntRect& valueRect, bool determinate, double animatedSeconds);
#endif
};

} // namespace WebCore

#endif
