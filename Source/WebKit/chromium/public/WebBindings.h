/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef WebBindings_h
#define WebBindings_h

#include "WebCommon.h"
#include "WebString.h"
#include "WebVector.h"
#include <bindings/npruntime.h>

namespace WebKit {

class WebDragData;
class WebElement;
class WebNode;
class WebRange;

// A haphazard collection of functions for dealing with plugins.
class WebBindings {
public:
    // NPN Functions ------------------------------------------------------
    // These are all defined in npruntime.h and are well documented.

    // NPN_Construct
    WEBKIT_API static bool construct(NPP, NPObject*, const NPVariant* args, uint32_t argCount, NPVariant* result);

    // NPN_CreateObject
    WEBKIT_API static NPObject* createObject(NPP, NPClass*);

    // NPN_Enumerate
    WEBKIT_API static bool enumerate(NPP, NPObject*, NPIdentifier**, uint32_t* identifierCount);

    // NPN_Evaluate
    WEBKIT_API static bool evaluate(NPP, NPObject*, NPString* script, NPVariant* result);

    // NPN_EvaluateHelper
    WEBKIT_API static bool evaluateHelper(NPP, bool popupsAllowed, NPObject*, NPString* script, NPVariant* result);

    // NPN_GetIntIdentifier
    WEBKIT_API static NPIdentifier getIntIdentifier(int32_t number);

    // NPN_GetProperty
    WEBKIT_API static bool getProperty(NPP, NPObject*, NPIdentifier property, NPVariant *result);

    // NPN_GetStringIdentifier
    WEBKIT_API static NPIdentifier getStringIdentifier(const NPUTF8* string);

    // NPN_GetStringIdentifiers
    WEBKIT_API static void getStringIdentifiers(const NPUTF8** names, int32_t nameCount, NPIdentifier*);

    // NPN_HasMethod
    WEBKIT_API static bool hasMethod(NPP, NPObject*, NPIdentifier method);

    // NPN_HasProperty
    WEBKIT_API static bool hasProperty(NPP, NPObject*, NPIdentifier property);

    // NPN_IdentifierIsString
    WEBKIT_API static bool identifierIsString(NPIdentifier);

    // NPN_InitializeVariantWithStringCopy (though sometimes prefixed with an underscore)
    WEBKIT_API static void initializeVariantWithStringCopy(NPVariant*, const NPString*);

    // NPN_IntFromIdentifier
    WEBKIT_API static int32_t intFromIdentifier(NPIdentifier);

    // NPN_Invoke
    WEBKIT_API static bool invoke(NPP, NPObject*, NPIdentifier method, const NPVariant* args, uint32_t argCount, NPVariant* result);

    // NPN_InvokeDefault
    WEBKIT_API static bool invokeDefault(NPP, NPObject*, const NPVariant* args, uint32_t argCount, NPVariant* result);

    // NPN_ReleaseObject
    WEBKIT_API static void releaseObject(NPObject*);

    // NPN_ReleaseVariantValue
    WEBKIT_API static void releaseVariantValue(NPVariant*);

    // NPN_RemoveProperty
    WEBKIT_API static bool removeProperty(NPP, NPObject*, NPIdentifier);

    // NPN_RetainObject
    WEBKIT_API static NPObject* retainObject(NPObject*);

    // NPN_SetException
    WEBKIT_API static void setException(NPObject*, const NPUTF8* message);

    // NPN_SetProperty
    WEBKIT_API static bool setProperty(NPP, NPObject*, NPIdentifier, const NPVariant*);

    // _NPN_UnregisterObject
    WEBKIT_API static void unregisterObject(NPObject*);

    // NPN_UTF8FromIdentifier
    WEBKIT_API static NPUTF8* utf8FromIdentifier(NPIdentifier);

    // Miscellaneous utility functions ----------------------------------------

    // Complement to NPN_Get___Identifier functions.  Extracts data from the NPIdentifier data
    // structure.  If isString is true upon return, string will be set but number's value is
    // undefined.  If iString is false, the opposite is true.
    WEBKIT_API static void extractIdentifierData(const NPIdentifier&, const NPUTF8*& string, int32_t& number, bool& isString);

    // DumpRenderTree support -------------------------------------------------

    // Return true (success) if the given npobj is a range object.
    // If so, return that range as a WebRange object.
    WEBKIT_API static bool getRange(NPObject* range, WebRange*);

    // Return true (success) if the given npobj is an element.
    // If so, return that element as a WebElement object.
    WEBKIT_API static bool getElement(NPObject* element, WebElement*);

    WEBKIT_API static NPObject* makeIntArray(const WebVector<int>&);
    WEBKIT_API static NPObject* makeStringArray(const WebVector<WebString>&);

    // Exceptions -------------------------------------------------------------

    typedef void (ExceptionHandler)(void* data, const NPUTF8* message);

    // The exception handler will be notified of any exceptions thrown while
    // operating on a NPObject.
    WEBKIT_API static void pushExceptionHandler(ExceptionHandler, void* data);
    WEBKIT_API static void popExceptionHandler();
};

} // namespace WebKit

#endif
