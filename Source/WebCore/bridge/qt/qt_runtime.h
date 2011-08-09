/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 *
 */

#ifndef BINDINGS_QT_RUNTIME_H_
#define BINDINGS_QT_RUNTIME_H_

#include "BridgeJSC.h"
#include "JavaScript.h"
#include "Weak.h"
#include "qt_instance.h"
#include "runtime_method.h"

#include <qbytearray.h>
#include <qmetaobject.h>
#include <qpointer.h>
#include <qvariant.h>

namespace JSC {
namespace Bindings {

class QtField : public Field {
public:

    typedef enum {
        MetaProperty,
#ifndef QT_NO_PROPERTIES
        DynamicProperty,
#endif
        ChildObject
    } QtFieldType;

    QtField(const QMetaProperty &p)
        : m_type(MetaProperty), m_property(p)
        {}

#ifndef QT_NO_PROPERTIES
    QtField(const QByteArray &b)
        : m_type(DynamicProperty), m_dynamicProperty(b)
        {}
#endif

    QtField(QObject *child)
        : m_type(ChildObject), m_childObject(child)
        {}

    virtual JSValue valueFromInstance(ExecState*, const Instance*) const;
    virtual void setValueToInstance(ExecState*, const Instance*, JSValue) const;
    QByteArray name() const;
    QtFieldType fieldType() const {return m_type;}
private:
    QtFieldType m_type;
    QByteArray m_dynamicProperty;
    QMetaProperty m_property;
    QPointer<QObject> m_childObject;
};


template <typename T> class QtArray : public Array
{
public:
    QtArray(QList<T> list, QMetaType::Type type, PassRefPtr<RootObject>);
    virtual ~QtArray();

    RootObject* rootObject() const;

    virtual void setValueAt(ExecState*, unsigned index, JSValue) const;
    virtual JSValue valueAt(ExecState*, unsigned index) const;
    virtual unsigned int getLength() const {return m_length;}

private:
    mutable QList<T> m_list; // setValueAt is const!
    unsigned int m_length;
    QMetaType::Type m_type;
};

// Based on RuntimeMethod

// Extra data classes (to avoid the CELL_SIZE limit on JS objects)
class QtRuntimeMethod;
class QtRuntimeMethodData : public WeakHandleOwner {
    public:
        virtual ~QtRuntimeMethodData();
        RefPtr<QtInstance> m_instance;
        Weak<QtRuntimeMethod> m_finalizer;

    private:
        void finalize(Handle<Unknown>, void*);
};

class QtRuntimeConnectionMethod;
class QtRuntimeMetaMethodData : public QtRuntimeMethodData {
    public:
        ~QtRuntimeMetaMethodData();
        QByteArray m_signature;
        bool m_allowPrivate;
        int m_index;
        WriteBarrier<QtRuntimeConnectionMethod> m_connect;
        WriteBarrier<QtRuntimeConnectionMethod> m_disconnect;
};

class QtRuntimeConnectionMethodData : public QtRuntimeMethodData {
    public:
        ~QtRuntimeConnectionMethodData();
        QByteArray m_signature;
        int m_index;
        bool m_isConnect;
};

// Common base class (doesn't really do anything interesting)
class QtRuntimeMethod : public InternalFunction {
public:
    typedef InternalFunction Base;

    virtual ~QtRuntimeMethod();

    static const ClassInfo s_info;

    static FunctionPrototype* createPrototype(ExecState*, JSGlobalObject* globalObject)
    {
        return globalObject->functionPrototype();
    }

    static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
    {
        return Structure::create(globalData, prototype, TypeInfo(ObjectType,  StructureFlags), AnonymousSlotCount, &s_info);
    }

protected:
    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesGetPropertyNames | InternalFunction::StructureFlags | OverridesVisitChildren;

    QtRuntimeMethodData *d_func() const {return d_ptr;}
    QtRuntimeMethod(QtRuntimeMethodData *dd, ExecState *exec, const Identifier &n, PassRefPtr<QtInstance> inst);
    QtRuntimeMethodData *d_ptr;
};

class QtRuntimeMetaMethod : public QtRuntimeMethod
{
public:
    typedef QtRuntimeMethod Base;

    static QtRuntimeMetaMethod* create(ExecState* exec, const Identifier& n, PassRefPtr<QtInstance> inst, int index, const QByteArray& signature, bool allowPrivate)
    {
        return new (allocateCell<QtRuntimeMetaMethod>(*exec->heap())) QtRuntimeMetaMethod(exec, n, inst, index, signature, allowPrivate);
    }

    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);
    virtual void getOwnPropertyNames(ExecState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);

    virtual void visitChildren(SlotVisitor&);

protected:
    QtRuntimeMetaMethodData* d_func() const {return reinterpret_cast<QtRuntimeMetaMethodData*>(d_ptr);}

private:
    QtRuntimeMetaMethod(ExecState*, const Identifier&, PassRefPtr<QtInstance>, int index, const QByteArray&, bool allowPrivate);

    virtual CallType getCallData(CallData&);
    static EncodedJSValue JSC_HOST_CALL call(ExecState* exec);
    static JSValue lengthGetter(ExecState*, JSValue, const Identifier&);
    static JSValue connectGetter(ExecState*, JSValue, const Identifier&);
    static JSValue disconnectGetter(ExecState*, JSValue, const Identifier&);
};

class QtConnectionObject;
class QtRuntimeConnectionMethod : public QtRuntimeMethod {
public:
    typedef QtRuntimeMethod Base;

    static QtRuntimeConnectionMethod* create(ExecState* exec, const Identifier& n, bool isConnect, PassRefPtr<QtInstance> inst, int index, const QByteArray& signature)
    {
        return new (allocateCell<QtRuntimeConnectionMethod>(*exec->heap())) QtRuntimeConnectionMethod(exec, n, isConnect, inst, index, signature);
    }

    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);
    virtual void getOwnPropertyNames(ExecState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);

protected:
    QtRuntimeConnectionMethodData* d_func() const {return reinterpret_cast<QtRuntimeConnectionMethodData*>(d_ptr);}

private:
    QtRuntimeConnectionMethod(ExecState*, const Identifier&, bool isConnect, PassRefPtr<QtInstance>, int index, const QByteArray&);

    virtual CallType getCallData(CallData&);
    static EncodedJSValue JSC_HOST_CALL call(ExecState* exec);
    static JSValue lengthGetter(ExecState*, JSValue, const Identifier&);
    static QMultiMap<QObject *, QtConnectionObject *> connections;
    friend class QtConnectionObject;
};

// A QtConnectionObject represents a connection created inside JS. It will connect its own execute() slot
// with the appropriate signal of 'sender'. When execute() is called, it will call JS 'receiverFunction'.
class QtConnectionObject : public QObject
{
public:
    QtConnectionObject(JSContextRef, PassRefPtr<QtInstance> senderInstance, int signalIndex, JSObjectRef receiver, JSObjectRef receiverFunction);
    ~QtConnectionObject();

    // Explicitly define these because want a custom qt_metacall(), so we can't use Q_OBJECT macro.
    static const QMetaObject staticMetaObject;
    virtual const QMetaObject *metaObject() const;
    virtual void *qt_metacast(const char *);
    virtual int qt_metacall(QMetaObject::Call, int, void **argv);

    void execute(void **argv);

    bool match(JSContextRef, QObject* sender, int signalIndex, JSObjectRef thisObject, JSObjectRef funcObject);

    // Note: for callers using JSC internals, remove once we don't need anymore.
    static QtConnectionObject* createWithInternalJSC(ExecState*, PassRefPtr<QtInstance> senderInstance, int signalIndex, JSObject* receiver, JSObject* receiverFunction);

private:
    JSContextRef m_context;
    RefPtr<QtInstance> m_senderInstance;

    // We use this as key in active connections multimap.
    QObject* m_originalSender;

    int m_signalIndex;
    JSObjectRef m_receiver;
    JSObjectRef m_receiverFunction;
};

QVariant convertValueToQVariant(ExecState* exec, JSValue value, QMetaType::Type hint, int *distance);
JSValue convertQVariantToValue(ExecState* exec, PassRefPtr<RootObject> root, const QVariant& variant);

} // namespace Bindings
} // namespace JSC

#endif
