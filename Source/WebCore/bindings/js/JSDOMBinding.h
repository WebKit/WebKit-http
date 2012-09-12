/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2009 Google, Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef JSDOMBinding_h
#define JSDOMBinding_h

#include "BindingState.h"
#include "CSSImportRule.h"
#include "CSSStyleDeclaration.h"
#include "CSSStyleSheet.h"
#include "JSDOMGlobalObject.h"
#include "JSDOMWrapper.h"
#include "DOMWrapperWorld.h"
#include "Document.h"
#include "Element.h"
#include "MediaList.h"
#include "StylePropertySet.h"
#include "StyledElement.h"
#include <heap/Weak.h>
#include <runtime/Error.h>
#include <runtime/FunctionPrototype.h>
#include <runtime/JSArray.h>
#include <runtime/Lookup.h>
#include <runtime/ObjectPrototype.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace WebCore {

class DOMStringList;

enum ParameterDefaultPolicy {
    DefaultIsUndefined,
    DefaultIsNullString
};

#define MAYBE_MISSING_PARAMETER(exec, index, policy) (((policy) == DefaultIsNullString && (index) >= (exec)->argumentCount()) ? (JSValue()) : ((exec)->argument(index)))

    class Frame;
    class KURL;

    typedef int ExceptionCode;

    // Base class for all constructor objects in the JSC bindings.
    class DOMConstructorObject : public JSDOMWrapper {
        typedef JSDOMWrapper Base;
    public:
        static JSC::Structure* createStructure(JSC::JSGlobalData& globalData, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
        {
            return JSC::Structure::create(globalData, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), &s_info);
        }

    protected:
        static const unsigned StructureFlags = JSC::ImplementsHasInstance | JSC::OverridesVisitChildren | JSDOMWrapper::StructureFlags;
        DOMConstructorObject(JSC::Structure* structure, JSDOMGlobalObject* globalObject)
            : JSDOMWrapper(structure, globalObject)
        {
        }
    };

    // Constructors using this base class depend on being in a Document and
    // can never be used from a WorkerContext.
    class DOMConstructorWithDocument : public DOMConstructorObject {
        typedef DOMConstructorObject Base;
    public:
        Document* document() const
        {
            return static_cast<Document*>(scriptExecutionContext());
        }

    protected:
        DOMConstructorWithDocument(JSC::Structure* structure, JSDOMGlobalObject* globalObject)
            : DOMConstructorObject(structure, globalObject)
        {
        }

        void finishCreation(JSDOMGlobalObject* globalObject)
        {
            Base::finishCreation(globalObject->globalData());
            ASSERT(globalObject->scriptExecutionContext()->isDocument());
        }
    };
    
    JSC::Structure* getCachedDOMStructure(JSDOMGlobalObject*, const JSC::ClassInfo*);
    JSC::Structure* cacheDOMStructure(JSDOMGlobalObject*, JSC::Structure*, const JSC::ClassInfo*);

    inline JSDOMGlobalObject* deprecatedGlobalObjectForPrototype(JSC::ExecState* exec)
    {
        // FIXME: Callers to this function should be using the global object
        // from which the object is being created, instead of assuming the lexical one.
        // e.g. subframe.document.body should use the subframe's global object, not the lexical one.
        return JSC::jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
    }

    template<class WrapperClass> inline JSC::Structure* getDOMStructure(JSC::ExecState* exec, JSDOMGlobalObject* globalObject)
    {
        if (JSC::Structure* structure = getCachedDOMStructure(globalObject, &WrapperClass::s_info))
            return structure;
        return cacheDOMStructure(globalObject, WrapperClass::createStructure(exec->globalData(), globalObject, WrapperClass::createPrototype(exec, globalObject)), &WrapperClass::s_info);
    }

    template<class WrapperClass> inline JSC::Structure* deprecatedGetDOMStructure(JSC::ExecState* exec)
    {
        // FIXME: This function is wrong.  It uses the wrong global object for creating the prototype structure.
        return getDOMStructure<WrapperClass>(exec, deprecatedGlobalObjectForPrototype(exec));
    }

    template<class WrapperClass> inline JSC::JSObject* getDOMPrototype(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject)
    {
        return JSC::jsCast<JSC::JSObject*>(asObject(getDOMStructure<WrapperClass>(exec, JSC::jsCast<JSDOMGlobalObject*>(globalObject))->storedPrototype()));
    }

    // Overload these functions to provide a fast path for wrapper access.
    inline JSDOMWrapper* getInlineCachedWrapper(DOMWrapperWorld*, void*) { return 0; }
    inline bool setInlineCachedWrapper(DOMWrapperWorld*, void*, JSDOMWrapper*) { return false; }
    inline bool clearInlineCachedWrapper(DOMWrapperWorld*, void*, JSDOMWrapper*) { return false; }

    template <typename DOMClass> inline JSDOMWrapper* getCachedWrapper(DOMWrapperWorld* world, DOMClass* domObject)
    {
        if (JSDOMWrapper* wrapper = getInlineCachedWrapper(world, domObject))
            return wrapper;
        return world->m_wrappers.get(domObject);
    }

    template <typename DOMClass> inline void cacheWrapper(DOMWrapperWorld* world, DOMClass* domObject, JSDOMWrapper* wrapper)
    {
        if (setInlineCachedWrapper(world, domObject, wrapper))
            return;
        JSC::PassWeak<JSDOMWrapper> passWeak(wrapper, wrapperOwner(world, domObject), wrapperContext(world, domObject));
        weakAdd(world->m_wrappers, (void*)domObject, passWeak);
    }

    template <typename DOMClass> inline void uncacheWrapper(DOMWrapperWorld* world, DOMClass* domObject, JSDOMWrapper* wrapper)
    {
        if (clearInlineCachedWrapper(world, domObject, wrapper))
            return;
        weakRemove(world->m_wrappers, (void*)domObject, wrapper);
    }
    
    #define CREATE_DOM_WRAPPER(exec, globalObject, className, object) createWrapper<JS##className>(exec, globalObject, static_cast<className*>(object))
    template<class WrapperClass, class DOMClass> inline JSDOMWrapper* createWrapper(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, DOMClass* node)
    {
        ASSERT(node);
        ASSERT(!getCachedWrapper(currentWorld(exec), node));
        WrapperClass* wrapper = WrapperClass::create(getDOMStructure<WrapperClass>(exec, globalObject), globalObject, node);
        // FIXME: The entire function can be removed, once we fix caching.
        // This function is a one-off hack to make Nodes cache in the right global object.
        cacheWrapper(currentWorld(exec), node, wrapper);
        return wrapper;
    }

    template<class WrapperClass, class DOMClass> inline JSC::JSValue wrap(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, DOMClass* domObject)
    {
        if (!domObject)
            return JSC::jsNull();
        if (JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), domObject))
            return wrapper;
        return createWrapper<WrapperClass>(exec, globalObject, domObject);
    }

    inline void* root(Node* node)
    {
        if (node->inDocument())
            return node->document();

        while (node->parentOrHostNode())
            node = node->parentOrHostNode();
        return node;
    }

    inline void* root(StyleSheet*);

    inline void* root(CSSRule* rule)
    {
        if (rule->parentRule())
            return root(rule->parentRule());
        if (rule->parentStyleSheet())
            return root(rule->parentStyleSheet());
        return rule;
    }

    inline void* root(StyleSheet* styleSheet)
    {
        if (styleSheet->ownerRule())
            return root(styleSheet->ownerRule());
        if (styleSheet->ownerNode())
            return root(styleSheet->ownerNode());
        return styleSheet;
    }

    inline void* root(CSSStyleDeclaration* style)
    {
        if (CSSRule* parentRule = style->parentRule())
            return root(parentRule);
        if (CSSStyleSheet* styleSheet = style->parentStyleSheet())
            return root(styleSheet);
        return style;
    }

    inline void* root(MediaList* mediaList)
    {
        if (CSSRule* parentRule = mediaList->parentRule())
            return root(parentRule);
        if (CSSStyleSheet* parentStyleSheet = mediaList->parentStyleSheet())
            return root(parentStyleSheet);
        return mediaList;
    }

    const JSC::HashTable* getHashTableForGlobalData(JSC::JSGlobalData&, const JSC::HashTable* staticTable);

    void reportException(JSC::ExecState*, JSC::JSValue exception);
    void reportCurrentException(JSC::ExecState*);

    // Convert a DOM implementation exception code into a JavaScript exception in the execution state.
    void setDOMException(JSC::ExecState*, ExceptionCode);

    JSC::JSValue jsStringWithCache(JSC::ExecState*, const String&);
    JSC::JSValue jsStringWithCacheSlowCase(JSC::ExecState*, JSStringCache&, StringImpl*);
    JSC::JSValue jsString(JSC::ExecState*, const KURL&); // empty if the URL is null
    inline JSC::JSValue jsStringWithCache(JSC::ExecState* exec, const AtomicString& s)
    { 
        return jsStringWithCache(exec, s.string());
    }
        
    JSC::JSValue jsStringOrNull(JSC::ExecState*, const String&); // null if the string is null
    JSC::JSValue jsStringOrNull(JSC::ExecState*, const KURL&); // null if the URL is null

    JSC::JSValue jsStringOrUndefined(JSC::ExecState*, const String&); // undefined if the string is null
    JSC::JSValue jsStringOrUndefined(JSC::ExecState*, const KURL&); // undefined if the URL is null

    // See JavaScriptCore for explanation: Should be used for any string that is already owned by another
    // object, to let the engine know that collecting the JSString wrapper is unlikely to save memory.
    JSC::JSValue jsOwnedStringOrNull(JSC::ExecState*, const String&); 

    String propertyNameToString(JSC::PropertyName);

    AtomicString propertyNameToAtomicString(JSC::PropertyName);
    AtomicStringImpl* findAtomicString(JSC::PropertyName);

    String valueToStringWithNullCheck(JSC::ExecState*, JSC::JSValue); // null if the value is null
    String valueToStringWithUndefinedOrNullCheck(JSC::ExecState*, JSC::JSValue); // null if the value is null or undefined

    inline int32_t finiteInt32Value(JSC::JSValue value, JSC::ExecState* exec, bool& okay)
    {
        double number = value.toNumber(exec);
        okay = isfinite(number);
        return JSC::toInt32(number);
    }

    // Returns a Date instance for the specified value, or null if the value is NaN or infinity.
    JSC::JSValue jsDateOrNull(JSC::ExecState*, double);
    // NaN if the value can't be converted to a date.
    double valueToDate(JSC::ExecState*, JSC::JSValue);

    // Validates that the passed object is a sequence type per section 4.1.13 of the WebIDL spec.
    inline JSC::JSObject* toJSSequence(JSC::ExecState* exec, JSC::JSValue value, unsigned& length)
    {
        JSC::JSObject* object = value.getObject();
        if (!object) {
            throwTypeError(exec);
            return 0;
        }

        JSC::JSValue lengthValue = object->get(exec, exec->propertyNames().length);
        if (exec->hadException())
            return 0;

        if (lengthValue.isUndefinedOrNull()) {
            throwTypeError(exec);
            return 0;
        }

        length = lengthValue.toUInt32(exec);
        if (exec->hadException())
            return 0;

        return object;
    }

    template <typename T>
    inline JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, PassRefPtr<T> ptr)
    {
        return toJS(exec, globalObject, ptr.get());
    }

    template <class T>
    struct JSValueTraits {
        static inline JSC::JSValue arrayJSValue(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, const T& value)
        {
            return toJS(exec, globalObject, WTF::getPtr(value));
        }
    };

    template<>
    struct JSValueTraits<String> {
        static inline JSC::JSValue arrayJSValue(JSC::ExecState* exec, JSDOMGlobalObject*, const String& value)
        {
            return jsStringWithCache(exec, value);
        }
    };

    template<>
    struct JSValueTraits<float> {
        static inline JSC::JSValue arrayJSValue(JSC::ExecState*, JSDOMGlobalObject*, const float& value)
        {
            return JSC::jsNumber(value);
        }
    };

    template<>
    struct JSValueTraits<unsigned long> {
        static inline JSC::JSValue arrayJSValue(JSC::ExecState*, JSDOMGlobalObject*, const unsigned long& value)
        {
            return JSC::jsNumber(value);
        }
    };

    template <typename T, size_t inlineCapacity>
    JSC::JSValue jsArray(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, const Vector<T, inlineCapacity>& iterator)
    {
        JSC::MarkedArgumentBuffer list;
        typename Vector<T, inlineCapacity>::const_iterator end = iterator.end();        
        typedef JSValueTraits<T> TraitsType;

        for (typename Vector<T, inlineCapacity>::const_iterator iter = iterator.begin(); iter != end; ++iter)
            list.append(TraitsType::arrayJSValue(exec, globalObject, *iter));

        return JSC::constructArray(exec, globalObject, list);
    }

    JSC::JSValue jsArray(JSC::ExecState*, JSDOMGlobalObject*, PassRefPtr<DOMStringList>);

    template<class T> struct NativeValueTraits;

    template<>
    struct NativeValueTraits<String> {
        static inline bool arrayNativeValue(JSC::ExecState* exec, JSC::JSValue jsValue, String& indexedValue)
        {
            indexedValue = jsValue.toString(exec)->value(exec);
            return true;
        }
    };

    template<>
    struct NativeValueTraits<unsigned long> {
        static inline bool arrayNativeValue(JSC::ExecState* exec, JSC::JSValue jsValue, unsigned long& indexedValue)
        {
            if (!jsValue.isNumber())
                return false;

            indexedValue = jsValue.toUInt32(exec);
            if (exec->hadException())
                return false;

            return true;
        }
    };

    template<>
    struct NativeValueTraits<float> {
        static inline bool arrayNativeValue(JSC::ExecState* exec, JSC::JSValue jsValue, float& indexedValue)
        {
            indexedValue = jsValue.toFloat(exec);
            return !exec->hadException();
        }
    };

    template <class T>
    Vector<T> toNativeArray(JSC::ExecState* exec, JSC::JSValue value)
    {
        unsigned length = 0;
        if (isJSArray(value)) {
            JSC::JSArray* array = asArray(value);
            length = array->length();
        } else
            toJSSequence(exec, value, length);

        JSC::JSObject* object = value.getObject();
        Vector<T> result;
        typedef NativeValueTraits<T> TraitsType;

        for (unsigned i = 0; i < length; ++i) {
            T indexValue;
            if (!TraitsType::arrayNativeValue(exec, object->get(exec, i), indexValue))
                return Vector<T>();
            result.append(indexValue);
        }
        return result;
    }

    bool shouldAllowAccessToNode(JSC::ExecState*, Node*);
    bool shouldAllowAccessToFrame(JSC::ExecState*, Frame*);
    bool shouldAllowAccessToFrame(JSC::ExecState*, Frame*, String& message);
    bool shouldAllowAccessToDOMWindow(BindingState*, DOMWindow*, String& message);

    void printErrorMessageForFrame(Frame*, const String& message);
    JSC::JSValue objectToStringFunctionGetter(JSC::ExecState*, JSC::JSValue, JSC::PropertyName);

    inline JSC::JSValue jsStringWithCache(JSC::ExecState* exec, const String& s)
    {
        StringImpl* stringImpl = s.impl();
        if (!stringImpl || !stringImpl->length())
            return jsEmptyString(exec);

        if (stringImpl->length() == 1) {
            UChar singleCharacter = (*stringImpl)[0u];
            if (singleCharacter <= JSC::maxSingleCharacterString) {
                JSC::JSGlobalData* globalData = &exec->globalData();
                return globalData->smallStrings.singleCharacterString(globalData, static_cast<unsigned char>(singleCharacter));
            }
        }

        JSStringCache& stringCache = currentWorld(exec)->m_stringCache;
        if (JSC::JSString* string = stringCache.get(stringImpl))
            return string;

        return jsStringWithCacheSlowCase(exec, stringCache, stringImpl);
    }

    inline String propertyNameToString(JSC::PropertyName propertyName)
    {
        return propertyName.publicName();
    }

    inline AtomicString propertyNameToAtomicString(JSC::PropertyName propertyName)
    {
        return AtomicString(propertyName.publicName());
    }

} // namespace WebCore

#endif // JSDOMBinding_h
