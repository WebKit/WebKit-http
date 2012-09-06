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

#ifndef InspectorValues_h
#define InspectorValues_h

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class InspectorArray;
class InspectorObject;

class InspectorValue : public RefCounted<InspectorValue> {
public:
    static const int maxDepth = 1000;

    InspectorValue() : m_type(TypeNull) { }
    virtual ~InspectorValue() { }

    static PassRefPtr<InspectorValue> null()
    {
        return adoptRef(new InspectorValue());
    }

    typedef enum {
        TypeNull = 0,
        TypeBoolean,
        TypeNumber,
        TypeString,
        TypeObject,
        TypeArray
    } Type;

    Type type() const { return m_type; }

    bool isNull() const { return m_type == TypeNull; }

    virtual bool asBoolean(bool* output) const;
    virtual bool asNumber(double* output) const;
    virtual bool asNumber(long* output) const;
    virtual bool asNumber(int* output) const;
    virtual bool asNumber(unsigned long* output) const;
    virtual bool asNumber(unsigned int* output) const;
    virtual bool asString(String* output) const;
    virtual bool asValue(RefPtr<InspectorValue>* output);
    virtual bool asObject(RefPtr<InspectorObject>* output);
    virtual bool asArray(RefPtr<InspectorArray>* output);
    virtual PassRefPtr<InspectorObject> asObject();
    virtual PassRefPtr<InspectorArray> asArray();

    static PassRefPtr<InspectorValue> parseJSON(const String& json);

    String toJSONString() const;
    virtual void writeJSON(StringBuilder* output) const;

protected:
    explicit InspectorValue(Type type) : m_type(type) { }

private:
    Type m_type;
};

class InspectorBasicValue : public InspectorValue {
public:

    static PassRefPtr<InspectorBasicValue> create(bool value)
    {
        return adoptRef(new InspectorBasicValue(value));
    }

    static PassRefPtr<InspectorBasicValue> create(int value)
    {
        return adoptRef(new InspectorBasicValue(value));
    }

    static PassRefPtr<InspectorBasicValue> create(double value)
    {
        return adoptRef(new InspectorBasicValue(value));
    }

    virtual bool asBoolean(bool* output) const;
    virtual bool asNumber(double* output) const;
    virtual bool asNumber(long* output) const;
    virtual bool asNumber(int* output) const;
    virtual bool asNumber(unsigned long* output) const;
    virtual bool asNumber(unsigned int* output) const;

    virtual void writeJSON(StringBuilder* output) const;

private:
    explicit InspectorBasicValue(bool value) : InspectorValue(TypeBoolean), m_boolValue(value) { }
    explicit InspectorBasicValue(int value) : InspectorValue(TypeNumber), m_doubleValue((double)value) { }
    explicit InspectorBasicValue(double value) : InspectorValue(TypeNumber), m_doubleValue(value) { }

    union {
        bool m_boolValue;
        double m_doubleValue;
    };
};

class InspectorString : public InspectorValue {
public:
    static PassRefPtr<InspectorString> create(const String& value)
    {
        return adoptRef(new InspectorString(value));
    }

    static PassRefPtr<InspectorString> create(const char* value)
    {
        return adoptRef(new InspectorString(value));
    }

    virtual bool asString(String* output) const;    

    virtual void writeJSON(StringBuilder* output) const;

private:
    explicit InspectorString(const String& value) : InspectorValue(TypeString), m_stringValue(value) { }
    explicit InspectorString(const char* value) : InspectorValue(TypeString), m_stringValue(value) { }

    String m_stringValue;
};

class InspectorObjectBase : public InspectorValue {
private:
    typedef HashMap<String, RefPtr<InspectorValue> > Dictionary;

public:
    typedef Dictionary::iterator iterator;
    typedef Dictionary::const_iterator const_iterator;

    virtual PassRefPtr<InspectorObject> asObject();
    InspectorObject* openAccessors();

protected:
    ~InspectorObjectBase();

    virtual bool asObject(RefPtr<InspectorObject>* output);

    void setBoolean(const String& name, bool);
    void setNumber(const String& name, double);
    void setString(const String& name, const String&);
    void setValue(const String& name, PassRefPtr<InspectorValue>);
    void setObject(const String& name, PassRefPtr<InspectorObject>);
    void setArray(const String& name, PassRefPtr<InspectorArray>);

    iterator find(const String& name);
    const_iterator find(const String& name) const;
    bool getBoolean(const String& name, bool* output) const;
    template<class T> bool getNumber(const String& name, T* output) const
    {
        RefPtr<InspectorValue> value = get(name);
        if (!value)
            return false;
        return value->asNumber(output);
    }
    bool getString(const String& name, String* output) const;
    PassRefPtr<InspectorObject> getObject(const String& name) const;
    PassRefPtr<InspectorArray> getArray(const String& name) const;
    PassRefPtr<InspectorValue> get(const String& name) const;

    void remove(const String& name);

    virtual void writeJSON(StringBuilder* output) const;

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

    int size() const { return m_data.size(); }

protected:
    InspectorObjectBase();

private:
    Dictionary m_data;
    Vector<String> m_order;
};

class InspectorObject : public InspectorObjectBase {
public:
    static PassRefPtr<InspectorObject> create()
    {
        return adoptRef(new InspectorObject());
    }

    using InspectorObjectBase::asObject;

    using InspectorObjectBase::setBoolean;
    using InspectorObjectBase::setNumber;
    using InspectorObjectBase::setString;
    using InspectorObjectBase::setValue;
    using InspectorObjectBase::setObject;
    using InspectorObjectBase::setArray;

    using InspectorObjectBase::find;
    using InspectorObjectBase::getBoolean;
    using InspectorObjectBase::getNumber;
    using InspectorObjectBase::getString;
    using InspectorObjectBase::getObject;
    using InspectorObjectBase::getArray;
    using InspectorObjectBase::get;

    using InspectorObjectBase::remove;

    using InspectorObjectBase::begin;
    using InspectorObjectBase::end;

    using InspectorObjectBase::size;
};


class InspectorArrayBase : public InspectorValue {
public:
    typedef Vector<RefPtr<InspectorValue> >::iterator iterator;
    typedef Vector<RefPtr<InspectorValue> >::const_iterator const_iterator;

    virtual PassRefPtr<InspectorArray> asArray();

    unsigned length() const { return m_data.size(); }

protected:
    ~InspectorArrayBase();

    virtual bool asArray(RefPtr<InspectorArray>* output);

    void pushBoolean(bool);
    void pushInt(int);
    void pushNumber(double);
    void pushString(const String&);
    void pushValue(PassRefPtr<InspectorValue>);
    void pushObject(PassRefPtr<InspectorObject>);
    void pushArray(PassRefPtr<InspectorArray>);

    PassRefPtr<InspectorValue> get(size_t index);

    virtual void writeJSON(StringBuilder* output) const;

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

protected:
    InspectorArrayBase();

private:
    Vector<RefPtr<InspectorValue> > m_data;
};

class InspectorArray : public InspectorArrayBase {
public:
    static PassRefPtr<InspectorArray> create()
    {
        return adoptRef(new InspectorArray());
    }

    using InspectorArrayBase::asArray;

    using InspectorArrayBase::pushBoolean;
    using InspectorArrayBase::pushInt;
    using InspectorArrayBase::pushNumber;
    using InspectorArrayBase::pushString;
    using InspectorArrayBase::pushValue;
    using InspectorArrayBase::pushObject;
    using InspectorArrayBase::pushArray;

    using InspectorArrayBase::get;

    using InspectorArrayBase::begin;
    using InspectorArrayBase::end;
};


inline InspectorObjectBase::iterator InspectorObjectBase::find(const String& name)
{
    return m_data.find(name);
}

inline InspectorObjectBase::const_iterator InspectorObjectBase::find(const String& name) const
{
    return m_data.find(name);
}

inline void InspectorObjectBase::setBoolean(const String& name, bool value)
{
    setValue(name, InspectorBasicValue::create(value));
}

inline void InspectorObjectBase::setNumber(const String& name, double value)
{
    setValue(name, InspectorBasicValue::create(value));
}

inline void InspectorObjectBase::setString(const String& name, const String& value)
{
    setValue(name, InspectorString::create(value));
}

inline void InspectorObjectBase::setValue(const String& name, PassRefPtr<InspectorValue> value)
{
    ASSERT(value);
    if (m_data.set(name, value).isNewEntry)
        m_order.append(name);
}

inline void InspectorObjectBase::setObject(const String& name, PassRefPtr<InspectorObject> value)
{
    ASSERT(value);
    if (m_data.set(name, value).isNewEntry)
        m_order.append(name);
}

inline void InspectorObjectBase::setArray(const String& name, PassRefPtr<InspectorArray> value)
{
    ASSERT(value);
    if (m_data.set(name, value).isNewEntry)
        m_order.append(name);
}

inline void InspectorArrayBase::pushBoolean(bool value)
{
    m_data.append(InspectorBasicValue::create(value));
}

inline void InspectorArrayBase::pushInt(int value)
{
    m_data.append(InspectorBasicValue::create(value));
}

inline void InspectorArrayBase::pushNumber(double value)
{
    m_data.append(InspectorBasicValue::create(value));
}

inline void InspectorArrayBase::pushString(const String& value)
{
    m_data.append(InspectorString::create(value));
}

inline void InspectorArrayBase::pushValue(PassRefPtr<InspectorValue> value)
{
    ASSERT(value);
    m_data.append(value);
}

inline void InspectorArrayBase::pushObject(PassRefPtr<InspectorObject> value)
{
    ASSERT(value);
    m_data.append(value);
}

inline void InspectorArrayBase::pushArray(PassRefPtr<InspectorArray> value)
{
    ASSERT(value);
    m_data.append(value);
}

} // namespace WebCore

#endif // !defined(InspectorValues_h)
