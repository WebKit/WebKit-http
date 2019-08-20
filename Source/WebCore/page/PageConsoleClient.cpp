/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PageConsoleClient.h"

#include "CachedImage.h"
#include "CanvasRenderingContext2D.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "ElementChildIterator.h"
#include "Frame.h"
#include "FrameSnapshotting.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "HTMLPictureElement.h"
#include "HTMLVideoElement.h"
#include "Image.h"
#include "ImageBitmap.h"
#include "ImageBitmapRenderingContext.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "InspectorController.h"
#include "InspectorInstrumentation.h"
#include "IntRect.h"
#include "JSCanvasRenderingContext2D.h"
#include "JSExecState.h"
#include "JSHTMLCanvasElement.h"
#include "JSImageBitmap.h"
#include "JSImageBitmapRenderingContext.h"
#include "JSImageData.h"
#include "JSNode.h"
#include "JSOffscreenCanvas.h"
#include "Node.h"
#include "OffscreenCanvas.h"
#include "Page.h"
#include "ScriptableDocumentParser.h"
#include "Settings.h"
#include <JavaScriptCore/ConsoleMessage.h>
#include <JavaScriptCore/JSCInlines.h>
#include <JavaScriptCore/ScriptArguments.h>
#include <JavaScriptCore/ScriptCallStack.h>
#include <JavaScriptCore/ScriptCallStackFactory.h>
#include <wtf/text/WTFString.h>

#if ENABLE(WEBGL)
#include "JSWebGLRenderingContext.h"
#include "WebGLRenderingContext.h"
#endif

#if ENABLE(WEBGL2)
#include "JSWebGL2RenderingContext.h"
#include "WebGL2RenderingContext.h"
#endif

namespace WebCore {
using namespace Inspector;

PageConsoleClient::PageConsoleClient(Page& page)
    : m_page(page)
{
}

PageConsoleClient::~PageConsoleClient() = default;

static int muteCount = 0;
static bool printExceptions = false;

bool PageConsoleClient::shouldPrintExceptions()
{
    return printExceptions;
}

void PageConsoleClient::setShouldPrintExceptions(bool print)
{
    printExceptions = print;
}

void PageConsoleClient::mute()
{
    muteCount++;
}

void PageConsoleClient::unmute()
{
    ASSERT(muteCount > 0);
    muteCount--;
}

void PageConsoleClient::addMessage(std::unique_ptr<Inspector::ConsoleMessage>&& consoleMessage)
{
    if (!m_page.usesEphemeralSession()) {
        String message;
        if (consoleMessage->type() == MessageType::Image) {
            ASSERT(consoleMessage->arguments());
            consoleMessage->arguments()->getFirstArgumentAsString(message);
        } else
            message = consoleMessage->message();
        m_page.chrome().client().addMessageToConsole(consoleMessage->source(), consoleMessage->level(), message, consoleMessage->line(), consoleMessage->column(), consoleMessage->url());

        if (UNLIKELY(m_page.settings().logsPageMessagesToSystemConsoleEnabled() || shouldPrintExceptions())) {
            if (consoleMessage->type() == MessageType::Image) {
                ASSERT(consoleMessage->arguments());
                ConsoleClient::printConsoleMessageWithArguments(MessageSource::ConsoleAPI, MessageType::Log, consoleMessage->level(), consoleMessage->arguments()->globalState(), *consoleMessage->arguments());
            } else
                ConsoleClient::printConsoleMessage(MessageSource::ConsoleAPI, MessageType::Log, consoleMessage->level(), consoleMessage->message(), consoleMessage->url(), consoleMessage->line(), consoleMessage->column());
        }
    }

    InspectorInstrumentation::addMessageToConsole(m_page, WTFMove(consoleMessage));
}

void PageConsoleClient::addMessage(MessageSource source, MessageLevel level, const String& message, unsigned long requestIdentifier, Document* document)
{
    String url;
    unsigned line = 0;
    unsigned column = 0;
    if (document)
        document->getParserLocation(url, line, column);

    addMessage(source, level, message, url, line, column, 0, JSExecState::currentState(), requestIdentifier);
}

void PageConsoleClient::addMessage(MessageSource source, MessageLevel level, const String& message, Ref<ScriptCallStack>&& callStack)
{
    addMessage(source, level, message, String(), 0, 0, WTFMove(callStack), 0);
}

void PageConsoleClient::addMessage(MessageSource source, MessageLevel level, const String& messageText, const String& suggestedURL, unsigned suggestedLineNumber, unsigned suggestedColumnNumber, RefPtr<ScriptCallStack>&& callStack, JSC::ExecState* state, unsigned long requestIdentifier)
{
    if (muteCount && source != MessageSource::ConsoleAPI)
        return;

    std::unique_ptr<Inspector::ConsoleMessage> message;

    if (callStack)
        message = std::make_unique<Inspector::ConsoleMessage>(source, MessageType::Log, level, messageText, callStack.releaseNonNull(), requestIdentifier);
    else
        message = std::make_unique<Inspector::ConsoleMessage>(source, MessageType::Log, level, messageText, suggestedURL, suggestedLineNumber, suggestedColumnNumber, state, requestIdentifier);

    addMessage(WTFMove(message));
}


void PageConsoleClient::messageWithTypeAndLevel(MessageType type, MessageLevel level, JSC::ExecState* exec, Ref<Inspector::ScriptArguments>&& arguments)
{
    String messageText;
    bool gotMessage = arguments->getFirstArgumentAsString(messageText);

    auto message = std::make_unique<Inspector::ConsoleMessage>(MessageSource::ConsoleAPI, type, level, messageText, arguments.copyRef(), exec);

    String url = message->url();
    unsigned lineNumber = message->line();
    unsigned columnNumber = message->column();

    InspectorInstrumentation::addMessageToConsole(m_page, WTFMove(message));

    if (m_page.usesEphemeralSession())
        return;

    if (gotMessage)
        m_page.chrome().client().addMessageToConsole(MessageSource::ConsoleAPI, level, messageText, lineNumber, columnNumber, url);

    if (m_page.settings().logsPageMessagesToSystemConsoleEnabled() || PageConsoleClient::shouldPrintExceptions())
        ConsoleClient::printConsoleMessageWithArguments(MessageSource::ConsoleAPI, type, level, exec, WTFMove(arguments));
}

void PageConsoleClient::count(JSC::ExecState* exec, const String& label)
{
    InspectorInstrumentation::consoleCount(m_page, exec, label);
}

void PageConsoleClient::countReset(JSC::ExecState* exec, const String& label)
{
    InspectorInstrumentation::consoleCountReset(m_page, exec, label);
}

void PageConsoleClient::profile(JSC::ExecState* exec, const String& title)
{
    // FIXME: <https://webkit.org/b/153499> Web Inspector: console.profile should use the new Sampling Profiler
    InspectorInstrumentation::startProfiling(m_page, exec, title);
}

void PageConsoleClient::profileEnd(JSC::ExecState* exec, const String& title)
{
    // FIXME: <https://webkit.org/b/153499> Web Inspector: console.profile should use the new Sampling Profiler
    InspectorInstrumentation::stopProfiling(m_page, exec, title);
}

void PageConsoleClient::takeHeapSnapshot(JSC::ExecState*, const String& title)
{
    InspectorInstrumentation::takeHeapSnapshot(m_page.mainFrame(), title);
}

void PageConsoleClient::time(JSC::ExecState* exec, const String& label)
{
    InspectorInstrumentation::startConsoleTiming(m_page.mainFrame(), exec, label);
}

void PageConsoleClient::timeLog(JSC::ExecState* exec, const String& label, Ref<ScriptArguments>&& arguments)
{
    InspectorInstrumentation::logConsoleTiming(m_page.mainFrame(), exec, label, WTFMove(arguments));
}

void PageConsoleClient::timeEnd(JSC::ExecState* exec, const String& label)
{
    InspectorInstrumentation::stopConsoleTiming(m_page.mainFrame(), exec, label);
}

void PageConsoleClient::timeStamp(JSC::ExecState*, Ref<ScriptArguments>&& arguments)
{
    InspectorInstrumentation::consoleTimeStamp(m_page.mainFrame(), WTFMove(arguments));
}

static JSC::JSObject* objectArgumentAt(ScriptArguments& arguments, unsigned index)
{
    return arguments.argumentCount() > index ? arguments.argumentAt(index).getObject() : nullptr;
}

static CanvasRenderingContext* canvasRenderingContext(JSC::VM& vm, JSC::JSValue target)
{
    if (auto* canvas = JSHTMLCanvasElement::toWrapped(vm, target))
        return canvas->renderingContext();
    if (auto* canvas = JSOffscreenCanvas::toWrapped(vm, target))
        return canvas->renderingContext();
    if (auto* context = JSCanvasRenderingContext2D::toWrapped(vm, target))
        return context;
    if (auto* context = JSImageBitmapRenderingContext::toWrapped(vm, target))
        return context;
#if ENABLE(WEBGL)
    if (auto* context = JSWebGLRenderingContext::toWrapped(vm, target))
        return context;
#endif
#if ENABLE(WEBGL2)
    if (auto* context = JSWebGL2RenderingContext::toWrapped(vm, target))
        return context;
#endif
    return nullptr;
}

void PageConsoleClient::record(JSC::ExecState* state, Ref<ScriptArguments>&& arguments)
{
    if (auto* target = objectArgumentAt(arguments, 0)) {
        if (auto* context = canvasRenderingContext(state->vm(), target))
            InspectorInstrumentation::consoleStartRecordingCanvas(*context, *state, objectArgumentAt(arguments, 1));
    }
}

void PageConsoleClient::recordEnd(JSC::ExecState* state, Ref<ScriptArguments>&& arguments)
{
    if (auto* target = objectArgumentAt(arguments, 0)) {
        if (auto* context = canvasRenderingContext(state->vm(), target))
            InspectorInstrumentation::didFinishRecordingCanvasFrame(*context, true);
    }
}

void PageConsoleClient::screenshot(JSC::ExecState* state, Ref<ScriptArguments>&& arguments)
{
    String dataURL;
    JSC::JSValue target;

    if (arguments->argumentCount()) {
        auto possibleTarget = arguments->argumentAt(0);

        if (auto* node = JSNode::toWrapped(state->vm(), possibleTarget)) {
            target = possibleTarget;
            if (UNLIKELY(InspectorInstrumentation::hasFrontends())) {
                std::unique_ptr<ImageBuffer> snapshot;

                // Only try to do something special for subclasses of Node if they're detached from the DOM tree.
                if (!node->document().contains(node)) {
                    auto snapshotImageElement = [&snapshot] (HTMLImageElement& imageElement) {
                        if (auto* cachedImage = imageElement.cachedImage()) {
                            auto* image = cachedImage->image();
                            if (image && image != &Image::nullImage()) {
                                snapshot = ImageBuffer::create(image->size(), RenderingMode::Unaccelerated);
                                snapshot->context().drawImage(*image, FloatPoint(0, 0));
                            }
                        }
                    };

                    if (is<HTMLImageElement>(node))
                        snapshotImageElement(downcast<HTMLImageElement>(*node));
                    else if (is<HTMLPictureElement>(node)) {
                        if (auto* firstImage = childrenOfType<HTMLImageElement>(downcast<HTMLPictureElement>(*node)).first())
                            snapshotImageElement(*firstImage);
                    }
#if ENABLE(VIDEO)
                    else if (is<HTMLVideoElement>(node)) {
                        auto& videoElement = downcast<HTMLVideoElement>(*node);
                        unsigned videoWidth = videoElement.videoWidth();
                        unsigned videoHeight = videoElement.videoHeight();
                        snapshot = ImageBuffer::create(FloatSize(videoWidth, videoHeight), RenderingMode::Unaccelerated);
                        videoElement.paintCurrentFrameInContext(snapshot->context(), FloatRect(0, 0, videoWidth, videoHeight));
                    }
#endif
                }

                if (!snapshot)
                    snapshot = WebCore::snapshotNode(m_page.mainFrame(), *node);

                if (snapshot)
                    dataURL = snapshot->toDataURL("image/png"_s, WTF::nullopt, PreserveResolution::Yes);
            }
        } else if (auto* imageData = JSImageData::toWrapped(state->vm(), possibleTarget)) {
            target = possibleTarget;
            if (UNLIKELY(InspectorInstrumentation::hasFrontends())) {
                auto sourceSize = imageData->size();
                if (auto imageBuffer = ImageBuffer::create(sourceSize, RenderingMode::Unaccelerated)) {
                    IntRect sourceRect(IntPoint(), sourceSize);
                    imageBuffer->putByteArray(*imageData->data(), AlphaPremultiplication::Unpremultiplied, sourceSize, sourceRect, IntPoint());
                    dataURL = imageBuffer->toDataURL("image/png"_s, WTF::nullopt, PreserveResolution::Yes);
                }
            }
        } else if (auto* imageBitmap = JSImageBitmap::toWrapped(state->vm(), possibleTarget)) {
            target = possibleTarget;
            if (UNLIKELY(InspectorInstrumentation::hasFrontends())) {
                if (auto* imageBuffer = imageBitmap->buffer())
                    dataURL = imageBuffer->toDataURL("image/png"_s, WTF::nullopt, PreserveResolution::Yes);
            }
        } else if (auto* context = canvasRenderingContext(state->vm(), possibleTarget)) {
            auto& canvas = context->canvasBase();
            if (is<HTMLCanvasElement>(canvas)) {
                target = possibleTarget;
                if (UNLIKELY(InspectorInstrumentation::hasFrontends())) {
#if ENABLE(WEBGL)
                    if (is<WebGLRenderingContextBase>(context))
                        downcast<WebGLRenderingContextBase>(context)->setPreventBufferClearForInspector(true);
#endif

                    auto result = downcast<HTMLCanvasElement>(canvas).toDataURL("image/png"_s);

#if ENABLE(WEBGL)
                    if (is<WebGLRenderingContextBase>(context))
                        downcast<WebGLRenderingContextBase>(context)->setPreventBufferClearForInspector(false);
#endif

                    if (!result.hasException())
                        dataURL = result.releaseReturnValue().string;
                }
            }

            // FIXME: <https://webkit.org/b/180833> Web Inspector: support OffscreenCanvas for Canvas related operations
        }
    }

    if (UNLIKELY(InspectorInstrumentation::hasFrontends())) {
        if (!target) {
            // If no target is provided, capture an image of the viewport.
            IntRect imageRect(IntPoint::zero(), m_page.mainFrame().view()->sizeForVisibleContent());
            if (auto snapshot = WebCore::snapshotFrameRect(m_page.mainFrame(), imageRect, SnapshotOptionsInViewCoordinates))
                dataURL = snapshot->toDataURL("image/png"_s, WTF::nullopt, PreserveResolution::Yes);
        }

        if (dataURL.isEmpty()) {
            addMessage(std::make_unique<Inspector::ConsoleMessage>(MessageSource::ConsoleAPI, MessageType::Image, MessageLevel::Error, "Could not capture screenshot"_s, WTFMove(arguments)));
            return;
        }
    }

    Vector<JSC::Strong<JSC::Unknown>> adjustedArguments;
    adjustedArguments.append({ state->vm(), target ? target : JSC::jsNontrivialString(state, "Viewport"_s) });
    for (size_t i = (!target ? 0 : 1); i < arguments->argumentCount(); ++i)
        adjustedArguments.append({ state->vm(), arguments->argumentAt(i) });
    arguments = ScriptArguments::create(*state, WTFMove(adjustedArguments));
    addMessage(std::make_unique<Inspector::ConsoleMessage>(MessageSource::ConsoleAPI, MessageType::Image, MessageLevel::Log, dataURL, WTFMove(arguments)));
}

} // namespace WebCore
