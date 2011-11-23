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

#include "TestShell.h"

using namespace WebKit;

GamepadController::GamepadController(TestShell* shell)
    : m_shell(shell)
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

void GamepadController::reset()
{
    memset(&internalData, 0, sizeof(internalData));
}

void GamepadController::connect(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 1) {
        printf("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    internalData.items[index].connected = true;
    internalData.length = 0;
    for (unsigned i = 0; i < WebKit::WebGamepads::itemsLengthCap; ++i)
        if (internalData.items[i].connected)
            internalData.length = i + 1;
    webkit_support::SetGamepadData(internalData);
    result->setNull();
}

void GamepadController::disconnect(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 1) {
        printf("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    internalData.items[index].connected = false;
    internalData.length = 0;
    for (unsigned i = 0; i < WebKit::WebGamepads::itemsLengthCap; ++i)
        if (internalData.items[i].connected)
            internalData.length = i + 1;
    webkit_support::SetGamepadData(internalData);
    result->setNull();
}

void GamepadController::setId(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 2) {
        printf("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    std::string src = args[1].toString();
    const char* p = src.c_str();
    memset(internalData.items[index].id, 0, sizeof(internalData.items[index].id));
    for (unsigned i = 0; *p && i < WebKit::WebGamepad::idLengthCap - 1; ++i)
        internalData.items[index].id[i] = *p++;
    webkit_support::SetGamepadData(internalData);
    result->setNull();
}

void GamepadController::setButtonCount(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 2) {
        printf("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    int buttons = args[1].toInt32();
    internalData.items[index].buttonsLength = buttons;
    webkit_support::SetGamepadData(internalData);
    result->setNull();
}

void GamepadController::setButtonData(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 3) {
        printf("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    int button = args[1].toInt32();
    double data = args[2].toDouble();
    internalData.items[index].buttons[button] = data;
    webkit_support::SetGamepadData(internalData);
    result->setNull();
}

void GamepadController::setAxisCount(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 2) {
        printf("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    int axes = args[1].toInt32();
    internalData.items[index].axesLength = axes;
    webkit_support::SetGamepadData(internalData);
    result->setNull();
}

void GamepadController::setAxisData(const CppArgumentList& args, CppVariant* result)
{
    if (args.size() < 3) {
        printf("Invalid args");
        return;
    }
    int index = args[0].toInt32();
    int axis = args[1].toInt32();
    double data = args[2].toDouble();
    internalData.items[index].axes[axis] = data;
    webkit_support::SetGamepadData(internalData);
    result->setNull();
}

void GamepadController::fallbackCallback(const CppArgumentList&, CppVariant* result)
{
    printf("CONSOLE MESSAGE: JavaScript ERROR: unknown method called on "
           "GamepadController\n");
    result->setNull();
}
