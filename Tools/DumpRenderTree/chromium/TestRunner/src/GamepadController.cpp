/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "GamepadController.h"
#include "TestDelegate.h"

using namespace WebKit;

namespace WebTestRunner {

GamepadController::GamepadController()
{
    bindMethod("connect", &GamepadController::connect);
    bindMethod("disconnect", &GamepadController::disconnect);
    bindMethod("setId", &GamepadController::setId);
    bindMethod("setButtonCount", &GamepadController::setButtonCount);
    bindMethod("setButtonData", &GamepadController::setButtonData);
    bindMethod("setAxisCount", &GamepadController::setAxisCount);
    bindMethod("setAxisData", &GamepadController::setAxisData);

    bindFallbackMethod(&GamepadController::fallbackCallback);

    reset();
}

void GamepadController::bindToJavascript(WebFrame* frame, const WebString& classname)
{
    CppBoundClass::bindToJavascript(frame, classname);
}

void GamepadController::setDelegate(TestDelegate* delegate)
{
    m_delegate = delegate;
}

void GamepadController::reset()
{
    memset(&m_gamepads, 0, sizeof(m_gamepads));
}

void GamepadController::connect(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 1) {
        m_delegate->printMessage("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    if (index < 0 || index >= static_cast<int>(WebKit::WebGamepads::itemsLengthCap))
        return;
    m_gamepads.items[index].connected = true;
    m_gamepads.length = 0;
    for (unsigned i = 0; i < WebKit::WebGamepads::itemsLengthCap; ++i)
        if (m_gamepads.items[i].connected)
            m_gamepads.length = i + 1;
    m_delegate->setGamepadData(m_gamepads);
    result->setNull();
}

void GamepadController::disconnect(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 1) {
        m_delegate->printMessage("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    if (index < 0 || index >= static_cast<int>(WebKit::WebGamepads::itemsLengthCap))
        return;
    m_gamepads.items[index].connected = false;
    m_gamepads.length = 0;
    for (unsigned i = 0; i < WebKit::WebGamepads::itemsLengthCap; ++i)
        if (m_gamepads.items[i].connected)
            m_gamepads.length = i + 1;
    m_delegate->setGamepadData(m_gamepads);
    result->setNull();
}

void GamepadController::setId(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 2) {
        m_delegate->printMessage("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    if (index < 0 || index >= static_cast<int>(WebKit::WebGamepads::itemsLengthCap))
        return;
    std::string src = args[1].toString();
    const char* p = src.c_str();
    memset(m_gamepads.items[index].id, 0, sizeof(m_gamepads.items[index].id));
    for (unsigned i = 0; *p && i < WebKit::WebGamepad::idLengthCap - 1; ++i)
        m_gamepads.items[index].id[i] = *p++;
    m_delegate->setGamepadData(m_gamepads);
    result->setNull();
}

void GamepadController::setButtonCount(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 2) {
        m_delegate->printMessage("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    if (index < 0 || index >= static_cast<int>(WebKit::WebGamepads::itemsLengthCap))
        return;
    int buttons = args[1].toInt32();
    if (buttons < 0 || buttons >= static_cast<int>(WebKit::WebGamepad::buttonsLengthCap))
        return;
    m_gamepads.items[index].buttonsLength = buttons;
    m_delegate->setGamepadData(m_gamepads);
    result->setNull();
}

void GamepadController::setButtonData(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 3) {
        m_delegate->printMessage("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    if (index < 0 || index >= static_cast<int>(WebKit::WebGamepads::itemsLengthCap))
        return;
    int button = args[1].toInt32();
    if (button < 0 || button >= static_cast<int>(WebKit::WebGamepad::buttonsLengthCap))
        return;
    double data = args[2].toDouble();
    m_gamepads.items[index].buttons[button] = data;
    m_delegate->setGamepadData(m_gamepads);
    result->setNull();
}

void GamepadController::setAxisCount(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 2) {
        m_delegate->printMessage("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    if (index < 0 || index >= static_cast<int>(WebKit::WebGamepads::itemsLengthCap))
        return;
    int axes = args[1].toInt32();
    if (axes < 0 || axes >= static_cast<int>(WebKit::WebGamepad::axesLengthCap))
        return;
    m_gamepads.items[index].axesLength = axes;
    m_delegate->setGamepadData(m_gamepads);
    result->setNull();
}

void GamepadController::setAxisData(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 3) {
        m_delegate->printMessage("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    if (index < 0 || index >= static_cast<int>(WebKit::WebGamepads::itemsLengthCap))
        return;
    int axis = args[1].toInt32();
    if (axis < 0 || axis >= static_cast<int>(WebKit::WebGamepad::axesLengthCap))
        return;
    double data = args[2].toDouble();
    m_gamepads.items[index].axes[axis] = data;
    m_delegate->setGamepadData(m_gamepads);
    result->setNull();
}

void GamepadController::fallbackCallback(const CppArgumentList&, CppVariant* result)
{
    m_delegate->printMessage("CONSOLE MESSAGE: JavaScript ERROR: unknown method called on GamepadController\n");
    result->setNull();
}

}
