/*
 * Copyright (C) 2003, 2004, 2005, 2007, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright 2010, The Android Open Source Project
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

#ifndef JavaMethodJSC_h
#define JavaMethodJSC_h

#if ENABLE(JAVA_BRIDGE)

#include "Bridge.h"
#include "JavaStringJSC.h"
#include "JavaType.h"

namespace JSC {

namespace Bindings {

typedef const char* RuntimeType;

class JavaMethod : public Method {
public:
    JavaMethod(JNIEnv*, jobject aMethod);
    ~JavaMethod();

    const String name() const { return m_name.impl(); }
    RuntimeType returnTypeClassName() const { return m_returnTypeClassName.utf8(); }
    const String parameterAt(int i) const { return m_parameters[i]; }
    const char* signature() const;
    JavaType returnType() const { return m_returnType; }
    bool isStatic() const { return m_isStatic; }

    // Method implementation
    int numParameters() const { return m_parameters.size(); }

private:
    Vector<WTF::String> m_parameters;
    JavaString m_name;
    mutable char* m_signature;
    JavaString m_returnTypeClassName;
    JavaType m_returnType;
    bool m_isStatic;
};

} // namespace Bindings

} // namespace JSC

#endif // ENABLE(JAVA_BRIDGE)

#endif // JavaMethodJSC_h
