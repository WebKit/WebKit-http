/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#if USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)

#import "ProxyInstance.h"

#import "NetscapePluginHostProxy.h"
#import "ProxyRuntimeObject.h"
#import <WebCore/IdentifierRep.h>
#import <WebCore/JSDOMWindow.h>
#import <WebCore/npruntime_impl.h>
#import <WebCore/runtime_method.h>
#import <runtime/Error.h>
#import <runtime/FunctionPrototype.h>
#import <runtime/PropertyNameArray.h>

extern "C" {
#import "WebKitPluginHost.h"
}

using namespace JSC;
using namespace JSC::Bindings;
using namespace std;
using namespace WebCore;

namespace WebKit {

class ProxyClass : public JSC::Bindings::Class {
private:
    virtual MethodList methodsNamed(const Identifier&, Instance*) const;
    virtual Field* fieldNamed(const Identifier&, Instance*) const;
};

MethodList ProxyClass::methodsNamed(const Identifier& identifier, Instance* instance) const
{
    return static_cast<ProxyInstance*>(instance)->methodsNamed(identifier);
}

Field* ProxyClass::fieldNamed(const Identifier& identifier, Instance* instance) const
{
    return static_cast<ProxyInstance*>(instance)->fieldNamed(identifier);
}

static ProxyClass* proxyClass()
{
    DEFINE_STATIC_LOCAL(ProxyClass, proxyClass, ());
    return &proxyClass;
}
    
class ProxyField : public JSC::Bindings::Field {
public:
    ProxyField(uint64_t serverIdentifier)
        : m_serverIdentifier(serverIdentifier)
    {
    }
    
    uint64_t serverIdentifier() const { return m_serverIdentifier; }

private:
    virtual JSValue valueFromInstance(ExecState*, const Instance*) const;
    virtual void setValueToInstance(ExecState*, const Instance*, JSValue) const;
    
    uint64_t m_serverIdentifier;
};

JSValue ProxyField::valueFromInstance(ExecState* exec, const Instance* instance) const
{
    return static_cast<const ProxyInstance*>(instance)->fieldValue(exec, this);
}
    
void ProxyField::setValueToInstance(ExecState* exec, const Instance* instance, JSValue value) const
{
    static_cast<const ProxyInstance*>(instance)->setFieldValue(exec, this, value);
}

class ProxyMethod : public JSC::Bindings::Method {
public:
    ProxyMethod(uint64_t serverIdentifier)
        : m_serverIdentifier(serverIdentifier)
    {
    }

    uint64_t serverIdentifier() const { return m_serverIdentifier; }

private:
    virtual int numParameters() const { return 0; }

    uint64_t m_serverIdentifier;
};

ProxyInstance::ProxyInstance(PassRefPtr<RootObject> rootObject, NetscapePluginInstanceProxy* instanceProxy, uint32_t objectID)
    : Instance(rootObject)
    , m_instanceProxy(instanceProxy)
    , m_objectID(objectID)
{
    m_instanceProxy->addInstance(this);
}

ProxyInstance::~ProxyInstance()
{
    deleteAllValues(m_fields);
    deleteAllValues(m_methods);

    if (!m_instanceProxy)
        return;
    
    m_instanceProxy->removeInstance(this);

    invalidate();
}
    
RuntimeObject* ProxyInstance::newRuntimeObject(ExecState* exec)
{
    return ProxyRuntimeObject::create(exec, exec->lexicalGlobalObject(), this);
}

JSC::Bindings::Class* ProxyInstance::getClass() const
{
    return proxyClass();
}

JSValue ProxyInstance::invoke(JSC::ExecState* exec, InvokeType type, uint64_t identifier, const ArgList& args)
{
    if (!m_instanceProxy)
        return jsUndefined();

    RetainPtr<NSData*> arguments(m_instanceProxy->marshalValues(exec, args));

    uint32_t requestID = m_instanceProxy->nextRequestID();

    for (unsigned i = 0; i < args.size(); i++)
        m_instanceProxy->retainLocalObject(args.at(i));

    if (_WKPHNPObjectInvoke(m_instanceProxy->hostProxy()->port(), m_instanceProxy->pluginID(), requestID, m_objectID,
                            type, identifier, (char*)[arguments.get() bytes], [arguments.get() length]) != KERN_SUCCESS) {
        if (m_instanceProxy) {
            for (unsigned i = 0; i < args.size(); i++)
                m_instanceProxy->releaseLocalObject(args.at(i));
        }
        return jsUndefined();
    }
    
    auto_ptr<NetscapePluginInstanceProxy::BooleanAndDataReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanAndDataReply>(requestID);
    NetscapePluginInstanceProxy::moveGlobalExceptionToExecState(exec);

    if (m_instanceProxy) {
        for (unsigned i = 0; i < args.size(); i++)
            m_instanceProxy->releaseLocalObject(args.at(i));
    }

    if (!reply.get() || !reply->m_returnValue)
        return jsUndefined();
    
    return m_instanceProxy->demarshalValue(exec, (char*)CFDataGetBytePtr(reply->m_result.get()), CFDataGetLength(reply->m_result.get()));
}

class ProxyRuntimeMethod : public RuntimeMethod {
public:
    typedef RuntimeMethod Base;

    static ProxyRuntimeMethod* create(ExecState* exec, JSGlobalObject* globalObject, const Identifier& name, Bindings::MethodList& list)
    {
        // FIXME: deprecatedGetDOMStructure uses the prototype off of the wrong global object
        // exec-globalData() is also likely wrong.
        Structure* domStructure = deprecatedGetDOMStructure<ProxyRuntimeMethod>(exec);
        ProxyRuntimeMethod* method = new (allocateCell<ProxyRuntimeMethod>(*exec->heap())) ProxyRuntimeMethod(globalObject, domStructure, list);
        method->finishCreation(exec->globalData(), name);
        return method;
    }

    static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(globalData, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
    }

    static const ClassInfo s_info;

private:
    ProxyRuntimeMethod(JSGlobalObject* globalObject, Structure* structure, Bindings::MethodList& list)
        : RuntimeMethod(globalObject, structure, list)
    {
    }

    void finishCreation(JSGlobalData& globalData, const Identifier& name)
    {
        Base::finishCreation(globalData, name);
        ASSERT(inherits(&s_info));
    }
};

const ClassInfo ProxyRuntimeMethod::s_info = { "ProxyRuntimeMethod", &RuntimeMethod::s_info, 0, 0, CREATE_METHOD_TABLE(ProxyRuntimeMethod) };

JSValue ProxyInstance::getMethod(JSC::ExecState* exec, const JSC::Identifier& propertyName)
{
    MethodList methodList = getClass()->methodsNamed(propertyName, this);
    return ProxyRuntimeMethod::create(exec, exec->lexicalGlobalObject(), propertyName, methodList);
}

JSValue ProxyInstance::invokeMethod(ExecState* exec, JSC::RuntimeMethod* runtimeMethod)
{
    if (!asObject(runtimeMethod)->inherits(&ProxyRuntimeMethod::s_info))
        return throwError(exec, createTypeError(exec, "Attempt to invoke non-plug-in method on plug-in object."));

    const MethodList& methodList = *runtimeMethod->methods();

    ASSERT(methodList.size() == 1);

    ProxyMethod* method = static_cast<ProxyMethod*>(methodList[0]);

    return invoke(exec, Invoke, method->serverIdentifier(), ArgList(exec));
}

bool ProxyInstance::supportsInvokeDefaultMethod() const
{
    if (!m_instanceProxy)
        return false;
    
    uint32_t requestID = m_instanceProxy->nextRequestID();
    
    if (_WKPHNPObjectHasInvokeDefaultMethod(m_instanceProxy->hostProxy()->port(),
                                            m_instanceProxy->pluginID(), requestID,
                                            m_objectID) != KERN_SUCCESS)
        return false;
    
    auto_ptr<NetscapePluginInstanceProxy::BooleanReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanReply>(requestID);
    if (reply.get() && reply->m_result)
        return true;
        
    return false;
}

JSValue ProxyInstance::invokeDefaultMethod(ExecState* exec)
{
    return invoke(exec, InvokeDefault, 0, ArgList(exec));
}

bool ProxyInstance::supportsConstruct() const
{
    if (!m_instanceProxy)
        return false;
    
    uint32_t requestID = m_instanceProxy->nextRequestID();
    
    if (_WKPHNPObjectHasConstructMethod(m_instanceProxy->hostProxy()->port(),
                                        m_instanceProxy->pluginID(), requestID,
                                        m_objectID) != KERN_SUCCESS)
        return false;
    
    auto_ptr<NetscapePluginInstanceProxy::BooleanReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanReply>(requestID);
    if (reply.get() && reply->m_result)
        return true;
        
    return false;
}
    
JSValue ProxyInstance::invokeConstruct(ExecState* exec, const ArgList& args)
{
    return invoke(exec, Construct, 0, args);
}

JSValue ProxyInstance::defaultValue(ExecState* exec, PreferredPrimitiveType hint) const
{
    if (hint == PreferString)
        return stringValue(exec);
    if (hint == PreferNumber)
        return numberValue(exec);
    return valueOf(exec);
}

JSValue ProxyInstance::stringValue(ExecState* exec) const
{
    // FIXME: Implement something sensible.
    return jsEmptyString(exec);
}

JSValue ProxyInstance::numberValue(ExecState*) const
{
    // FIXME: Implement something sensible.
    return jsNumber(0);
}

JSValue ProxyInstance::booleanValue() const
{
    // FIXME: Implement something sensible.
    return jsBoolean(false);
}

JSValue ProxyInstance::valueOf(ExecState* exec) const
{
    return stringValue(exec);
}

void ProxyInstance::getPropertyNames(ExecState* exec, PropertyNameArray& nameArray)
{
    if (!m_instanceProxy)
        return;
    
    uint32_t requestID = m_instanceProxy->nextRequestID();
    
    if (_WKPHNPObjectEnumerate(m_instanceProxy->hostProxy()->port(), m_instanceProxy->pluginID(), requestID, m_objectID) != KERN_SUCCESS)
        return;
    
    auto_ptr<NetscapePluginInstanceProxy::BooleanAndDataReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanAndDataReply>(requestID);
    NetscapePluginInstanceProxy::moveGlobalExceptionToExecState(exec);
    if (!reply.get() || !reply->m_returnValue)
        return;
    
    RetainPtr<NSArray*> array = [NSPropertyListSerialization propertyListFromData:(NSData *)reply->m_result.get()
                                                                 mutabilityOption:NSPropertyListImmutable
                                                                           format:0
                                                                 errorDescription:0];
    
    for (NSNumber *number in array.get()) {
        IdentifierRep* identifier = reinterpret_cast<IdentifierRep*>([number longLongValue]);
        if (!IdentifierRep::isValid(identifier))
            continue;

        if (identifier->isString()) {
            const char* str = identifier->string();
            nameArray.add(Identifier(JSDOMWindow::commonJSGlobalData(), stringToUString(String::fromUTF8WithLatin1Fallback(str, strlen(str)))));
        } else
            nameArray.add(Identifier::from(exec, identifier->number()));
    }
}

MethodList ProxyInstance::methodsNamed(const Identifier& identifier)
{
    if (!m_instanceProxy)
        return MethodList();
    
    // If we already have an entry in the map, use it.
    MethodMap::iterator existingMapEntry = m_methods.find(identifier.impl());
    if (existingMapEntry != m_methods.end()) {
        MethodList methodList;
        if (existingMapEntry->second)
            methodList.append(existingMapEntry->second);
        return methodList;
    }
    
    uint64_t methodName = reinterpret_cast<uint64_t>(_NPN_GetStringIdentifier(identifier.ascii().data()));
    uint32_t requestID = m_instanceProxy->nextRequestID();
    
    if (_WKPHNPObjectHasMethod(m_instanceProxy->hostProxy()->port(),
                               m_instanceProxy->pluginID(), requestID,
                               m_objectID, methodName) != KERN_SUCCESS)
        return MethodList();
    
    auto_ptr<NetscapePluginInstanceProxy::BooleanReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanReply>(requestID);
    if (!reply.get())
        return MethodList();

    if (!reply->m_result && !m_instanceProxy->hostProxy()->shouldCacheMissingPropertiesAndMethods())
        return MethodList();

    // Add a new entry to the map unless an entry was added while we were in waitForReply.
    pair<MethodMap::iterator, bool> mapAddResult = m_methods.add(identifier.impl(), 0);
    if (mapAddResult.second && reply->m_result)
        mapAddResult.first->second = new ProxyMethod(methodName);

    MethodList methodList;
    if (mapAddResult.first->second)
        methodList.append(mapAddResult.first->second);
    return methodList;
}

Field* ProxyInstance::fieldNamed(const Identifier& identifier)
{
    if (!m_instanceProxy)
        return 0;
    
    // If we already have an entry in the map, use it.
    FieldMap::iterator existingMapEntry = m_fields.find(identifier.impl());
    if (existingMapEntry != m_fields.end())
        return existingMapEntry->second;
    
    uint64_t propertyName = reinterpret_cast<uint64_t>(_NPN_GetStringIdentifier(identifier.ascii().data()));
    uint32_t requestID = m_instanceProxy->nextRequestID();
    
    if (_WKPHNPObjectHasProperty(m_instanceProxy->hostProxy()->port(),
                                 m_instanceProxy->pluginID(), requestID,
                                 m_objectID, propertyName) != KERN_SUCCESS)
        return 0;
    
    auto_ptr<NetscapePluginInstanceProxy::BooleanReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanReply>(requestID);
    if (!reply.get())
        return 0;
    
    if (!reply->m_result && !m_instanceProxy->hostProxy()->shouldCacheMissingPropertiesAndMethods())
        return 0;
    
    // Add a new entry to the map unless an entry was added while we were in waitForReply.
    pair<FieldMap::iterator, bool> mapAddResult = m_fields.add(identifier.impl(), 0);
    if (mapAddResult.second && reply->m_result)
        mapAddResult.first->second = new ProxyField(propertyName);
    return mapAddResult.first->second;
}

JSC::JSValue ProxyInstance::fieldValue(ExecState* exec, const Field* field) const
{
    if (!m_instanceProxy)
        return jsUndefined();
    
    uint64_t serverIdentifier = static_cast<const ProxyField*>(field)->serverIdentifier();
    uint32_t requestID = m_instanceProxy->nextRequestID();
    
    if (_WKPHNPObjectGetProperty(m_instanceProxy->hostProxy()->port(),
                                 m_instanceProxy->pluginID(), requestID,
                                 m_objectID, serverIdentifier) != KERN_SUCCESS)
        return jsUndefined();
    
    auto_ptr<NetscapePluginInstanceProxy::BooleanAndDataReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanAndDataReply>(requestID);
    NetscapePluginInstanceProxy::moveGlobalExceptionToExecState(exec);
    if (!reply.get() || !reply->m_returnValue)
        return jsUndefined();
    
    return m_instanceProxy->demarshalValue(exec, (char*)CFDataGetBytePtr(reply->m_result.get()), CFDataGetLength(reply->m_result.get()));
}
    
void ProxyInstance::setFieldValue(ExecState* exec, const Field* field, JSValue value) const
{
    if (!m_instanceProxy)
        return;
    
    uint64_t serverIdentifier = static_cast<const ProxyField*>(field)->serverIdentifier();
    uint32_t requestID = m_instanceProxy->nextRequestID();
    
    data_t valueData;
    mach_msg_type_number_t valueLength;

    m_instanceProxy->marshalValue(exec, value, valueData, valueLength);
    m_instanceProxy->retainLocalObject(value);
    kern_return_t kr = _WKPHNPObjectSetProperty(m_instanceProxy->hostProxy()->port(),
                                                m_instanceProxy->pluginID(), requestID,
                                                m_objectID, serverIdentifier, valueData, valueLength);
    mig_deallocate(reinterpret_cast<vm_address_t>(valueData), valueLength);
    if (m_instanceProxy)
        m_instanceProxy->releaseLocalObject(value);
    if (kr != KERN_SUCCESS)
        return;
    
    auto_ptr<NetscapePluginInstanceProxy::BooleanReply> reply = waitForReply<NetscapePluginInstanceProxy::BooleanReply>(requestID);
    NetscapePluginInstanceProxy::moveGlobalExceptionToExecState(exec);
}

void ProxyInstance::invalidate()
{
    ASSERT(m_instanceProxy);
    
    if (NetscapePluginHostProxy* hostProxy = m_instanceProxy->hostProxy())
        _WKPHNPObjectRelease(hostProxy->port(),
                             m_instanceProxy->pluginID(), m_objectID);
    m_instanceProxy = 0;
}

} // namespace WebKit

#endif // USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)

