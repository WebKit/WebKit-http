/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ImageLoader.h"

#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "CrossOriginAccessControl.h"
#include "Document.h"
#include "Element.h"
#include "ElementShadow.h"
#include "Event.h"
#include "EventSender.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "HTMLParserIdioms.h"
#include "ImageLoaderClient.h"
#include "RenderImage.h"
#include "ScriptCallStack.h"
#include "SecurityOrigin.h"

#if ENABLE(SVG)
#include "RenderSVGImage.h"
#endif
#if ENABLE(VIDEO)
#include "RenderVideo.h"
#endif

#if !ASSERT_DISABLED
// ImageLoader objects are allocated as members of other objects, so generic pointer check would always fail.
namespace WTF {

template<> struct ValueCheck<WebCore::ImageLoader*> {
    typedef WebCore::ImageLoader* TraitType;
    static void checkConsistency(const WebCore::ImageLoader* p)
    {
        if (!p)
            return;
        ASSERT(p->client()->imageElement());
        ValueCheck<WebCore::Element*>::checkConsistency(p->client()->imageElement());
    }
};

}
#endif

namespace WebCore {

static ImageEventSender& beforeLoadEventSender()
{
    DEFINE_STATIC_LOCAL(ImageEventSender, sender, (eventNames().beforeloadEvent));
    return sender;
}

static ImageEventSender& loadEventSender()
{
    DEFINE_STATIC_LOCAL(ImageEventSender, sender, (eventNames().loadEvent));
    return sender;
}

static ImageEventSender& errorEventSender()
{
    DEFINE_STATIC_LOCAL(ImageEventSender, sender, (eventNames().errorEvent));
    return sender;
}

ImageLoader::ImageLoader(ImageLoaderClient* client)
    : m_client(client)
    , m_image(0)
    , m_hasPendingBeforeLoadEvent(false)
    , m_hasPendingLoadEvent(false)
    , m_hasPendingErrorEvent(false)
    , m_imageComplete(true)
    , m_loadManually(false)
    , m_elementIsProtected(false)
{
}

ImageLoader::~ImageLoader()
{
    if (m_image)
        m_image->removeClient(this);

    ASSERT(m_hasPendingBeforeLoadEvent || !beforeLoadEventSender().hasPendingEvents(this));
    if (m_hasPendingBeforeLoadEvent)
        beforeLoadEventSender().cancelEvent(this);

    ASSERT(m_hasPendingLoadEvent || !loadEventSender().hasPendingEvents(this));
    if (m_hasPendingLoadEvent)
        loadEventSender().cancelEvent(this);

    ASSERT(m_hasPendingErrorEvent || !errorEventSender().hasPendingEvents(this));
    if (m_hasPendingErrorEvent)
        errorEventSender().cancelEvent(this);

    // If the ImageLoader is being destroyed but it is still protecting its image-loading Element,
    // remove that protection here.
    if (m_elementIsProtected)
        client()->derefSourceElement();
}

inline Document* ImageLoader::document()
{
    return client()->sourceElement()->document();
}

void ImageLoader::setImage(CachedImage* newImage)
{
    setImageWithoutConsideringPendingLoadEvent(newImage);

    // Only consider updating the protection ref-count of the Element immediately before returning
    // from this function as doing so might result in the destruction of this ImageLoader.
    updatedHasPendingLoadEvent();
}

void ImageLoader::setImageWithoutConsideringPendingLoadEvent(CachedImage* newImage)
{
    ASSERT(m_failedLoadURL.isEmpty());
    CachedImage* oldImage = m_image.get();
    if (newImage != oldImage) {
        m_image = newImage;
        if (m_hasPendingBeforeLoadEvent) {
            beforeLoadEventSender().cancelEvent(this);
            m_hasPendingBeforeLoadEvent = false;
        }
        if (m_hasPendingLoadEvent) {
            loadEventSender().cancelEvent(this);
            m_hasPendingLoadEvent = false;
        }
        if (m_hasPendingErrorEvent) {
            errorEventSender().cancelEvent(this);
            m_hasPendingErrorEvent = false;
        }
        m_imageComplete = true;
        if (newImage)
            newImage->addClient(this);
        if (oldImage)
            oldImage->removeClient(this);
    }

    if (RenderImageResource* imageResource = renderImageResource())
        imageResource->resetAnimation();
}

void ImageLoader::updateFromElement()
{
    // If we're not making renderers for the page, then don't load images.  We don't want to slow
    // down the raw HTML parsing case by loading images we don't intend to display.
    if (!document()->renderer())
        return;

    AtomicString attr = client()->sourceElement()->getAttribute(client()->sourceElement()->imageSourceAttributeName());

    if (attr == m_failedLoadURL)
        return;

    // Do not load any image if the 'src' attribute is missing or if it is
    // an empty string.
    CachedResourceHandle<CachedImage> newImage = 0;
    if (!attr.isNull() && !stripLeadingAndTrailingHTMLSpaces(attr).isEmpty()) {
        ResourceRequest request = ResourceRequest(document()->completeURL(sourceURI(attr)));

        String crossOriginMode = client()->sourceElement()->fastGetAttribute(HTMLNames::crossoriginAttr);
        if (!crossOriginMode.isNull()) {
            StoredCredentials allowCredentials = equalIgnoringCase(crossOriginMode, "use-credentials") ? AllowStoredCredentials : DoNotAllowStoredCredentials;
            updateRequestForAccessControl(request, document()->securityOrigin(), allowCredentials);
        }

        if (m_loadManually) {
            bool autoLoadOtherImages = document()->cachedResourceLoader()->autoLoadImages();
            document()->cachedResourceLoader()->setAutoLoadImages(false);
            newImage = new CachedImage(request);
            newImage->setLoading(true);
            newImage->setOwningCachedResourceLoader(document()->cachedResourceLoader());
            document()->cachedResourceLoader()->m_documentResources.set(newImage->url(), newImage.get());
            document()->cachedResourceLoader()->setAutoLoadImages(autoLoadOtherImages);
        } else
            newImage = document()->cachedResourceLoader()->requestImage(request);

        // If we do not have an image here, it means that a cross-site
        // violation occurred, or that the image was blocked via Content
        // Security Policy. Either way, trigger an error event.
        if (!newImage) {
            m_failedLoadURL = attr;
            m_hasPendingErrorEvent = true;
            errorEventSender().dispatchEventSoon(this);
        } else
            m_failedLoadURL = AtomicString();
    } else if (!attr.isNull()) {
        // Fire an error event if the url is empty.
        // FIXME: Should we fire this event asynchronoulsy via errorEventSender()?
        client()->imageElement()->dispatchEvent(Event::create(eventNames().errorEvent, false, false));
    }
    
    CachedImage* oldImage = m_image.get();
    if (newImage != oldImage) {
        if (m_hasPendingBeforeLoadEvent)
            beforeLoadEventSender().cancelEvent(this);
        if (m_hasPendingLoadEvent)
            loadEventSender().cancelEvent(this);
        if (m_hasPendingErrorEvent)
            errorEventSender().cancelEvent(this);

        m_image = newImage;
        m_hasPendingBeforeLoadEvent = !document()->isImageDocument() && newImage;
        m_hasPendingLoadEvent = newImage;
        m_imageComplete = !newImage;

        if (newImage) {
            if (!document()->isImageDocument()) {
                if (!document()->hasListenerType(Document::BEFORELOAD_LISTENER))
                    dispatchPendingBeforeLoadEvent();
                else
                    beforeLoadEventSender().dispatchEventSoon(this);
            } else
                updateRenderer();

            // If newImage is cached, addClient() will result in the load event
            // being queued to fire. Ensure this happens after beforeload is
            // dispatched.
            newImage->addClient(this);
        }
        if (oldImage)
            oldImage->removeClient(this);
    }

    if (RenderImageResource* imageResource = renderImageResource())
        imageResource->resetAnimation();

    // Only consider updating the protection ref-count of the Element immediately before returning
    // from this function as doing so might result in the destruction of this ImageLoader.
    updatedHasPendingLoadEvent();
}

void ImageLoader::updateFromElementIgnoringPreviousError()
{
    // Clear previous error.
    m_failedLoadURL = AtomicString();
    updateFromElement();
}

void ImageLoader::notifyFinished(CachedResource* resource)
{
    ASSERT(m_failedLoadURL.isEmpty());
    ASSERT(resource == m_image.get());

    m_imageComplete = true;
    if (!hasPendingBeforeLoadEvent())
        updateRenderer();

    if (!m_hasPendingLoadEvent)
        return;

    if (client()->sourceElement()->fastHasAttribute(HTMLNames::crossoriginAttr)
        && !document()->securityOrigin()->canRequest(image()->response().url())
        && !resource->passesAccessControlCheck(document()->securityOrigin())) {

        setImageWithoutConsideringPendingLoadEvent(0);

        m_hasPendingErrorEvent = true;
        errorEventSender().dispatchEventSoon(this);

        DEFINE_STATIC_LOCAL(String, consoleMessage, (ASCIILiteral("Cross-origin image load denied by Cross-Origin Resource Sharing policy.")));
        document()->addConsoleMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, consoleMessage);

        ASSERT(!m_hasPendingLoadEvent);

        // Only consider updating the protection ref-count of the Element immediately before returning
        // from this function as doing so might result in the destruction of this ImageLoader.
        updatedHasPendingLoadEvent();
        return;
    }

    if (resource->wasCanceled()) {
        m_hasPendingLoadEvent = false;
        // Only consider updating the protection ref-count of the Element immediately before returning
        // from this function as doing so might result in the destruction of this ImageLoader.
        updatedHasPendingLoadEvent();
        return;
    }

    loadEventSender().dispatchEventSoon(this);
}

RenderImageResource* ImageLoader::renderImageResource()
{
    RenderObject* renderer = client()->imageElement()->renderer();

    if (!renderer)
        return 0;

    // We don't return style generated image because it doesn't belong to the ImageLoader.
    // See <https://bugs.webkit.org/show_bug.cgi?id=42840>
    if (renderer->isImage() && !static_cast<RenderImage*>(renderer)->isGeneratedContent())
        return toRenderImage(renderer)->imageResource();

#if ENABLE(SVG)
    if (renderer->isSVGImage())
        return toRenderSVGImage(renderer)->imageResource();
#endif

#if ENABLE(VIDEO)
    if (renderer->isVideo())
        return toRenderVideo(renderer)->imageResource();
#endif

    return 0;
}

void ImageLoader::updateRenderer()
{
    RenderImageResource* imageResource = renderImageResource();

    if (!imageResource)
        return;

    // Only update the renderer if it doesn't have an image or if what we have
    // is a complete image.  This prevents flickering in the case where a dynamic
    // change is happening between two images.
    CachedImage* cachedImage = imageResource->cachedImage();
    if (m_image != cachedImage && (m_imageComplete || !cachedImage))
        imageResource->setCachedImage(m_image.get());
}

void ImageLoader::updatedHasPendingLoadEvent()
{
    // If an Element that does image loading is removed from the DOM the load event for the image is still observable.
    // As long as the ImageLoader is actively loading, the Element itself needs to be ref'ed to keep it from being
    // destroyed by DOM manipulation or garbage collection.
    // If such an Element wishes for the load to stop when removed from the DOM it needs to stop the ImageLoader explicitly.

    if (m_hasPendingLoadEvent == m_elementIsProtected)
        return;

    m_elementIsProtected = m_hasPendingLoadEvent;

    if (m_elementIsProtected)
        client()->refSourceElement();
    else
        client()->derefSourceElement();
}

void ImageLoader::dispatchPendingEvent(ImageEventSender* eventSender)
{
    ASSERT(eventSender == &beforeLoadEventSender() || eventSender == &loadEventSender() || eventSender == &errorEventSender());
    const AtomicString& eventType = eventSender->eventType();
    if (eventType == eventNames().beforeloadEvent)
        dispatchPendingBeforeLoadEvent();
    if (eventType == eventNames().loadEvent)
        dispatchPendingLoadEvent();
    if (eventType == eventNames().errorEvent)
        dispatchPendingErrorEvent();
}

void ImageLoader::dispatchPendingBeforeLoadEvent()
{
    if (!m_hasPendingBeforeLoadEvent)
        return;
    if (!m_image)
        return;
    if (!document()->attached())
        return;
    m_hasPendingBeforeLoadEvent = false;
    if (client()->sourceElement()->dispatchBeforeLoadEvent(m_image->url())) {
        updateRenderer();
        return;
    }
    if (m_image) {
        m_image->removeClient(this);
        m_image = 0;
    }

    loadEventSender().cancelEvent(this);
    m_hasPendingLoadEvent = false;

    if (client()->sourceElement()->hasTagName(HTMLNames::objectTag))
        static_cast<HTMLObjectElement*>(client()->sourceElement())->renderFallbackContent();

    // Only consider updating the protection ref-count of the Element immediately before returning
    // from this function as doing so might result in the destruction of this ImageLoader.
    updatedHasPendingLoadEvent();
}

void ImageLoader::dispatchPendingLoadEvent()
{
    if (!m_hasPendingLoadEvent)
        return;
    if (!m_image)
        return;
    if (!document()->attached())
        return;
    m_hasPendingLoadEvent = false;
    dispatchLoadEvent();

    // Only consider updating the protection ref-count of the Element immediately before returning
    // from this function as doing so might result in the destruction of this ImageLoader.
    updatedHasPendingLoadEvent();
}

void ImageLoader::dispatchPendingErrorEvent()
{
    if (!m_hasPendingErrorEvent)
        return;
    if (!document()->attached())
        return;
    m_hasPendingErrorEvent = false;
    client()->imageElement()->dispatchEvent(Event::create(eventNames().errorEvent, false, false));
}

void ImageLoader::dispatchPendingBeforeLoadEvents()
{
    beforeLoadEventSender().dispatchPendingEvents();
}

void ImageLoader::dispatchPendingLoadEvents()
{
    loadEventSender().dispatchPendingEvents();
}

void ImageLoader::dispatchPendingErrorEvents()
{
    errorEventSender().dispatchPendingEvents();
}

void ImageLoader::elementDidMoveToNewDocument()
{
    setImage(0);
}

}
