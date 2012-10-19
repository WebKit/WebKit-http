/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InspectorFrontendClientLocal.h"

#include "Chrome.h"
#include "Document.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "InspectorBackendDispatcher.h"
#include "InspectorController.h"
#include "InspectorFrontendHost.h"
#include "InspectorPageAgent.h"
#include "Page.h"
#include "ScriptFunctionCall.h"
#include "ScriptObject.h"
#include "ScriptState.h"
#include "Settings.h"
#include "Timer.h"
#include "UserGestureIndicator.h"
#include "WindowFeatures.h"
#include <wtf/Deque.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static const char* inspectorAttachedHeightSetting = "inspectorAttachedHeight";
static const unsigned defaultAttachedHeight = 300;
static const float minimumAttachedHeight = 250.0f;
static const float maximumAttachedHeightRatio = 0.75f;

class InspectorBackendDispatchTask {
    WTF_MAKE_FAST_ALLOCATED;
public:
    InspectorBackendDispatchTask(InspectorController* inspectorController)
        : m_inspectorController(inspectorController)
        , m_timer(this, &InspectorBackendDispatchTask::onTimer)
    {
    }

    void dispatch(const String& message)
    {
        m_messages.append(message);
        if (!m_timer.isActive())
            m_timer.startOneShot(0);
    }

    void reset()
    {
        m_messages.clear();
        m_timer.stop();
    }

    void onTimer(Timer<InspectorBackendDispatchTask>*)
    {
        if (!m_messages.isEmpty()) {
            // Dispatch can lead to the timer destruction -> schedule the next shot first.
            m_timer.startOneShot(0);
            m_inspectorController->dispatchMessageFromFrontend(m_messages.takeFirst());
        }
    }

private:
    InspectorController* m_inspectorController;
    Timer<InspectorBackendDispatchTask> m_timer;
    Deque<String> m_messages;
};

String InspectorFrontendClientLocal::Settings::getProperty(const String&)
{
    return String();
}

void InspectorFrontendClientLocal::Settings::setProperty(const String&, const String&)
{
}

InspectorFrontendClientLocal::InspectorFrontendClientLocal(InspectorController* inspectorController, Page* frontendPage, PassOwnPtr<Settings> settings)
    : m_inspectorController(inspectorController)
    , m_frontendPage(frontendPage)
    , m_settings(settings)
    , m_frontendLoaded(false)
{
    m_frontendPage->settings()->setAllowFileAccessFromFileURLs(true);
    m_dispatchTask = adoptPtr(new InspectorBackendDispatchTask(inspectorController));
}

InspectorFrontendClientLocal::~InspectorFrontendClientLocal()
{
    if (m_frontendHost)
        m_frontendHost->disconnectClient();
    m_frontendPage = 0;
    m_inspectorController = 0;
}

void InspectorFrontendClientLocal::windowObjectCleared()
{
    if (m_frontendHost)
        m_frontendHost->disconnectClient();
    
    ScriptState* frontendScriptState = scriptStateFromPage(debuggerWorld(), m_frontendPage);
    m_frontendHost = InspectorFrontendHost::create(this, m_frontendPage);
    ScriptGlobalObject::set(frontendScriptState, "InspectorFrontendHost", m_frontendHost.get());
}

void InspectorFrontendClientLocal::frontendLoaded()
{
    bringToFront();
    setDockingUnavailable(!canAttachWindow());
    m_frontendLoaded = true;
    for (Vector<String>::iterator it = m_evaluateOnLoad.begin(); it != m_evaluateOnLoad.end(); ++it)
        evaluateOnLoad(*it);
    m_evaluateOnLoad.clear();
}

void InspectorFrontendClientLocal::requestAttachWindow()
{
    if (!canAttachWindow())
        return;
    attachWindow();
    setAttachedWindow(true);
}

void InspectorFrontendClientLocal::requestDetachWindow()
{
    detachWindow();
    setAttachedWindow(false);
}

bool InspectorFrontendClientLocal::canAttachWindow()
{
    // Don't allow the attach if the window would be too small to accommodate the minimum inspector height.
    // Also don't allow attaching to another inspector -- two inspectors in one window is too much!
    bool isInspectorPage = m_inspectorController->inspectedPage()->inspectorController()->hasInspectorFrontendClient();
    unsigned inspectedPageHeight = m_inspectorController->inspectedPage()->mainFrame()->view()->visibleHeight();
    unsigned maximumAttachedHeight = inspectedPageHeight * maximumAttachedHeightRatio;
    return minimumAttachedHeight <= maximumAttachedHeight && !isInspectorPage;
}

void InspectorFrontendClientLocal::setDockingUnavailable(bool unavailable)
{
    evaluateOnLoad(String::format("[\"setDockingUnavailable\", %s]", unavailable ? "true" : "false"));
}

void InspectorFrontendClientLocal::changeAttachedWindowHeight(unsigned height)
{
    unsigned totalHeight = m_frontendPage->mainFrame()->view()->visibleHeight() + m_inspectorController->inspectedPage()->mainFrame()->view()->visibleHeight();
    unsigned attachedHeight = constrainedAttachedWindowHeight(height, totalHeight);
    m_settings->setProperty(inspectorAttachedHeightSetting, String::number(attachedHeight));
    setAttachedWindowHeight(attachedHeight);
}

void InspectorFrontendClientLocal::openInNewTab(const String& url)
{
    UserGestureIndicator indicator(DefinitelyProcessingUserGesture);
    Page* page = m_inspectorController->inspectedPage();
    Frame* mainFrame = page->mainFrame();
    FrameLoadRequest request(mainFrame->document()->securityOrigin(), ResourceRequest(), "_blank");

    bool created;
    WindowFeatures windowFeatures;
    Frame* frame = WebCore::createWindow(mainFrame, mainFrame, request, windowFeatures, created);
    if (!frame)
        return;

    frame->loader()->setOpener(mainFrame);
    frame->page()->setOpenedByDOM();

    // FIXME: Why does one use mainFrame and the other frame?
    frame->loader()->changeLocation(mainFrame->document()->securityOrigin(), frame->document()->completeURL(url), "", false, false);
}

void InspectorFrontendClientLocal::moveWindowBy(float x, float y)
{
    FloatRect frameRect = m_frontendPage->chrome()->windowRect();
    frameRect.move(x, y);
    m_frontendPage->chrome()->setWindowRect(frameRect);
}

void InspectorFrontendClientLocal::setAttachedWindow(bool attached)
{
    evaluateOnLoad(String::format("[\"setAttachedWindow\", %s]", attached ? "true" : "false"));
}

void InspectorFrontendClientLocal::restoreAttachedWindowHeight()
{
    unsigned inspectedPageHeight = m_inspectorController->inspectedPage()->mainFrame()->view()->visibleHeight();
    String value = m_settings->getProperty(inspectorAttachedHeightSetting);
    unsigned preferredHeight = value.isEmpty() ? defaultAttachedHeight : value.toUInt();
    
    // This call might not go through (if the window starts out detached), but if the window is initially created attached,
    // InspectorController::attachWindow is never called, so we need to make sure to set the attachedWindowHeight.
    // FIXME: Clean up code so we only have to call setAttachedWindowHeight in InspectorController::attachWindow
    setAttachedWindowHeight(constrainedAttachedWindowHeight(preferredHeight, inspectedPageHeight));
}

bool InspectorFrontendClientLocal::isDebuggingEnabled()
{
    if (m_frontendLoaded)
        return evaluateAsBoolean("[\"isDebuggingEnabled\"]");
    return false;
}

void InspectorFrontendClientLocal::setDebuggingEnabled(bool enabled)
{
    evaluateOnLoad(String::format("[\"setDebuggingEnabled\", %s]", enabled ? "true" : "false"));
}

bool InspectorFrontendClientLocal::isTimelineProfilingEnabled()
{
    if (m_frontendLoaded)
        return evaluateAsBoolean("[\"isTimelineProfilingEnabled\"]");
    return false;
}

void InspectorFrontendClientLocal::setTimelineProfilingEnabled(bool enabled)
{
    evaluateOnLoad(String::format("[\"setTimelineProfilingEnabled\", %s]", enabled ? "true" : "false"));
}

bool InspectorFrontendClientLocal::isProfilingJavaScript()
{
    if (m_frontendLoaded)
        return evaluateAsBoolean("[\"isProfilingJavaScript\"]");
    return false;
}

void InspectorFrontendClientLocal::startProfilingJavaScript()
{
    evaluateOnLoad("[\"startProfilingJavaScript\"]");
}

void InspectorFrontendClientLocal::stopProfilingJavaScript()
{
    evaluateOnLoad("[\"stopProfilingJavaScript\"]");
}

void InspectorFrontendClientLocal::showConsole()
{
    evaluateOnLoad("[\"showConsole\"]");
}

void InspectorFrontendClientLocal::showResources()
{
    evaluateOnLoad("[\"showResources\"]");
}

void InspectorFrontendClientLocal::showMainResourceForFrame(Frame* frame)
{
    String frameId = m_inspectorController->pageAgent()->frameId(frame);
    evaluateOnLoad(String::format("[\"showMainResourceForFrame\", \"%s\"]", frameId.ascii().data()));
}

unsigned InspectorFrontendClientLocal::constrainedAttachedWindowHeight(unsigned preferredHeight, unsigned totalWindowHeight)
{
    using namespace std;
    return roundf(max(minimumAttachedHeight, min<float>(preferredHeight, totalWindowHeight * maximumAttachedHeightRatio)));
}

void InspectorFrontendClientLocal::sendMessageToBackend(const String& message)
{
    m_dispatchTask->dispatch(message);
}

bool InspectorFrontendClientLocal::evaluateAsBoolean(const String& expression)
{
    if (!m_frontendPage->mainFrame())
        return false;
    ScriptValue value = m_frontendPage->mainFrame()->script()->executeScript(expression);
    return value.toString(mainWorldScriptState(m_frontendPage->mainFrame())) == "true";
}

void InspectorFrontendClientLocal::evaluateOnLoad(const String& expression)
{
    if (m_frontendLoaded)
        m_frontendPage->mainFrame()->script()->executeScript("InspectorFrontendAPI.dispatch(" + expression + ")");
    else
        m_evaluateOnLoad.append(expression);
}

} // namespace WebCore

#endif
