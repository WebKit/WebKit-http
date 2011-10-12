/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef DFGOperands_h
#define DFGOperands_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include <wtf/Vector.h>

namespace JSC { namespace DFG {

// helper function to distinguish vars & temporaries from arguments.
inline bool operandIsArgument(int operand) { return operand < 0; }

template<typename T> struct OperandValueTraits;

template<typename T>
struct OperandValueTraits {
    static T defaultValue() { return T(); }
};

template<typename T, typename Traits = OperandValueTraits<T> >
class Operands {
public:
    Operands() { }
    
    explicit Operands(size_t numArguments, size_t numLocals)
    {
        m_arguments.fill(Traits::defaultValue(), numArguments);
        m_locals.fill(Traits::defaultValue(), numLocals);
    }
    
    size_t numberOfArguments() const { return m_arguments.size(); }
    size_t numberOfLocals() const { return m_locals.size(); }
    
    T& argument(size_t idx) { return m_arguments[idx]; }
    const T& argument(size_t idx) const { return m_arguments[idx]; }
    
    T& local(size_t idx) { return m_locals[idx]; }
    const T& local(size_t idx) const { return m_locals[idx]; }
    
    void setLocal(size_t idx, const T& value)
    {
        if (idx >= m_locals.size()) {
            size_t oldSize = m_locals.size();
            m_locals.resize(idx + 1);
            for (size_t i = oldSize; i < m_locals.size(); ++i)
                m_locals[i] = Traits::defaultValue();
        }
        
        m_locals[idx] = value;
    }
    
    void setArgumentFirstTime(size_t idx, const T& value)
    {
        ASSERT(m_arguments[idx] == Traits::defaultValue());
        argument(idx) = value;
    }
    
    void setLocalFirstTime(size_t idx, const T& value)
    {
        ASSERT(idx >= m_locals.size() || m_locals[idx] == Traits::defaultValue());
        setLocal(idx, value);
    }
    
    T& operand(int operand)
    {
        if (operandIsArgument(operand)) {
            int argument = operand + m_arguments.size() + RegisterFile::CallFrameHeaderSize;
            return m_arguments[argument];
        }
        
        return m_locals[operand];
    }
    
    const T& operand(int operand) const { return const_cast<const T&>(const_cast<Operands*>(this)->operand(operand)); }
    
    void setOperand(int operand, const T& value)
    {
        if (operandIsArgument(operand)) {
            int argument = operand + m_arguments.size() + RegisterFile::CallFrameHeaderSize;
            m_arguments[argument] = value;
            return;
        }
        
        setLocal(operand, value);
    }
    
    void clear()
    {
        for (size_t i = 0; i < m_arguments.size(); ++i)
            m_arguments[i] = Traits::defaultValue();
        for (size_t i = 0; i < m_locals.size(); ++i)
            m_locals[i] = Traits::defaultValue();
    }
    
private:
    Vector<T, 8> m_arguments;
    Vector<T, 16> m_locals;
};

template<typename T>
void dumpOperands(Operands<T>& operands, FILE* out)
{
    for (size_t argument = 0; argument < operands.numberOfArguments(); ++argument) {
        if (argument)
            fprintf(out, " ");
        operands.argument(argument).dump(out);
    }
    fprintf(out, " : ");
    for (size_t local = 0; local < operands.numberOfLocals(); ++local) {
        if (local)
            fprintf(out, " ");
        operands.local(local).dump(out);
    }
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGOperands_h

