/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "LinkBuffer.h"

#if ENABLE(ASSEMBLER)

#include "Options.h"
#include <wtf/CompilationThread.h>

namespace JSC {

LinkBuffer::CodeRef LinkBuffer::finalizeCodeWithoutDisassembly()
{
    performFinalization();
    
    ASSERT(m_didAllocate);
    if (m_executableMemory)
        return CodeRef(m_executableMemory);
    
    return CodeRef::createSelfManagedCodeRef(MacroAssemblerCodePtr(m_code));
}

LinkBuffer::CodeRef LinkBuffer::finalizeCodeWithDisassembly(const char* format, ...)
{
    ASSERT(Options::showDisassembly() || Options::showDFGDisassembly());
    
    CodeRef result = finalizeCodeWithoutDisassembly();
    
    dataLogF("Generated JIT code for ");
    va_list argList;
    va_start(argList, format);
    WTF::dataLogFV(format, argList);
    va_end(argList);
    dataLogF(":\n");
    
    dataLogF("    Code at [%p, %p):\n", result.code().executableAddress(), static_cast<char*>(result.code().executableAddress()) + result.size());
    disassemble(result.code(), m_size, "    ", WTF::dataFile());
    
    return result;
}

#if ENABLE(BRANCH_COMPACTION)
template <typename InstructionType>
void LinkBuffer::copyCompactAndLinkCode(void* ownerUID, JITCompilationEffort effort)
{
    m_initialSize = m_assembler->m_assembler.codeSize();
    allocate(m_initialSize, ownerUID, effort);
    uint8_t* inData = (uint8_t*)m_assembler->unlinkedCode();
    uint8_t* outData = reinterpret_cast<uint8_t*>(m_code);
    int readPtr = 0;
    int writePtr = 0;
    Vector<LinkRecord, 0, UnsafeVectorOverflow>& jumpsToLink = m_assembler->jumpsToLink();
    unsigned jumpCount = jumpsToLink.size();
    for (unsigned i = 0; i < jumpCount; ++i) {
        int offset = readPtr - writePtr;
        ASSERT(!(offset & 1));
            
        // Copy the instructions from the last jump to the current one.
        size_t regionSize = jumpsToLink[i].from() - readPtr;
        InstructionType* copySource = reinterpret_cast_ptr<InstructionType*>(inData + readPtr);
        InstructionType* copyEnd = reinterpret_cast_ptr<InstructionType*>(inData + readPtr + regionSize);
        InstructionType* copyDst = reinterpret_cast_ptr<InstructionType*>(outData + writePtr);
        ASSERT(!(regionSize % 2));
        ASSERT(!(readPtr % 2));
        ASSERT(!(writePtr % 2));
        while (copySource != copyEnd)
            *copyDst++ = *copySource++;
        m_assembler->recordLinkOffsets(readPtr, jumpsToLink[i].from(), offset);
        readPtr += regionSize;
        writePtr += regionSize;
            
        // Calculate absolute address of the jump target, in the case of backwards
        // branches we need to be precise, forward branches we are pessimistic
        const uint8_t* target;
        if (jumpsToLink[i].to() >= jumpsToLink[i].from())
            target = outData + jumpsToLink[i].to() - offset; // Compensate for what we have collapsed so far
        else
            target = outData + jumpsToLink[i].to() - m_assembler->executableOffsetFor(jumpsToLink[i].to());
            
        JumpLinkType jumpLinkType = m_assembler->computeJumpType(jumpsToLink[i], outData + writePtr, target);
        // Compact branch if we can...
        if (m_assembler->canCompact(jumpsToLink[i].type())) {
            // Step back in the write stream
            int32_t delta = m_assembler->jumpSizeDelta(jumpsToLink[i].type(), jumpLinkType);
            if (delta) {
                writePtr -= delta;
                m_assembler->recordLinkOffsets(jumpsToLink[i].from() - delta, readPtr, readPtr - writePtr);
            }
        }
        jumpsToLink[i].setFrom(writePtr);
    }
    // Copy everything after the last jump
    memcpy(outData + writePtr, inData + readPtr, m_initialSize - readPtr);
    m_assembler->recordLinkOffsets(readPtr, m_initialSize, readPtr - writePtr);
        
    for (unsigned i = 0; i < jumpCount; ++i) {
        uint8_t* location = outData + jumpsToLink[i].from();
        uint8_t* target = outData + jumpsToLink[i].to() - m_assembler->executableOffsetFor(jumpsToLink[i].to());
        m_assembler->link(jumpsToLink[i], location, target);
    }

    jumpsToLink.clear();
    shrink(writePtr + m_initialSize - readPtr);

#if DUMP_LINK_STATISTICS
    dumpLinkStatistics(m_code, m_initialSize, m_size);
#endif
#if DUMP_CODE
    dumpCode(m_code, m_size);
#endif
}
#endif


void LinkBuffer::linkCode(void* ownerUID, JITCompilationEffort effort)
{
#if !ENABLE(BRANCH_COMPACTION)
#if defined(ASSEMBLER_HAS_CONSTANT_POOL) && ASSEMBLER_HAS_CONSTANT_POOL
    m_assembler->m_assembler.buffer().flushConstantPool(false);
#endif
    AssemblerBuffer& buffer = m_assembler->m_assembler.buffer();
    allocate(buffer.codeSize(), ownerUID, effort);
    if (!m_didAllocate)
        return;
    ASSERT(m_code);
#if CPU(ARM_TRADITIONAL)
    m_assembler->m_assembler.prepareExecutableCopy(m_code);
#endif
    memcpy(m_code, buffer.data(), buffer.codeSize());
#elif CPU(ARM_THUMB2)
    copyCompactAndLinkCode<uint16_t>(ownerUID, effort);
#elif CPU(ARM64)
    copyCompactAndLinkCode<uint32_t>(ownerUID, effort);
#endif
}

void LinkBuffer::allocate(size_t initialSize, void* ownerUID, JITCompilationEffort effort)
{
    if (m_code) {
        if (initialSize > m_size)
            return;
        
        m_didAllocate = true;
        m_size = initialSize;
        return;
    }
    
    m_executableMemory = m_vm->executableAllocator.allocate(*m_vm, initialSize, ownerUID, effort);
    if (!m_executableMemory)
        return;
    ExecutableAllocator::makeWritable(m_executableMemory->start(), m_executableMemory->sizeInBytes());
    m_code = m_executableMemory->start();
    m_size = initialSize;
    m_didAllocate = true;
}

void LinkBuffer::shrink(size_t newSize)
{
    m_size = newSize;
    m_executableMemory->shrink(m_size);
}

void LinkBuffer::performFinalization()
{
#ifndef NDEBUG
    ASSERT(!isCompilationThread());
    ASSERT(!m_completed);
    ASSERT(isValid());
    m_completed = true;
#endif
    
#if ENABLE(BRANCH_COMPACTION)
    ExecutableAllocator::makeExecutable(code(), m_initialSize);
#else
    ExecutableAllocator::makeExecutable(code(), m_size);
#endif
    MacroAssembler::cacheFlush(code(), m_size);
}

#if DUMP_LINK_STATISTICS
void LinkBuffer::dumpLinkStatistics(void* code, size_t initializeSize, size_t finalSize)
{
    static unsigned linkCount = 0;
    static unsigned totalInitialSize = 0;
    static unsigned totalFinalSize = 0;
    linkCount++;
    totalInitialSize += initialSize;
    totalFinalSize += finalSize;
    dataLogF("link %p: orig %u, compact %u (delta %u, %.2f%%)\n", 
            code, static_cast<unsigned>(initialSize), static_cast<unsigned>(finalSize),
            static_cast<unsigned>(initialSize - finalSize),
            100.0 * (initialSize - finalSize) / initialSize);
    dataLogF("\ttotal %u: orig %u, compact %u (delta %u, %.2f%%)\n", 
            linkCount, totalInitialSize, totalFinalSize, totalInitialSize - totalFinalSize,
            100.0 * (totalInitialSize - totalFinalSize) / totalInitialSize);
}
#endif

#if DUMP_CODE
void LinkBuffer::dumpCode(void* code, size_t size)
{
#if CPU(ARM_THUMB2)
    // Dump the generated code in an asm file format that can be assembled and then disassembled
    // for debugging purposes. For example, save this output as jit.s:
    //   gcc -arch armv7 -c jit.s
    //   otool -tv jit.o
    static unsigned codeCount = 0;
    unsigned short* tcode = static_cast<unsigned short*>(code);
    size_t tsize = size / sizeof(short);
    char nameBuf[128];
    snprintf(nameBuf, sizeof(nameBuf), "_jsc_jit%u", codeCount++);
    dataLogF("\t.syntax unified\n"
            "\t.section\t__TEXT,__text,regular,pure_instructions\n"
            "\t.globl\t%s\n"
            "\t.align 2\n"
            "\t.code 16\n"
            "\t.thumb_func\t%s\n"
            "# %p\n"
            "%s:\n", nameBuf, nameBuf, code, nameBuf);
        
    for (unsigned i = 0; i < tsize; i++)
        dataLogF("\t.short\t0x%x\n", tcode[i]);
#elif CPU(ARM_TRADITIONAL)
    //   gcc -c jit.s
    //   objdump -D jit.o
    static unsigned codeCount = 0;
    unsigned int* tcode = static_cast<unsigned int*>(code);
    size_t tsize = size / sizeof(unsigned int);
    char nameBuf[128];
    snprintf(nameBuf, sizeof(nameBuf), "_jsc_jit%u", codeCount++);
    dataLogF("\t.globl\t%s\n"
            "\t.align 4\n"
            "\t.code 32\n"
            "\t.text\n"
            "# %p\n"
            "%s:\n", nameBuf, code, nameBuf);

    for (unsigned i = 0; i < tsize; i++)
        dataLogF("\t.long\t0x%x\n", tcode[i]);
#endif
}
#endif

} // namespace JSC

#endif // ENABLE(ASSEMBLER)


