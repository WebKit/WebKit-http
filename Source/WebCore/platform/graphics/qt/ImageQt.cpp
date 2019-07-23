/*
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2010 Sencha, Inc.
 *
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Image.h"

#include "BitmapImage.h"
#include "GraphicsContext.h"
#include "ImageObserver.h"
#include "StillImageQt.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/text/WTFString.h>

typedef Vector<QImage, 3> WebGraphicVector;
typedef HashMap<CString, WebGraphicVector> WebGraphicHash;

static WebGraphicHash& graphics()
{
    static NeverDestroyed<WebGraphicHash> hash;

    if (hash.get().isEmpty()) {
        // QWebSettings::MissingImageGraphic
        hash.get().add("missingImage", WebGraphicVector { {
            QImage(QStringLiteral(":webkit/resources/missingImage.png")),
            QImage(QStringLiteral(":webkit/resources/missingImage@2x.png")),
            QImage(QStringLiteral(":webkit/resources/missingImage@3x.png"))
        } });

        // QWebSettings::MissingPluginGraphic
        hash.get().add("nullPlugin", WebGraphicVector { {
            QImage(QStringLiteral(":webkit/resources/nullPlugin.png")),
            QImage(QStringLiteral(":webkit/resources/nullPlugin@2x.png"))
        } });

        // QWebSettings::DefaultFrameIconGraphic
        hash.get().add("urlIcon", WebGraphicVector { {
            QImage(QStringLiteral(":webkit/resources/urlIcon.png"))
        } });

        // QWebSettings::TextAreaSizeGripCornerGraphic
        hash.get().add("textAreaResizeCorner", WebGraphicVector { {
            QImage(QStringLiteral(":webkit/resources/textAreaResizeCorner.png")),
            QImage(QStringLiteral(":webkit/resources/textAreaResizeCorner@2x.png"))
        } });
    }

    return hash;
}

static QImage loadResourcePixmapForScale(const CString& name, size_t scale)
{
    const auto& iterator = graphics().find(name);
    if (iterator == graphics().end())
        return QImage();

    WebGraphicVector v = iterator->value;
    if (scale <= v.size())
        return v[scale - 1];

    return v.last();
}

static QImage loadResourcePixmap(const char* name)
{
    int length = strlen(name);

    // Handle "name@2x" and "name@3x"
    if (length > 3 && name[length - 1] == 'x' && name[length - 3] == '@' && isASCIIDigit(name[length - 2])) {
        CString nameWithoutScale(name, length - 3);
        char digit = name[length - 2];
        size_t scale = digit - '0';
        return loadResourcePixmapForScale(nameWithoutScale, scale);
    }

    return loadResourcePixmapForScale(CString(name, length), 1);
}

namespace WebCore {

Ref<Image> Image::loadPlatformResource(const char* name)
{
    return StillImage::create(loadResourcePixmap(name));
}

void Image::setPlatformResource(const char* name, const QImage& pixmap)
{
    if (pixmap.isNull())
        graphics().remove(name);
    else
        graphics().add(name, WebGraphicVector { pixmap });
}

void BitmapImage::invalidatePlatformData()
{
}

} // namespace WebCore


// vim: ts=4 sw=4 et
