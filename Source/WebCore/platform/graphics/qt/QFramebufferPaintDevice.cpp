/*
    Copyright (C) 2014 Digia Plc. and/or its subsidiary(-ies)

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
#include "QFramebufferPaintDevice.h"

QFramebufferPaintDevice::QFramebufferPaintDevice(const QSize& size)
    : QOpenGLPaintDevice(size)
    , m_framebufferObject(size, QOpenGLFramebufferObject::CombinedDepthStencil)
{
    m_surface = QOpenGLContext::currentContext()->surface();
    setPaintFlipped(true);
    m_framebufferObject.bind();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void QFramebufferPaintDevice::ensureActiveTarget()
{
    if (QOpenGLContext::currentContext() != context() || context()->surface() != m_surface)
        context()->makeCurrent(m_surface);

    m_framebufferObject.bind();
}

static QImage qt_gl_read_framebuffer(const QSize &size)
{
    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    int w = size.width();
    int h = size.height();

#ifdef QT_OPENGL_ES
    GLint fmt = GL_BGRA_EXT;
#else
    GLint fmt = GL_BGRA;
#endif
    // FIXME: Check for BGRA support before trying it.
    while (glGetError());
    glReadPixels(0, 0, w, h, fmt, GL_UNSIGNED_BYTE, img.bits());
    if (glGetError()) {
        img = QImage(size, QImage::Format_RGBA8888_Premultiplied);
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
    }
    return img;
}

QImage QFramebufferPaintDevice::toImage() const
{
    QOpenGLContext* currentContext = QOpenGLContext::currentContext();
    QSurface* currentSurface = currentContext ? currentContext->surface() : 0;

    context()->makeCurrent(m_surface);

    bool wasBound = m_framebufferObject.isBound();
    if (!wasBound)
        const_cast<QFramebufferPaintDevice*>(this)->bind();
    QImage image = qt_gl_read_framebuffer(size());
    if (!wasBound)
        const_cast<QFramebufferPaintDevice*>(this)->release();

    if (currentContext)
        currentContext->makeCurrent(currentSurface);
    else
        context()->doneCurrent();

    return image;
}
