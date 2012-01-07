//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// libEGL.cpp: Implements the exported EGL functions.

#include <exception>

#include "common/debug.h"
#include "common/version.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Texture.h"

#include "libEGL/main.h"
#include "libEGL/Display.h"


bool validateDisplay(egl::Display *display)
{
    if (display == EGL_NO_DISPLAY)
    {
        return error(EGL_BAD_DISPLAY, false);
    }

    if (!display->isInitialized())
    {
        return error(EGL_NOT_INITIALIZED, false);
    }

    return true;
}

bool validateConfig(egl::Display *display, EGLConfig config)
{
    if (!validateDisplay(display))
    {
        return false;
    }

    if (!display->isValidConfig(config))
    {
        return error(EGL_BAD_CONFIG, false);
    }

    return true;
}

bool validateContext(egl::Display *display, gl::Context *context)
{
    if (!validateDisplay(display))
    {
        return false;
    }

    if (!display->isValidContext(context))
    {
        return error(EGL_BAD_CONTEXT, false);
    }

    return true;
}

bool validateSurface(egl::Display *display, egl::Surface *surface)
{
    if (!validateDisplay(display))
    {
        return false;
    }

    if (!display->isValidSurface(surface))
    {
        return error(EGL_BAD_SURFACE, false);
    }

    return true;
}

extern "C"
{
EGLint __stdcall eglGetError(void)
{
    EVENT("()");

    EGLint error = egl::getCurrentError();

    if (error != EGL_SUCCESS)
    {
        egl::setCurrentError(EGL_SUCCESS);
    }

    return error;
}

EGLDisplay __stdcall eglGetDisplay(EGLNativeDisplayType display_id)
{
    EVENT("(EGLNativeDisplayType display_id = 0x%0.8p)", display_id);

    try
    {
        return egl::Display::getDisplay(display_id);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_NO_DISPLAY);
    }

    return EGL_NO_DISPLAY;
}

EGLBoolean __stdcall eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLint *major = 0x%0.8p, EGLint *minor = 0x%0.8p)",
          dpy, major, minor);

    try
    {
        if (dpy == EGL_NO_DISPLAY)
        {
            return error(EGL_BAD_DISPLAY, EGL_FALSE);
        }

        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!display->initialize())
        {
            return error(EGL_NOT_INITIALIZED, EGL_FALSE);
        }

        if (major) *major = 1;
        if (minor) *minor = 4;

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglTerminate(EGLDisplay dpy)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p)", dpy);

    try
    {
        if (dpy == EGL_NO_DISPLAY)
        {
            return error(EGL_BAD_DISPLAY, EGL_FALSE);
        }

        egl::Display *display = static_cast<egl::Display*>(dpy);

        display->terminate();

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

const char *__stdcall eglQueryString(EGLDisplay dpy, EGLint name)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLint name = %d)", dpy, name);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!validateDisplay(display))
        {
            return NULL;
        }

        switch (name)
        {
          case EGL_CLIENT_APIS:
            return success("OpenGL_ES");
          case EGL_EXTENSIONS:
            return display->getExtensionString();
          case EGL_VENDOR:
            return success("Google Inc.");
          case EGL_VERSION:
            return success("1.4 (ANGLE "VERSION_STRING")");
        }

        return error(EGL_BAD_PARAMETER, (const char*)NULL);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, (const char*)NULL);
    }

    return NULL;
}

EGLBoolean __stdcall eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig *configs = 0x%0.8p, "
          "EGLint config_size = %d, EGLint *num_config = 0x%0.8p)",
          dpy, configs, config_size, num_config);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!validateDisplay(display))
        {
            return EGL_FALSE;
        }

        if (!num_config)
        {
            return error(EGL_BAD_PARAMETER, EGL_FALSE);
        }

        const EGLint attribList[] =    {EGL_NONE};

        if (!display->getConfigs(configs, attribList, config_size, num_config))
        {
            return error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
        }

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, const EGLint *attrib_list = 0x%0.8p, "
          "EGLConfig *configs = 0x%0.8p, EGLint config_size = %d, EGLint *num_config = 0x%0.8p)",
          dpy, attrib_list, configs, config_size, num_config);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!validateDisplay(display))
        {
            return EGL_FALSE;
        }

        if (!num_config)
        {
            return error(EGL_BAD_PARAMETER, EGL_FALSE);
        }

        const EGLint attribList[] =    {EGL_NONE};

        if (!attrib_list)
        {
            attrib_list = attribList;
        }

        display->getConfigs(configs, attrib_list, config_size, num_config);

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLint attribute = %d, EGLint *value = 0x%0.8p)",
          dpy, config, attribute, value);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!validateConfig(display, config))
        {
            return EGL_FALSE;
        }

        if (!display->getConfigAttrib(config, attribute, value))
        {
            return error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
        }

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLSurface __stdcall eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLNativeWindowType win = 0x%0.8p, "
          "const EGLint *attrib_list = 0x%0.8p)", dpy, config, win, attrib_list);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!validateConfig(display, config))
        {
            return EGL_NO_SURFACE;
        }

        HWND window = (HWND)win;

        if (!IsWindow(window))
        {
            return error(EGL_BAD_NATIVE_WINDOW, EGL_NO_SURFACE);
        }

        return display->createWindowSurface(window, config, attrib_list);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
    }

    return EGL_NO_SURFACE;
}

EGLSurface __stdcall eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, const EGLint *attrib_list = 0x%0.8p)",
          dpy, config, attrib_list);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!validateConfig(display, config))
        {
            return EGL_NO_SURFACE;
        }

        return display->createOffscreenSurface(config, NULL, attrib_list);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
    }

    return EGL_NO_SURFACE;
}

EGLSurface __stdcall eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLNativePixmapType pixmap = 0x%0.8p, "
          "const EGLint *attrib_list = 0x%0.8p)", dpy, config, pixmap, attrib_list);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!validateConfig(display, config))
        {
            return EGL_NO_SURFACE;
        }

        UNIMPLEMENTED();   // FIXME

        return success(EGL_NO_SURFACE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
    }

    return EGL_NO_SURFACE;
}

EGLBoolean __stdcall eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p)", dpy, surface);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

        if (!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if (surface == EGL_NO_SURFACE)
        {
            return error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        display->destroySurface((egl::Surface*)surface);

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint attribute = %d, EGLint *value = 0x%0.8p)",
          dpy, surface, attribute, value);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = (egl::Surface*)surface;

        if (!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if (surface == EGL_NO_SURFACE)
        {
            return error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        switch (attribute)
        {
          case EGL_VG_ALPHA_FORMAT:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_VG_COLORSPACE:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_CONFIG_ID:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_HEIGHT:
            *value = eglSurface->getHeight();
            break;
          case EGL_HORIZONTAL_RESOLUTION:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_LARGEST_PBUFFER:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_MIPMAP_TEXTURE:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_MIPMAP_LEVEL:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_MULTISAMPLE_RESOLVE:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_PIXEL_ASPECT_RATIO:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_RENDER_BUFFER:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_SWAP_BEHAVIOR:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_TEXTURE_FORMAT:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_TEXTURE_TARGET:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_VERTICAL_RESOLUTION:
            UNIMPLEMENTED();   // FIXME
            break;
          case EGL_WIDTH:
            *value = eglSurface->getWidth();
            break;
          case EGL_POST_SUB_BUFFER_SUPPORTED_NV:
            *value = eglSurface->isPostSubBufferSupported();
            break;
          default:
            return error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
        }

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglQuerySurfacePointerANGLE(EGLDisplay dpy, EGLSurface surface, EGLint attribute, void **value)
{
    TRACE("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint attribute = %d, void **value = 0x%0.8p)",
          dpy, surface, attribute, value);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = (egl::Surface*)surface;

        if (!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if (surface == EGL_NO_SURFACE)
        {
            return error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        switch (attribute)
        {
          case EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE:
            *value = (void*) eglSurface->getShareHandle();
            break;
          default:
            return error(EGL_BAD_ATTRIBUTE, EGL_FALSE);
        }

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglBindAPI(EGLenum api)
{
    EVENT("(EGLenum api = 0x%X)", api);

    try
    {
        switch (api)
        {
          case EGL_OPENGL_API:
          case EGL_OPENVG_API:
            return error(EGL_BAD_PARAMETER, EGL_FALSE);   // Not supported by this implementation
          case EGL_OPENGL_ES_API:
            break;
          default:
            return error(EGL_BAD_PARAMETER, EGL_FALSE);
        }

        egl::setCurrentAPI(api);

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLenum __stdcall eglQueryAPI(void)
{
    EVENT("()");

    try
    {
        EGLenum API = egl::getCurrentAPI();

        return success(API);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglWaitClient(void)
{
    EVENT("()");

    try
    {
        UNIMPLEMENTED();   // FIXME

        return success(0);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglReleaseThread(void)
{
    EVENT("()");

    try
    {
        eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_CONTEXT, EGL_NO_SURFACE, EGL_NO_SURFACE);

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLSurface __stdcall eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLenum buftype = 0x%X, EGLClientBuffer buffer = 0x%0.8p, "
          "EGLConfig config = 0x%0.8p, const EGLint *attrib_list = 0x%0.8p)",
          dpy, buftype, buffer, config, attrib_list);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!validateConfig(display, config))
        {
            return EGL_NO_SURFACE;
        }

        if (buftype != EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE || !buffer)
        {
            return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
        }

        return display->createOffscreenSurface(config, (HANDLE)buffer, attrib_list);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
    }

    return EGL_NO_SURFACE;
}

EGLBoolean __stdcall eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint attribute = %d, EGLint value = %d)",
          dpy, surface, attribute, value);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

        if (!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        UNIMPLEMENTED();   // FIXME

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint buffer = %d)", dpy, surface, buffer);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

        if (!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if (buffer != EGL_BACK_BUFFER)
        {
            return error(EGL_BAD_PARAMETER, EGL_FALSE);
        }

        if (surface == EGL_NO_SURFACE || eglSurface->getWindowHandle())
        {
            return error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        if (eglSurface->getBoundTexture())
        {
            return error(EGL_BAD_ACCESS, EGL_FALSE);
        }

        if (eglSurface->getTextureFormat() == EGL_NO_TEXTURE)
        {
            return error(EGL_BAD_MATCH, EGL_FALSE);
        }

        if (!glBindTexImage(eglSurface))
        {
            return error(EGL_BAD_MATCH, EGL_FALSE);
        }

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint buffer = %d)", dpy, surface, buffer);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

        if (!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if (buffer != EGL_BACK_BUFFER)
        {
            return error(EGL_BAD_PARAMETER, EGL_FALSE);
        }

        if (surface == EGL_NO_SURFACE || eglSurface->getWindowHandle())
        {
            return error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        if (eglSurface->getTextureFormat() == EGL_NO_TEXTURE)
        {
            return error(EGL_BAD_MATCH, EGL_FALSE);
        }

        gl::Texture2D *texture = eglSurface->getBoundTexture();

        if (texture)
        {
            texture->releaseTexImage();
        }

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLint interval = %d)", dpy, interval);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!validateDisplay(display))
        {
            return EGL_FALSE;
        }

        egl::Surface *draw_surface = static_cast<egl::Surface*>(egl::getCurrentDrawSurface());

        if (draw_surface == NULL)
        {
            return error(EGL_BAD_SURFACE, EGL_FALSE);
        }
        
        draw_surface->setSwapInterval(interval);

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLContext __stdcall eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLConfig config = 0x%0.8p, EGLContext share_context = 0x%0.8p, "
          "const EGLint *attrib_list = 0x%0.8p)", dpy, config, share_context, attrib_list);

    try
    {
        // Get the requested client version (default is 1) and check it is two.
        EGLint client_version = 1;
        bool reset_notification = false;
        bool robust_access = false;

        if (attrib_list)
        {
            for (const EGLint* attribute = attrib_list; attribute[0] != EGL_NONE; attribute += 2)
            {
                switch (attribute[0])
                {
                  case EGL_CONTEXT_CLIENT_VERSION:
                    client_version = attribute[1];
                    break;
                  case EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT:
                    if (attribute[1] == EGL_TRUE)
                    {
                        return error(EGL_BAD_CONFIG, EGL_NO_CONTEXT);   // Unimplemented
                        robust_access = true;
                    }
                    else if (attribute[1] != EGL_FALSE)
                        return error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
                    break;
                  case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT:
                    if (attribute[1] == EGL_LOSE_CONTEXT_ON_RESET_EXT)
                        reset_notification = true;
                    else if (attribute[1] != EGL_NO_RESET_NOTIFICATION_EXT)
                        return error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
                    break;
                  default:
                    return error(EGL_BAD_ATTRIBUTE, EGL_NO_CONTEXT);
                }
            }
        }

        if (client_version != 2)
        {
            return error(EGL_BAD_CONFIG, EGL_NO_CONTEXT);
        }

        if (share_context && static_cast<gl::Context*>(share_context)->isResetNotificationEnabled() != reset_notification)
        {
            return error(EGL_BAD_MATCH, EGL_NO_CONTEXT);
        }

        egl::Display *display = static_cast<egl::Display*>(dpy);

        if (!validateConfig(display, config))
        {
            return EGL_NO_CONTEXT;
        }

        EGLContext context = display->createContext(config, static_cast<gl::Context*>(share_context), reset_notification, robust_access);

        if (context)
            return success(context);
        else
            return error(EGL_CONTEXT_LOST, EGL_NO_CONTEXT);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_NO_CONTEXT);
    }

    return EGL_NO_CONTEXT;
}

EGLBoolean __stdcall eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLContext ctx = 0x%0.8p)", dpy, ctx);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        gl::Context *context = static_cast<gl::Context*>(ctx);

        if (!validateContext(display, context))
        {
            return EGL_FALSE;
        }

        if (ctx == EGL_NO_CONTEXT)
        {
            return error(EGL_BAD_CONTEXT, EGL_FALSE);
        }

        display->destroyContext(context);

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface draw = 0x%0.8p, EGLSurface read = 0x%0.8p, EGLContext ctx = 0x%0.8p)",
          dpy, draw, read, ctx);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        gl::Context *context = static_cast<gl::Context*>(ctx);
        IDirect3DDevice9 *device = display->getDevice();

        if (!device || display->testDeviceLost())
        {
            display->notifyDeviceLost();
            return EGL_FALSE;
        }

        if (display->isDeviceLost())
        {
            return error(EGL_CONTEXT_LOST, EGL_FALSE);
        }

        if (ctx != EGL_NO_CONTEXT && !validateContext(display, context))
        {
            return EGL_FALSE;
        }

        if ((draw != EGL_NO_SURFACE && !validateSurface(display, static_cast<egl::Surface*>(draw))) ||
            (read != EGL_NO_SURFACE && !validateSurface(display, static_cast<egl::Surface*>(read))))
        {
            return EGL_FALSE;
        }

        if (draw != read)
        {
            UNIMPLEMENTED();   // FIXME
        }

        egl::Surface* previousDraw = static_cast<egl::Surface*>(egl::getCurrentDrawSurface());
        egl::Surface* previousRead = static_cast<egl::Surface*>(egl::getCurrentReadSurface());

        egl::setCurrentDisplay(dpy);
        egl::setCurrentDrawSurface(draw);
        egl::setCurrentReadSurface(read);

        glMakeCurrent(context, display, static_cast<egl::Surface*>(draw));

        return success(EGL_TRUE);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLContext __stdcall eglGetCurrentContext(void)
{
    EVENT("()");

    try
    {
        EGLContext context = glGetCurrentContext();

        return success(context);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_NO_CONTEXT);
    }

    return EGL_NO_CONTEXT;
}

EGLSurface __stdcall eglGetCurrentSurface(EGLint readdraw)
{
    EVENT("(EGLint readdraw = %d)", readdraw);

    try
    {
        if (readdraw == EGL_READ)
        {
            EGLSurface read = egl::getCurrentReadSurface();
            return success(read);
        }
        else if (readdraw == EGL_DRAW)
        {
            EGLSurface draw = egl::getCurrentDrawSurface();
            return success(draw);
        }
        else
        {
            return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
    }

    return EGL_NO_SURFACE;
}

EGLDisplay __stdcall eglGetCurrentDisplay(void)
{
    EVENT("()");

    try
    {
        EGLDisplay dpy = egl::getCurrentDisplay();

        return success(dpy);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_NO_DISPLAY);
    }

    return EGL_NO_DISPLAY;
}

EGLBoolean __stdcall eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLContext ctx = 0x%0.8p, EGLint attribute = %d, EGLint *value = 0x%0.8p)",
          dpy, ctx, attribute, value);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        gl::Context *context = static_cast<gl::Context*>(ctx);

        if (!validateContext(display, context))
        {
            return EGL_FALSE;
        }

        UNIMPLEMENTED();   // FIXME

        return success(0);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglWaitGL(void)
{
    EVENT("()");

    try
    {
        UNIMPLEMENTED();   // FIXME

        return success(0);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglWaitNative(EGLint engine)
{
    EVENT("(EGLint engine = %d)", engine);

    try
    {
        UNIMPLEMENTED();   // FIXME

        return success(0);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p)", dpy, surface);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = (egl::Surface*)surface;

        if (!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if (display->isDeviceLost())
        {
            return error(EGL_CONTEXT_LOST, EGL_FALSE);
        }

        if (surface == EGL_NO_SURFACE)
        {
            return error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        if (eglSurface->swap())
        {
            return success(EGL_TRUE);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLNativePixmapType target = 0x%0.8p)", dpy, surface, target);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

        if (!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if (display->isDeviceLost())
        {
            return error(EGL_CONTEXT_LOST, EGL_FALSE);
        }

        UNIMPLEMENTED();   // FIXME

        return success(0);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

EGLBoolean __stdcall eglPostSubBufferNV(EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y, EGLint width, EGLint height)
{
    EVENT("(EGLDisplay dpy = 0x%0.8p, EGLSurface surface = 0x%0.8p, EGLint x = %d, EGLint y = %d, EGLint width = %d, EGLint height = %d)", dpy, surface, x, y, width, height);

    try
    {
        egl::Display *display = static_cast<egl::Display*>(dpy);
        egl::Surface *eglSurface = static_cast<egl::Surface*>(surface);

        if (!validateSurface(display, eglSurface))
        {
            return EGL_FALSE;
        }

        if (display->isDeviceLost())
        {
            return error(EGL_CONTEXT_LOST, EGL_FALSE);
        }

        if (surface == EGL_NO_SURFACE)
        {
            return error(EGL_BAD_SURFACE, EGL_FALSE);
        }

        if (eglSurface->postSubBuffer(x, y, width, height))
        {
            return success(EGL_TRUE);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, EGL_FALSE);
    }

    return EGL_FALSE;
}

__eglMustCastToProperFunctionPointerType __stdcall eglGetProcAddress(const char *procname)
{
    EVENT("(const char *procname = \"%s\")", procname);

    try
    {
        struct Extension
        {
            const char *name;
            __eglMustCastToProperFunctionPointerType address;
        };

        static const Extension eglExtensions[] =
        {
            {"eglQuerySurfacePointerANGLE", (__eglMustCastToProperFunctionPointerType)eglQuerySurfacePointerANGLE},
            {"eglPostSubBufferNV", (__eglMustCastToProperFunctionPointerType)eglPostSubBufferNV},
            {"", NULL},
        };

        for (int ext = 0; ext < sizeof(eglExtensions) / sizeof(Extension); ext++)
        {
            if (strcmp(procname, eglExtensions[ext].name) == 0)
            {
                return (__eglMustCastToProperFunctionPointerType)eglExtensions[ext].address;
            }
        }

        return glGetProcAddress(procname);
    }
    catch(std::bad_alloc&)
    {
        return error(EGL_BAD_ALLOC, (__eglMustCastToProperFunctionPointerType)NULL);
    }

    return NULL;
}
}
