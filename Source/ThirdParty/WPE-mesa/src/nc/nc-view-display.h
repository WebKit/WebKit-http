/*
 * Copyright (C) 2016 Garmin Ltd.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef nc_view_display_h
#define nc_view_display_h

#include <glib.h>
#include <wayland-server-core.h>

struct wl_display;
struct wl_global;
struct wl_client;
struct wl_resource;

namespace NC {

class ViewDisplay {
public:
    class Resource {
    public:
        struct wl_resource* resource() const { return m_resource; }

    protected:
        Resource(struct wl_resource* resource);
        virtual ~Resource();

    private:
        struct wl_resource* m_resource;
        struct wl_listener m_listener;

        static void destroyListener(struct wl_listener*, void*);
    };

    class Buffer: public Resource {
    public:
        int32_t X() const { return m_x; }
        int32_t Y() const { return m_y; }

        Buffer(struct wl_resource* resource, int32_t x, int32_t y)
            : Resource(resource)
            , m_x(x)
            , m_y(y)
            {}

    private:
        int32_t m_x;
        int32_t m_y;
    };

    class Surface: public Resource {
    public:
        Surface(struct wl_resource* resource)
            : Resource(resource)
            {}
    };

    class FrameCallback: public Resource {
    public:
        FrameCallback(struct wl_resource* resource)
            : Resource(resource)
            {}
    };

    class Damage {
    public:
        int32_t X() const { return m_x; }
        int32_t Y() const { return m_y; }
        int32_t width() const { return m_width; }
        int32_t height() const { return m_height; }

        void expand(int32_t, int32_t, int32_t, int32_t);

        Damage(int32_t x, int32_t y, int32_t w, int32_t h)
            : m_x(x)
            , m_y(y)
            , m_width(w)
            , m_height(h)
            {}

    private:
        int32_t m_x;
        int32_t m_y;
        int32_t m_width;
        int32_t m_height;
    };

    class Client {
    public:
        virtual void OnSurfaceAttach(Surface const&, Buffer const*) {};
        virtual void OnSurfaceDamage(Surface const&, Damage const*) {};
        virtual void OnSurfaceFrame(Surface const&, FrameCallback const*) {};
        virtual void OnSurfaceCommit(Surface const&, Buffer const*, FrameCallback const*, Damage const*) = 0;
    };

    ViewDisplay(Client*);
    ~ViewDisplay();

    void initialize(struct wl_display*);

    void setClient(Client*);

private:
    struct {
        Buffer* buffer {nullptr};
        FrameCallback* frameCallback {nullptr};
        Damage* damage {nullptr};
    } m_pending;

    Surface* m_surface {nullptr};

    Client* m_client {nullptr};

    struct wl_global* m_compositor_global {nullptr};
    struct wl_resource* m_compositor {nullptr};

    void clearPending();

    static void unsupportedOperation(struct wl_resource*);

};

}; // namespace NC

#endif // nc_view_display


