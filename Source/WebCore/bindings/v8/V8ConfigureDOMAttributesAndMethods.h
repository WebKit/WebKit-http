/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

#ifndef V8ConfigureDOMAttributesAndMethods_h
#define V8ConfigureDOMAttributesAndMethods_h

#include "V8DOMWrapper.h"
#include <v8.h>

namespace WebCore {

// The following Batch structs and methods are used for setting multiple
// properties on an ObjectTemplate, used from the generated bindings
// initialization (ConfigureXXXTemplate). This greatly reduces the binary
// size by moving from code driven setup to data table driven setup.

// BatchedAttribute translates into calls to SetAccessor() on either the
// instance or the prototype ObjectTemplate, based on |onProto|.
struct BatchedAttribute {
    const char* const name;
    v8::AccessorGetter getter;
    v8::AccessorSetter setter;
    WrapperTypeInfo* data;
    v8::AccessControl settings;
    v8::PropertyAttribute attribute;
    bool onProto;
};

void batchConfigureAttributes(v8::Handle<v8::ObjectTemplate>,
                              v8::Handle<v8::ObjectTemplate>,
                              const BatchedAttribute*,
                              size_t attributeCount);

template<class ObjectOrTemplate>
inline void configureAttribute(v8::Handle<ObjectOrTemplate> instance,
                               v8::Handle<ObjectOrTemplate> proto,
                               const BatchedAttribute& attribute)
{
    (attribute.onProto ? proto : instance)->SetAccessor(v8::String::New(attribute.name),
                                                        attribute.getter,
                                                        attribute.setter,
                                                        v8::External::Wrap(attribute.data),
                                                        attribute.settings,
                                                        attribute.attribute);
}

// BatchedConstant translates into calls to Set() for setting up an object's
// constants. It sets the constant on both the FunctionTemplate and the
// ObjectTemplate. PropertyAttributes is always ReadOnly.
struct BatchedConstant {
    const char* const name;
    int value;
};

void batchConfigureConstants(v8::Handle<v8::FunctionTemplate>,
                             v8::Handle<v8::ObjectTemplate>,
                             const BatchedConstant*,
                             size_t constantCount);

// BatchedCallback translates into calls to Set() on the prototype ObjectTemplate.
struct BatchedCallback {
    const char* const name;
    v8::InvocationCallback callback;
};

void batchConfigureCallbacks(v8::Handle<v8::ObjectTemplate>, 
                             v8::Handle<v8::Signature>,
                             v8::PropertyAttribute,
                             const BatchedCallback*, 
                             size_t callbackCount);

v8::Local<v8::Signature> configureTemplate(v8::Persistent<v8::FunctionTemplate>,
                                           const char* interfaceName,
                                           v8::Persistent<v8::FunctionTemplate> parentClass,
                                           int fieldCount,
                                           const BatchedAttribute*,
                                           size_t attributeCount,
                                           const BatchedCallback*,
                                           size_t callbackCount);

} // namespace WebCore

#endif // V8ConfigureDOMAttributesAndMethods_h
