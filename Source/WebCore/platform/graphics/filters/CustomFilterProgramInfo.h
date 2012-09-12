/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef CustomFilterProgramInfo_h
#define CustomFilterProgramInfo_h

#if ENABLE(CSS_SHADERS)
#include "GraphicsTypes.h"

#include <wtf/HashTraits.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

enum CustomFilterProgramType {
    PROGRAM_TYPE_NO_ELEMENT_TEXTURE,
    PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE
};

struct CustomFilterProgramMixSettings {
    CustomFilterProgramMixSettings()
        : enabled(false)
        , blendMode(BlendModeNormal)
        , compositeOperator(CompositeSourceOver)
    {
    }
    
    bool operator==(const CustomFilterProgramMixSettings& o) const
    {
        return (!enabled && !o.enabled)
            || (blendMode == o.blendMode && compositeOperator == o.compositeOperator);
    }
    
    bool enabled;
    BlendMode blendMode;
    CompositeOperator compositeOperator;
};

// CustomFilterProgramInfo is the key used to link CustomFilterProgram with CustomFilterCompiledProgram.
// It can be used as a key in a HashMap, with the note that at least one of Strings needs to be non-null. 
// Null strings are placeholders for the default shader.
class CustomFilterProgramInfo {
public:
    CustomFilterProgramInfo(const String&, const String&, const CustomFilterProgramMixSettings&);

    CustomFilterProgramInfo();
    bool isEmptyValue() const;

    CustomFilterProgramInfo(WTF::HashTableDeletedValueType);
    bool isHashTableDeletedValue() const;
    
    unsigned hash() const;
    bool operator==(const CustomFilterProgramInfo&) const;

    const String& vertexShaderString() const { return m_vertexShaderString; }
    const String& fragmentShaderString() const { return m_fragmentShaderString; }
    // FIXME: We should add CustomFilterProgramType to CustomFilterProgramInfo and remove mixSettings.enabled.
    // https://bugs.webkit.org/show_bug.cgi?id=96448
    CustomFilterProgramType programType() const { return m_mixSettings.enabled ? PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE : PROGRAM_TYPE_NO_ELEMENT_TEXTURE; }
    const CustomFilterProgramMixSettings& mixSettings() const { return m_mixSettings; }
private:
    String m_vertexShaderString;
    String m_fragmentShaderString;
    CustomFilterProgramMixSettings m_mixSettings;
};

struct CustomFilterProgramInfoHash {
    static unsigned hash(const CustomFilterProgramInfo& programInfo) { return programInfo.hash(); }
    static bool equal(const CustomFilterProgramInfo& a, const CustomFilterProgramInfo& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = false;
};

struct CustomFilterProgramInfoHashTraits : WTF::SimpleClassHashTraits<CustomFilterProgramInfo> {
    static const bool hasIsEmptyValueFunction = true;
    static bool isEmptyValue(const CustomFilterProgramInfo& info) { return info.isEmptyValue(); }
};

} // namespace WebCore

namespace WTF {

template<> struct HashTraits<WebCore::CustomFilterProgramInfo> : WebCore::CustomFilterProgramInfoHashTraits { };
template<> struct DefaultHash<WebCore::CustomFilterProgramInfo> { 
    typedef WebCore::CustomFilterProgramInfoHash Hash; 
};

}
#endif // ENABLE(CSS_SHADERS)

#endif // CustomFilterProgramInfo_h
