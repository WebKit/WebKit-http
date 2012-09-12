/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#if USE(3D_GRAPHICS)

#include "ANGLEWebKitBridge.h"
#include <wtf/OwnArrayPtr.h>

namespace WebCore {

inline static int getValidationResultValue(const ShHandle compiler, ShShaderInfo shaderInfo)
{
    int value = -1;
    ShGetInfo(compiler, shaderInfo, &value);
    return value;
}

ANGLEWebKitBridge::ANGLEWebKitBridge(ShShaderOutput shaderOutput, ShShaderSpec shaderSpec)
    : builtCompilers(false)
    , m_fragmentCompiler(0)
    , m_vertexCompiler(0)
    , m_shaderOutput(shaderOutput)
    , m_shaderSpec(shaderSpec)
{
    // This is a no-op if it's already initialized.
    ShInitialize();
}

ANGLEWebKitBridge::~ANGLEWebKitBridge()
{
    cleanupCompilers();
}

void ANGLEWebKitBridge::cleanupCompilers()
{
    if (m_fragmentCompiler)
        ShDestruct(m_fragmentCompiler);
    m_fragmentCompiler = 0;
    if (m_vertexCompiler)
        ShDestruct(m_vertexCompiler);
    m_vertexCompiler = 0;

    builtCompilers = false;
}
    
void ANGLEWebKitBridge::setResources(ShBuiltInResources resources)
{
    // Resources are (possibly) changing - cleanup compilers if we had them already
    cleanupCompilers();
    
    m_resources = resources;
}

bool ANGLEWebKitBridge::validateShaderSource(const char* shaderSource, ANGLEShaderType shaderType, String& translatedShaderSource, String& shaderValidationLog, int extraCompileOptions)
{
    if (!builtCompilers) {
        m_fragmentCompiler = ShConstructCompiler(SH_FRAGMENT_SHADER, m_shaderSpec, m_shaderOutput, &m_resources);
        m_vertexCompiler = ShConstructCompiler(SH_VERTEX_SHADER, m_shaderSpec, m_shaderOutput, &m_resources);
        if (!m_fragmentCompiler || !m_vertexCompiler) {
            cleanupCompilers();
            return false;
        }

        builtCompilers = true;
    }
    
    ShHandle compiler;

    if (shaderType == SHADER_TYPE_VERTEX)
        compiler = m_vertexCompiler;
    else
        compiler = m_fragmentCompiler;

    const char* const shaderSourceStrings[] = { shaderSource };

    bool validateSuccess = ShCompile(compiler, shaderSourceStrings, 1, SH_OBJECT_CODE | extraCompileOptions);
    if (!validateSuccess) {
        int logSize = getValidationResultValue(compiler, SH_INFO_LOG_LENGTH);
        if (logSize > 1) {
            OwnArrayPtr<char> logBuffer = adoptArrayPtr(new char[logSize]);
            if (logBuffer) {
                ShGetInfoLog(compiler, logBuffer.get());
                shaderValidationLog = logBuffer.get();
            }
        }
        return false;
    }

    int translationLength = getValidationResultValue(compiler, SH_OBJECT_CODE_LENGTH);
    if (translationLength > 1) {
        OwnArrayPtr<char> translationBuffer = adoptArrayPtr(new char[translationLength]);
        if (!translationBuffer)
            return false;
        ShGetObjectCode(compiler, translationBuffer.get());
        translatedShaderSource = translationBuffer.get();
    }

    return true;
}

bool ANGLEWebKitBridge::getUniforms(ShShaderType shaderType, Vector<ANGLEShaderSymbol> &symbols)
{
    const ShHandle compiler = (shaderType == SH_VERTEX_SHADER ? m_vertexCompiler : m_fragmentCompiler);

    int numUniforms = getValidationResultValue(compiler, SH_ACTIVE_UNIFORMS);
    if (numUniforms < 0)
        return false;
    if (!numUniforms)
        return true;

    int maxNameLength = getValidationResultValue(compiler, SH_ACTIVE_UNIFORM_MAX_LENGTH);
    if (maxNameLength <= 1)
        return false;
    OwnArrayPtr<char> nameBuffer = adoptArrayPtr(new char[maxNameLength]);

    for (int i = 0; i < numUniforms; ++i) {
        ANGLEShaderSymbol symbol;
        symbol.symbolType = SHADER_SYMBOL_TYPE_UNIFORM;
        int nameLength = -1;
        ShGetActiveUniform(compiler, i, &nameLength, &symbol.size, &symbol.dataType, nameBuffer.get(), 0);
        if (nameLength <= 0)
            return false;
        symbol.name = String::fromUTF8(nameBuffer.get(), nameLength);
        symbols.append(symbol);
    }

    return true;
}

}

#endif // USE(3D_GRAPHICS)
