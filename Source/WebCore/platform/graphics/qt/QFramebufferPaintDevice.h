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

#ifndef QFramebufferPaintDevice_h
#define QFramebufferPaintDevice_h

#include <QImage>
#include <QOpenGLPaintDevice>
#include <QOpenGLFramebufferObject>
#include <QSurface>

class QFramebufferPaintDevice : public QOpenGLPaintDevice {
public:
    QFramebufferPaintDevice(const QSize& size,
                            QOpenGLFramebufferObject::Attachment attachment = QOpenGLFramebufferObject::CombinedDepthStencil,
                            bool clearOnInit = true);

    // QOpenGLPaintDevice:
    virtual void ensureActiveTarget() Q_DECL_OVERRIDE;

    bool isValid() const { return m_framebufferObject.isValid(); }
    GLuint handle() const { return m_framebufferObject.handle(); }
    GLuint texture() const { return m_framebufferObject.texture(); }
    QImage toImage() const;

    bool bind() { return m_framebufferObject.bind(); }
    bool release() { return m_framebufferObject.release(); }
    QSize size() const { return m_framebufferObject.size(); }

    QOpenGLFramebufferObject* framebufferObject() { return &m_framebufferObject; }
    const QOpenGLFramebufferObject* framebufferObject() const { return &m_framebufferObject; }

    static bool isSupported() { return QOpenGLFramebufferObject::hasOpenGLFramebufferObjects(); }

private:
    QOpenGLFramebufferObject m_framebufferObject;
    QSurface* m_surface;
};

#endif
