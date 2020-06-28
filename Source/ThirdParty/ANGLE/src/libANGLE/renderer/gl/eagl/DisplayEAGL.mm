//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayEAGL.cpp: EAGL implementation of egl::Display

#import "common/platform.h"

#if defined(ANGLE_PLATFORM_IOS) && !defined(ANGLE_PLATFORM_MACCATALYST)

#    import "libANGLE/renderer/gl/eagl/DisplayEAGL.h"

#    import "common/debug.h"
#    import "gpu_info_util/SystemInfo.h"
#    import "libANGLE/Display.h"
#    import "libANGLE/renderer/gl/eagl/ContextEAGL.h"
#    import "libANGLE/renderer/gl/eagl/DeviceEAGL.h"
#    import "libANGLE/renderer/gl/eagl/IOSurfaceSurfaceEAGL.h"
#    import "libANGLE/renderer/gl/eagl/PbufferSurfaceEAGL.h"
#    import "libANGLE/renderer/gl/eagl/RendererEAGL.h"
#    import "libANGLE/renderer/gl/eagl/WindowSurfaceEAGL.h"

#    import <Foundation/Foundation.h>
#    import <OpenGLES/EAGL.h>
#    import <QuartzCore/QuartzCore.h>
#    import <dlfcn.h>

#    define GLES_SILENCE_DEPRECATION

namespace
{

const char *kOpenGLESDylibName = "/System/Library/Frameworks/OpenGLES.framework/OpenGLES";

}

namespace rx
{

class FunctionsGLEAGL : public FunctionsGL
{
  public:
    FunctionsGLEAGL(void *dylibHandle) : mDylibHandle(dylibHandle) {}

    ~FunctionsGLEAGL() override { dlclose(mDylibHandle); }

  private:
    void *loadProcAddress(const std::string &function) const override
    {
        return dlsym(mDylibHandle, function.c_str());
    }

    void *mDylibHandle;
};

DisplayEAGL::DisplayEAGL(const egl::DisplayState &state)
    : DisplayGL(state), mEGLDisplay(nullptr), mContext(nullptr)
{}

DisplayEAGL::~DisplayEAGL() {}

egl::Error DisplayEAGL::initialize(egl::Display *display)
{
    mEGLDisplay = display;

    angle::SystemInfo info;
    if (!angle::GetSystemInfo(&info))
    {
        return egl::EglNotInitialized() << "Unable to query ANGLE's SystemInfo.";
    }

    mContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (mContext == nullptr)
    {
        return egl::EglNotInitialized() << "Could not create the EAGL context.";
    }
    [EAGLContext setCurrentContext:mContext];

    // There is no equivalent getProcAddress in EAGL so we open the dylib directly
    void *handle = dlopen(kOpenGLESDylibName, RTLD_NOW);
    if (!handle)
    {
        return egl::EglNotInitialized() << "Could not open the OpenGLES Framework.";
    }

    std::unique_ptr<FunctionsGL> functionsGL(new FunctionsGLEAGL(handle));
    functionsGL->initialize(display->getAttributeMap());

    mRenderer.reset(new RendererEAGL(std::move(functionsGL), display->getAttributeMap(), this));

    const gl::Version &maxVersion = mRenderer->getMaxSupportedESVersion();
    if (maxVersion < gl::Version(2, 0))
    {
        return egl::EglNotInitialized() << "OpenGL ES 2.0 is not supportable.";
    }

    return DisplayGL::initialize(display);
}

void DisplayEAGL::terminate()
{
    DisplayGL::terminate();

    mRenderer.reset();
    if (mContext != nullptr)
    {
        [EAGLContext setCurrentContext:nil];
        [mContext release];
        mContext = nullptr;
    }
}

SurfaceImpl *DisplayEAGL::createWindowSurface(const egl::SurfaceState &state,
                                              EGLNativeWindowType window,
                                              const egl::AttributeMap &attribs)
{
    return new WindowSurfaceEAGL(state, mRenderer.get(), window, mContext);
}

SurfaceImpl *DisplayEAGL::createPbufferSurface(const egl::SurfaceState &state,
                                               const egl::AttributeMap &attribs)
{
    EGLint width  = static_cast<EGLint>(attribs.get(EGL_WIDTH, 0));
    EGLint height = static_cast<EGLint>(attribs.get(EGL_HEIGHT, 0));
    return new PbufferSurfaceEAGL(state, mRenderer.get(), width, height);
}

SurfaceImpl *DisplayEAGL::createPbufferFromClientBuffer(const egl::SurfaceState &state,
                                                        EGLenum buftype,
                                                        EGLClientBuffer clientBuffer,
                                                        const egl::AttributeMap &attribs)
{
    ASSERT(buftype == EGL_IOSURFACE_ANGLE);
    return new IOSurfaceSurfaceEAGL(state, mContext, clientBuffer, attribs);
}

SurfaceImpl *DisplayEAGL::createPixmapSurface(const egl::SurfaceState &state,
                                              NativePixmapType nativePixmap,
                                              const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

ContextImpl *DisplayEAGL::createContext(const gl::State &state,
                                        gl::ErrorSet *errorSet,
                                        const egl::Config *configuration,
                                        const gl::Context *shareContext,
                                        const egl::AttributeMap &attribs)
{
    return new ContextEAGL(state, errorSet, mRenderer);
}

DeviceImpl *DisplayEAGL::createDevice()
{
    return new DeviceEAGL();
}

egl::ConfigSet DisplayEAGL::generateConfigs()
{
    // TODO(cwallez): generate more config permutations
    egl::ConfigSet configs;

    const gl::Version &maxVersion = getMaxSupportedESVersion();
    ASSERT(maxVersion >= gl::Version(2, 0));
    bool supportsES3 = maxVersion >= gl::Version(3, 0);

    egl::Config config;

    // Native stuff
    config.nativeVisualID   = 0;
    config.nativeVisualType = 0;
    config.nativeRenderable = EGL_TRUE;

    // Buffer sizes
    config.redSize     = 8;
    config.greenSize   = 8;
    config.blueSize    = 8;
    config.alphaSize   = 8;
    config.depthSize   = 24;
    config.stencilSize = 8;

    config.colorBufferType = EGL_RGB_BUFFER;
    config.luminanceSize   = 0;
    config.alphaMaskSize   = 0;

    config.bufferSize = config.redSize + config.greenSize + config.blueSize + config.alphaSize;

    config.transparentType = EGL_NONE;

    // Pbuffer
    config.maxPBufferWidth  = 4096;
    config.maxPBufferHeight = 4096;
    config.maxPBufferPixels = 4096 * 4096;

    // Caveat
    config.configCaveat = EGL_NONE;

    // Misc
    config.sampleBuffers     = 0;
    config.samples           = 0;
    config.level             = 0;
    config.bindToTextureRGB  = EGL_FALSE;
    config.bindToTextureRGBA = EGL_FALSE;

#    if !ANGLE_PLATFORM_MACCATALYST
    config.bindToTextureTarget = EGL_TEXTURE_2D;
#    else
    config.bindToTextureTarget = EGL_TEXTURE_RECTANGLE_ANGLE;
#    endif

    config.surfaceType = EGL_WINDOW_BIT | EGL_PBUFFER_BIT;

    config.minSwapInterval = 1;
    config.maxSwapInterval = 1;

    config.renderTargetFormat = GL_RGBA8;
    config.depthStencilFormat = GL_DEPTH24_STENCIL8;

    config.conformant     = EGL_OPENGL_ES2_BIT | (supportsES3 ? EGL_OPENGL_ES3_BIT_KHR : 0);
    config.renderableType = config.conformant;

    config.matchNativePixmap = EGL_NONE;

    config.colorComponentType = EGL_COLOR_COMPONENT_TYPE_FIXED_EXT;

    configs.add(config);
    return configs;
}

bool DisplayEAGL::testDeviceLost()
{
    // TODO(cwallez) investigate implementing this
    return false;
}

egl::Error DisplayEAGL::restoreLostDevice(const egl::Display *display)
{
    UNIMPLEMENTED();
    return egl::EglBadDisplay();
}

bool DisplayEAGL::isValidNativeWindow(EGLNativeWindowType window) const
{
    NSObject *layer = reinterpret_cast<NSObject *>(window);
    return [layer isKindOfClass:[CALayer class]];
}

egl::Error DisplayEAGL::validateClientBuffer(const egl::Config *configuration,
                                             EGLenum buftype,
                                             EGLClientBuffer clientBuffer,
                                             const egl::AttributeMap &attribs) const
{
    ASSERT(buftype == EGL_IOSURFACE_ANGLE);

    if (!IOSurfaceSurfaceEAGL::validateAttributes(clientBuffer, attribs))
    {
        return egl::EglBadAttribute();
    }

    return egl::NoError();
}

std::string DisplayEAGL::getVendorString() const
{
    // TODO(cwallez) find a useful vendor string
    return "";
}

EAGLContextObj DisplayEAGL::getEAGLContext() const
{
    return mContext;
}

void DisplayEAGL::generateExtensions(egl::DisplayExtensions *outExtensions) const
{
    outExtensions->iosurfaceClientBuffer = true;
    outExtensions->surfacelessContext    = true;
    outExtensions->deviceQuery           = true;

    // Contexts are virtualized so textures can be shared globally
    outExtensions->displayTextureShareGroup = true;

    outExtensions->powerPreference = false;

    DisplayGL::generateExtensions(outExtensions);
}

void DisplayEAGL::generateCaps(egl::Caps *outCaps) const
{
    outCaps->textureNPOT = true;
}

egl::Error DisplayEAGL::waitClient(const gl::Context *context)
{
    // TODO(cwallez) UNIMPLEMENTED()
    return egl::NoError();
}

egl::Error DisplayEAGL::waitNative(const gl::Context *context, EGLint engine)
{
    // TODO(cwallez) UNIMPLEMENTED()
    return egl::NoError();
}

gl::Version DisplayEAGL::getMaxSupportedESVersion() const
{
    return mRenderer->getMaxSupportedESVersion();
}

egl::Error DisplayEAGL::makeCurrentSurfaceless(gl::Context *context)
{
    // We have nothing to do as mContext is always current, and that EAGL is surfaceless by
    // default.
    return egl::NoError();
}

class WorkerContextEAGL final : public WorkerContext
{
  public:
    WorkerContextEAGL(EAGLContextObj context);
    ~WorkerContextEAGL() override;

    bool makeCurrent() override;
    void unmakeCurrent() override;

  private:
    EAGLContextObj mContext;
};

WorkerContextEAGL::WorkerContextEAGL(EAGLContextObj context) : mContext(context) {}

WorkerContextEAGL::~WorkerContextEAGL()
{
    [EAGLContext setCurrentContext:nil];
    [mContext release];
    mContext = nullptr;
}

bool WorkerContextEAGL::makeCurrent()
{
    if (![EAGLContext setCurrentContext:static_cast<EAGLContext *>(mContext)])
    {
        ERR() << "Unable to make gl context current.";
        return false;
    }
    return true;
}

void WorkerContextEAGL::unmakeCurrent()
{
    [EAGLContext setCurrentContext:nil];
}

WorkerContext *DisplayEAGL::createWorkerContext(std::string *infoLog)
{
    EAGLContextObj context = nullptr;
    context                = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (!context)
    {
        *infoLog += "Could not create the EAGL context.";
        return nullptr;
    }

    return new WorkerContextEAGL(context);
}

void DisplayEAGL::initializeFrontendFeatures(angle::FrontendFeatures *features) const
{
    mRenderer->initializeFrontendFeatures(features);
}

void DisplayEAGL::populateFeatureList(angle::FeatureList *features)
{
    mRenderer->getFeatures().populateFeatureList(features);
}
}

#endif  // defined(ANGLE_PLATFORM_IOS) && !defined(ANGLE_PLATFORM_MACCATALYST)
