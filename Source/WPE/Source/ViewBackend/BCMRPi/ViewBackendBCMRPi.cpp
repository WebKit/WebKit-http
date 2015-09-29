#include "ViewBackendBCMRPi.h"

#if WPE_PLATFORM_BCM_RPI

#include "LibinputServer.h"
#include <bcm_host.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>

namespace WPE {

namespace ViewBackend {

static const char s_vertexShader[] =
    "attribute vec4 position;\n"
    "attribute vec4 intexcoord;\n"
    "varying vec2 texcoord;\n"
    "void main() {\n"
    "  gl_Position = position;\n"
    "  texcoord = intexcoord.xy;\n"
    "}\n";
static const char s_fragmentShader[] =
    "varying highp vec2 texcoord;\n"
    "uniform sampler2D tex;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(tex, texcoord);\n"
    "}\n";

static const GLfloat s_squareVertices[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    -1.0f, 1.0f,
    1.0f, 1.0f
};

static const GLfloat s_textureVertices[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
};

static GLuint createShader(const char* source, GLenum shaderType)
{
    GLuint shader = glCreateShader(shaderType);
    if (!shader)
        fprintf(stderr, "createShader(): error %x\n", glGetError());
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        fprintf(stderr, "createShader(): failed to compile\n");
        return 0;
    }

    return shader;
}

ViewBackendBCMRPi::ViewBackendBCMRPi()
{
    bcm_host_init();

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(m_eglDisplay, nullptr, nullptr);

    static const EGLint configAttributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    static const EGLint contextAttributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLConfig eglConfig;
    EGLint numConfig;
    eglChooseConfig(m_eglDisplay, configAttributes, &eglConfig, 1, &numConfig);
    eglBindAPI(EGL_OPENGL_ES_API);

    m_eglContext = eglCreateContext(m_eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttributes);

    uint32_t width, height;
    graphics_get_display_size(0, &width, &height);

    {
        VC_RECT_T srcRect, destRect;

        srcRect.x = srcRect.y = 0;
        srcRect.width = width << 16; srcRect.height = height << 16;

        destRect.x = destRect.y = 0;
        destRect.width = width; destRect.height = height;

        DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open(0);
        DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);

        m_nativeElement = vc_dispmanx_element_add(update, display, 0,
            &destRect, 0, &srcRect, DISPMANX_PROTECTION_NONE, 0, 0, (DISPMANX_TRANSFORM_T)0);
        fprintf(stderr, "\tdispmanx display %x element %x\n", display, m_nativeElement);

        m_nativeWindow.element = m_nativeElement;
        m_nativeWindow.width = width;
        m_nativeWindow.height = height;
        vc_dispmanx_update_submit_sync(update);
    }

    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, eglConfig, &m_nativeWindow, nullptr);

    fprintf(stderr, "\tdpy %p surf %p context %p\n", m_eglDisplay, m_eglSurface, m_eglContext);
    eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);

    m_vertexShader = createShader(s_vertexShader, GL_VERTEX_SHADER);
    m_fragmentShader = createShader(s_fragmentShader, GL_FRAGMENT_SHADER);
    m_program = glCreateProgram();
    glAttachShader(m_program, m_vertexShader);
    glAttachShader(m_program, m_fragmentShader);
    glBindAttribLocation(m_program, 0, "position");
    glBindAttribLocation(m_program, 1, "intexcoord");
    glLinkProgram(m_program);

    GLint status;
    glGetProgramiv(m_program, GL_LINK_STATUS, &status);
    if (!status) {
        fprintf(stderr, "\tfailed to link the program\n");
        char log[1024];
        GLsizei len;
        glGetProgramInfoLog(m_program, 1024, &len, log);
        fprintf(stderr, "%*s\n", len, log);
    }

    m_texUniform = glGetUniformLocation(m_program, "tex");
    fprintf(stderr, "\tprogram %d shaders %d & %d tex uniform %d\n",
        m_program, m_vertexShader, m_fragmentShader, m_texUniform);

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.7f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(m_eglDisplay, m_eglSurface);
}

ViewBackendBCMRPi::~ViewBackendBCMRPi()
{
    LibinputServer::singleton().setClient(nullptr);
}

void ViewBackendBCMRPi::setClient(Client* client)
{
    m_client = client;
}

void ViewBackendBCMRPi::commitBCMBuffer(uint32_t handle1, uint32_t handle2, uint32_t width, uint32_t height, uint32_t format)
{
    eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);

    auto it = m_bufferMap.find(handle1);
    if (it == m_bufferMap.end()) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (EGLImageKHR)handle2);
        glBindTexture(GL_TEXTURE_2D, 0);

        it = m_bufferMap.insert({ handle1, std::pair<GLuint, EGLImageKHR>{ texture, (EGLImageKHR)handle2 } }).first;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, it->second.first);

    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_program);

    glUniform1i(m_texUniform, 0);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, s_squareVertices);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, s_textureVertices);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    eglFlushBRCM();
    eglSwapBuffers(m_eglDisplay, m_eglSurface);

    m_client->releaseBuffer(handle1);
    m_client->frameComplete();
}

void ViewBackendBCMRPi::setInputClient(Input::Client* client)
{
    LibinputServer::singleton().setClient(client);
}

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_PLATFORM_BCM_RPI
