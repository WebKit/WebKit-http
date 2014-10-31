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
#include <QOpenGLFunctions>

QFramebufferPaintDevice::QFramebufferPaintDevice(const QSize& size,
                                                 QOpenGLFramebufferObject::Attachment attachment,
                                                 bool clearOnInit)
    : QOpenGLPaintDevice(size)
    , m_framebufferObject(size, attachment)
{
    m_surface = QOpenGLContext::currentContext()->surface();
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    setPaintFlipped(true);
#endif
    if (clearOnInit) {
        m_framebufferObject.bind();

        context()->functions()->glClearColor(0, 0, 0, 0);
        context()->functions()->glClear(GL_COLOR_BUFFER_BIT);
    }
}

void QFramebufferPaintDevice::ensureActiveTarget()
{
    if (QOpenGLContext::currentContext() != context())
        context()->makeCurrent(m_surface);

    m_framebufferObject.bind();
}

QImage QFramebufferPaintDevice::toImage() const
{
    QOpenGLContext* currentContext = QOpenGLContext::currentContext();
    QSurface* currentSurface = currentContext ? currentContext->surface() : 0;

    context()->makeCurrent(m_surface);

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    QImage image = m_framebufferObject.toImage(false);
#else
    QImage image = m_framebufferObject.toImage();
#endif

    if (currentContext)
        currentContext->makeCurrent(currentSurface);
    else
        context()->doneCurrent();

    return image;
}
