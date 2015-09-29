#if WPE_PLATFORM_BCM_RPI

#include <WPE/ViewBackend/ViewBackend.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <unordered_map>

namespace WPE {

namespace ViewBackend {

class ViewBackendBCMRPi final : public ViewBackend {
public:
    ViewBackendBCMRPi();
    virtual ~ViewBackendBCMRPi();

    void setClient(Client*) override;
    void commitBCMBuffer(uint32_t handle1, uint32_t handle2, uint32_t width, uint32_t height, uint32_t format) override;

    void setInputClient(Input::Client*) override;

private:
    Client* m_client;

    EGLDisplay m_eglDisplay;
    EGLContext m_eglContext;
    EGLSurface m_eglSurface;
    EGL_DISPMANX_WINDOW_T m_nativeWindow;
    DISPMANX_ELEMENT_HANDLE_T m_nativeElement;

    GLuint m_program;
    GLuint m_vertexShader;
    GLuint m_fragmentShader;
    GLuint m_texUniform;

    std::unordered_map<uint32_t, std::pair<GLuint, EGLImageKHR>> m_bufferMap;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_PLATFORM_BCM_RPI
