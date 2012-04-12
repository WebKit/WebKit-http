# Copyright (C) 2012 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

def isX64
    case $activeBackend
    when "X86"
        false
    when "X86_64"
        true
    else
        raise "bad value for $activeBackend: #{$activeBackend}"
    end
end

class SpecialRegister < NoChildren
    def x86Operand(kind)
        raise unless @name =~ /^r/
        raise unless isX64
        case kind
        when :half
            "%" + @name + "w"
        when :int
            "%" + @name + "d"
        when :ptr
            "%" + @name
        else
            raise
        end
    end
    def x86CallOperand(kind)
        "*#{x86Operand(kind)}"
    end
end

X64_SCRATCH_REGISTER = SpecialRegister.new("r11")

class RegisterID
    def supports8BitOnX86
        case name
        when "t0", "a0", "r0", "t1", "a1", "r1", "t2", "t3"
            true
        when "cfr", "ttnr", "tmr"
            false
        when "t4", "t5"
            isX64
        else
            raise
        end
    end
    
    def x86Operand(kind)
        case name
        when "t0", "a0", "r0"
            case kind
            when :byte
                "%al"
            when :half
                "%ax"
            when :int
                "%eax"
            when :ptr
                isX64 ? "%rax" : "%eax"
            else
                raise
            end
        when "t1", "a1", "r1"
            case kind
            when :byte
                "%dl"
            when :half
                "%dx"
            when :int
                "%edx"
            when :ptr
                isX64 ? "%rdx" : "%edx"
            else
                raise
            end
        when "t2"
            case kind
            when :byte
                "%cl"
            when :half
                "%cx"
            when :int
                "%ecx"
            when :ptr
                isX64 ? "%rcx" : "%ecx"
            else
                raise
            end
        when "t3"
            case kind
            when :byte
                "%bl"
            when :half
                "%bx"
            when :int
                "%ebx"
            when :ptr
                isX64 ? "%rbx" : "%ebx"
            else
                raise
            end
        when "t4"
            case kind
            when :byte
                "%sil"
            when :half
                "%si"
            when :int
                "%esi"
            when :ptr
                isX64 ? "%rsi" : "%esi"
            else
                raise
            end
        when "cfr"
            if isX64
                case kind
                when :half
                    "%r13w"
                when :int
                    "%r13d"
                when :ptr
                    "%r13"
                else
                    raise
                end
            else
                case kind
                when :byte
                    "%dil"
                when :half
                    "%di"
                when :int
                    "%edi"
                when :ptr
                    "%edi"
                else
                    raise
                end
            end
        when "sp"
            case kind
            when :byte
                "%spl"
            when :half
                "%sp"
            when :int
                "%esp"
            when :ptr
                isX64 ? "%rsp" : "%esp"
            else
                raise
            end
        when "t5"
            raise "Cannot use #{name} in 32-bit X86 at #{codeOriginString}" unless isX64
            case kind
            when :byte
                "%dil"
            when :half
                "%di"
            when :int
                "%edi"
            when :ptr
                "%rdi"
            end
        when "t6"
            raise "Cannot use #{name} in 32-bit X86 at #{codeOriginString}" unless isX64
            case kind
            when :half
                "%r10w"
            when :int
                "%r10d"
            when :ptr
                "%r10"
            end
        when "csr1"
            raise "Cannot use #{name} in 32-bit X86 at #{codeOriginString}" unless isX64
            case kind
            when :half
                "%r14w"
            when :int
                "%r14d"
            when :ptr
                "%r14"
            end
        when "csr2"
            raise "Cannot use #{name} in 32-bit X86 at #{codeOriginString}" unless isX64
            case kind
            when :half
                "%r15w"
            when :int
                "%r15d"
            when :ptr
                "%r15"
            end
        else
            raise "Bad register #{name} for X86 at #{codeOriginString}"
        end
    end
    def x86CallOperand(kind)
        "*#{x86Operand(kind)}"
    end
end

class FPRegisterID
    def x86Operand(kind)
        raise unless kind == :double
        case name
        when "ft0", "fa0", "fr"
            "%xmm0"
        when "ft1", "fa1"
            "%xmm1"
        when "ft2", "fa2"
            "%xmm2"
        when "ft3", "fa3"
            "%xmm3"
        when "ft4"
            "%xmm4"
        when "ft5"
            "%xmm5"
        else
            raise "Bad register #{name} for X86 at #{codeOriginString}"
        end
    end
    def x86CallOperand(kind)
        "*#{x86Operand(kind)}"
    end
end

class Immediate
    def validX86Immediate?
        if isX64
            value >= -0x80000000 and value <= 0x7fffffff
        else
            true
        end
    end
    def x86Operand(kind)
        "$#{value}"
    end
    def x86CallOperand(kind)
        "#{value}"
    end
end

class Address
    def supports8BitOnX86
        true
    end
    
    def x86AddressOperand(addressKind)
        "#{offset.value}(#{base.x86Operand(addressKind)})"
    end
    def x86Operand(kind)
        x86AddressOperand(:ptr)
    end
    def x86CallOperand(kind)
        "*#{x86Operand(kind)}"
    end
end

class BaseIndex
    def supports8BitOnX86
        true
    end
    
    def x86AddressOperand(addressKind)
        "#{offset.value}(#{base.x86Operand(addressKind)}, #{index.x86Operand(addressKind)}, #{scale})"
    end
    
    def x86Operand(kind)
        x86AddressOperand(:ptr)
    end

    def x86CallOperand(kind)
        "*#{x86Operand(kind)}"
    end
end

class AbsoluteAddress
    def supports8BitOnX86
        true
    end
    
    def x86AddressOperand(addressKind)
        "#{address.value}"
    end
    
    def x86Operand(kind)
        "#{address.value}"
    end

    def x86CallOperand(kind)
        "*#{address.value}"
    end
end

class LabelReference
    def x86CallOperand(kind)
        asmLabel
    end
end

class LocalLabelReference
    def x86CallOperand(kind)
        asmLabel
    end
end

class Sequence
    def getModifiedListX86_64
        newList = []
        
        @list.each {
            | node |
            newNode = node
            if node.is_a? Instruction
                unless node.opcode == "move"
                    usedScratch = false
                    newOperands = node.operands.map {
                        | operand |
                        if operand.immediate? and not operand.validX86Immediate?
                            if usedScratch
                                raise "Attempt to use scratch register twice at #{operand.codeOriginString}"
                            end
                            newList << Instruction.new(operand.codeOrigin, "move", [operand, X64_SCRATCH_REGISTER])
                            usedScratch = true
                            X64_SCRATCH_REGISTER
                        else
                            operand
                        end
                    }
                    newNode = Instruction.new(node.codeOrigin, node.opcode, newOperands)
                end
            else
                unless node.is_a? Label or
                        node.is_a? LocalLabel or
                        node.is_a? Skip
                    raise "Unexpected #{node.inspect} at #{node.codeOrigin}" 
                end
            end
            if newNode
                newList << newNode
            end
        }
        
        return newList
    end
end

class Instruction
    def x86Operands(*kinds)
        raise unless kinds.size == operands.size
        result = []
        kinds.size.times {
            | idx |
            result << operands[idx].x86Operand(kinds[idx])
        }
        result.join(", ")
    end

    def x86Suffix(kind)
        case kind
        when :byte
            "b"
        when :half
            "w"
        when :int
            "l"
        when :ptr
            isX64 ? "q" : "l"
        when :double
            "sd"
        else
            raise
        end
    end
    
    def x86Bytes(kind)
        case kind
        when :byte
            1
        when :half
            2
        when :int
            4
        when :ptr
            isX64 ? 8 : 4
        when :double
            8
        else
            raise
        end
    end
    
    def handleX86OpWithNumOperands(opcode, kind, numOperands)
        if numOperands == 3
            if operands[0] == operands[2]
                $asm.puts "#{opcode} #{operands[1].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
            elsif operands[1] == operands[2]
                $asm.puts "#{opcode} #{operands[0].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
            else
                $asm.puts "mov#{x86Suffix(kind)} #{operands[0].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
                $asm.puts "#{opcode} #{operands[1].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
            end
        else
            $asm.puts "#{opcode} #{operands[0].x86Operand(kind)}, #{operands[1].x86Operand(kind)}"
        end
    end
    
    def handleX86Op(opcode, kind)
        handleX86OpWithNumOperands(opcode, kind, operands.size)
    end
    
    def handleX86Shift(opcode, kind)
        if operands[0].is_a? Immediate or operands[0] == RegisterID.forName(nil, "t2")
            $asm.puts "#{opcode} #{operands[0].x86Operand(:byte)}, #{operands[1].x86Operand(kind)}"
        else
            cx = RegisterID.forName(nil, "t2")
            $asm.puts "xchg#{x86Suffix(:ptr)} #{operands[0].x86Operand(:ptr)}, #{cx.x86Operand(:ptr)}"
            $asm.puts "#{opcode} %cl, #{operands[1].x86Operand(kind)}"
            $asm.puts "xchg#{x86Suffix(:ptr)} #{operands[0].x86Operand(:ptr)}, #{cx.x86Operand(:ptr)}"
        end
    end
    
    def handleX86DoubleBranch(branchOpcode, mode)
        case mode
        when :normal
            $asm.puts "ucomisd #{operands[1].x86Operand(:double)}, #{operands[0].x86Operand(:double)}"
        when :reverse
            $asm.puts "ucomisd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:double)}"
        else
            raise mode.inspect
        end
        $asm.puts "#{branchOpcode} #{operands[2].asmLabel}"
    end
    
    def handleX86IntCompare(opcodeSuffix, kind)
        if operands[0].is_a? Immediate and operands[0].value == 0 and operands[1].is_a? RegisterID and (opcodeSuffix == "e" or opcodeSuffix == "ne")
            $asm.puts "test#{x86Suffix(kind)} #{operands[1].x86Operand(kind)}"
        elsif operands[1].is_a? Immediate and operands[1].value == 0 and operands[0].is_a? RegisterID and (opcodeSuffix == "e" or opcodeSuffix == "ne")
            $asm.puts "test#{x86Suffix(kind)} #{operands[0].x86Operand(kind)}"
        else
            $asm.puts "cmp#{x86Suffix(kind)} #{operands[1].x86Operand(kind)}, #{operands[0].x86Operand(kind)}"
        end
    end
    
    def handleX86IntBranch(branchOpcode, kind)
        handleX86IntCompare(branchOpcode[1..-1], kind)
        $asm.puts "#{branchOpcode} #{operands[2].asmLabel}"
    end
    
    def handleX86Set(setOpcode, operand)
        if operand.supports8BitOnX86
            $asm.puts "#{setOpcode} #{operand.x86Operand(:byte)}"
            $asm.puts "movzbl #{operand.x86Operand(:byte)}, #{operand.x86Operand(:int)}"
        else
            ax = RegisterID.new(nil, "t0")
            $asm.puts "xchg#{x86Suffix(:ptr)} #{operand.x86Operand(:ptr)}, #{ax.x86Operand(:ptr)}"
            $asm.puts "#{setOpcode} %al"
            $asm.puts "movzbl %al, %eax"
            $asm.puts "xchg#{x86Suffix(:ptr)} #{operand.x86Operand(:ptr)}, #{ax.x86Operand(:ptr)}"
        end
    end
    
    def handleX86IntCompareSet(setOpcode, kind)
        handleX86IntCompare(setOpcode[3..-1], kind)
        handleX86Set(setOpcode, operands[2])
    end
    
    def handleX86Test(kind)
        value = operands[0]
        case operands.size
        when 2
            mask = Immediate.new(codeOrigin, -1)
        when 3
            mask = operands[1]
        else
            raise "Expected 2 or 3 operands, but got #{operands.size} at #{codeOriginString}"
        end
        
        if mask.is_a? Immediate and mask.value == -1
            if value.is_a? RegisterID
                $asm.puts "test#{x86Suffix(kind)} #{value.x86Operand(kind)}, #{value.x86Operand(kind)}"
            else
                $asm.puts "cmp#{x86Suffix(kind)} $0, #{value.x86Operand(kind)}"
            end
        else
            $asm.puts "test#{x86Suffix(kind)} #{mask.x86Operand(kind)}, #{value.x86Operand(kind)}"
        end
    end
    
    def handleX86BranchTest(branchOpcode, kind)
        handleX86Test(kind)
        $asm.puts "#{branchOpcode} #{operands.last.asmLabel}"
    end
    
    def handleX86SetTest(setOpcode, kind)
        handleX86Test(kind)
        handleX86Set(setOpcode, operands.last)
    end
    
    def handleX86OpBranch(opcode, branchOpcode, kind)
        handleX86OpWithNumOperands(opcode, kind, operands.size - 1)
        case operands.size
        when 4
            jumpTarget = operands[3]
        when 3
            jumpTarget = operands[2]
        else
            raise self.inspect
        end
        $asm.puts "#{branchOpcode} #{jumpTarget.asmLabel}"
    end
    
    def handleX86SubBranch(branchOpcode, kind)
        if operands.size == 4 and operands[1] == operands[2]
            $asm.puts "neg#{x86Suffix(kind)} #{operands[2].x86Operand(kind)}"
            $asm.puts "add#{x86Suffix(kind)} #{operands[0].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
        else
            handleX86OpWithNumOperands("sub#{x86Suffix(kind)}", kind, operands.size - 1)
        end
        case operands.size
        when 4
            jumpTarget = operands[3]
        when 3
            jumpTarget = operands[2]
        else
            raise self.inspect
        end
        $asm.puts "#{branchOpcode} #{jumpTarget.asmLabel}"
    end
    
    def handleX86Add(kind)
        if operands.size == 3 and operands[0].is_a? Immediate
            raise unless operands[1].is_a? RegisterID
            raise unless operands[2].is_a? RegisterID
            if operands[0].value == 0
                unless operands[1] == operands[2]
                    $asm.puts "mov#{x86Suffix(kind)} #{operands[1].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
                end
            else
                $asm.puts "lea#{x86Suffix(kind)} #{operands[0].value}(#{operands[1].x86Operand(kind)}), #{operands[2].x86Operand(kind)}"
            end
        elsif operands.size == 3 and operands[0].is_a? RegisterID
            raise unless operands[1].is_a? RegisterID
            raise unless operands[2].is_a? RegisterID
            $asm.puts "lea#{x86Suffix(kind)} (#{operands[0].x86Operand(kind)}, #{operands[1].x86Operand(kind)}), #{operands[2].x86Operand(kind)}"
        else
            unless Immediate.new(nil, 0) == operands[0]
                $asm.puts "add#{x86Suffix(kind)} #{x86Operands(kind, kind)}"
            end
        end
    end
    
    def handleX86Sub(kind)
        if operands.size == 3 and operands[1] == operands[2]
            $asm.puts "neg#{x86Suffix(kind)} #{operands[2].x86Operand(kind)}"
            $asm.puts "add#{x86Suffix(kind)} #{operands[0].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
        else
            handleX86Op("sub#{x86Suffix(kind)}", kind)
        end
    end
    
    def handleX86Mul(kind)
        if operands.size == 3 and operands[0].is_a? Immediate
            $asm.puts "imul#{x86Suffix(kind)} #{x86Operands(kind, kind, kind)}"
        else
            # FIXME: could do some peephole in case the left operand is immediate and it's
            # a power of two.
            handleX86Op("imul#{x86Suffix(kind)}", kind)
        end
    end
    
    def handleMove
        if Immediate.new(nil, 0) == operands[0] and operands[1].is_a? RegisterID
            $asm.puts "xor#{x86Suffix(:ptr)} #{operands[1].x86Operand(:ptr)}, #{operands[1].x86Operand(:ptr)}"
        elsif operands[0] != operands[1]
            $asm.puts "mov#{x86Suffix(:ptr)} #{x86Operands(:ptr, :ptr)}"
        end
    end
    
    def lowerX86
        raise unless $activeBackend == "X86"
        lowerX86Common
    end
    
    def lowerX86_64
        raise unless $activeBackend == "X86_64"
        lowerX86Common
    end
    
    def lowerX86Common
        $asm.comment codeOriginString
        case opcode
        when "addi"
            handleX86Add(:int)
        when "addp"
            handleX86Add(:ptr)
        when "andi"
            handleX86Op("andl", :int)
        when "andp"
            handleX86Op("and#{x86Suffix(:ptr)}", :ptr)
        when "lshifti"
            handleX86Shift("sall", :int)
        when "lshiftp"
            handleX86Shift("sal#{x86Suffix(:ptr)}", :ptr)
        when "muli"
            handleX86Mul(:int)
        when "mulp"
            handleX86Mul(:ptr)
        when "negi"
            $asm.puts "negl #{x86Operands(:int)}"
        when "negp"
            $asm.puts "neg#{x86Suffix(:ptr)} #{x86Operands(:ptr)}"
        when "noti"
            $asm.puts "notl #{x86Operands(:int)}"
        when "ori"
            handleX86Op("orl", :int)
        when "orp"
            handleX86Op("or#{x86Suffix(:ptr)}", :ptr)
        when "rshifti"
            handleX86Shift("sarl", :int)
        when "rshiftp"
            handleX86Shift("sar#{x86Suffix(:ptr)}", :ptr)
        when "urshifti"
            handleX86Shift("shrl", :int)
        when "urshiftp"
            handleX86Shift("shr#{x86Suffix(:ptr)}", :ptr)
        when "subi"
            handleX86Sub(:int)
        when "subp"
            handleX86Sub(:ptr)
        when "xori"
            handleX86Op("xorl", :int)
        when "xorp"
            handleX86Op("xor#{x86Suffix(:ptr)}", :ptr)
        when "loadi", "storei"
            $asm.puts "movl #{x86Operands(:int, :int)}"
        when "loadis"
            if isX64
                $asm.puts "movslq #{x86Operands(:int, :ptr)}"
            else
                $asm.puts "movl #{x86Operands(:int, :int)}"
            end
        when "loadp", "storep"
            $asm.puts "mov#{x86Suffix(:ptr)} #{x86Operands(:ptr, :ptr)}"
        when "loadb"
            $asm.puts "movzbl #{operands[0].x86Operand(:byte)}, #{operands[1].x86Operand(:int)}"
        when "loadbs"
            $asm.puts "movsbl #{operands[0].x86Operand(:byte)}, #{operands[1].x86Operand(:int)}"
        when "loadh"
            $asm.puts "movzwl #{operands[0].x86Operand(:half)}, #{operands[1].x86Operand(:int)}"
        when "loadhs"
            $asm.puts "movswl #{operands[0].x86Operand(:half)}, #{operands[1].x86Operand(:int)}"
        when "storeb"
            $asm.puts "movb #{x86Operands(:byte, :byte)}"
        when "loadd", "moved", "stored"
            $asm.puts "movsd #{x86Operands(:double, :double)}"
        when "addd"
            $asm.puts "addsd #{x86Operands(:double, :double)}"
        when "divd"
            $asm.puts "divsd #{x86Operands(:double, :double)}"
        when "subd"
            $asm.puts "subsd #{x86Operands(:double, :double)}"
        when "muld"
            $asm.puts "mulsd #{x86Operands(:double, :double)}"
        when "sqrtd"
            $asm.puts "sqrtsd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:double)}"
        when "ci2d"
            $asm.puts "cvtsi2sd #{operands[0].x86Operand(:int)}, #{operands[1].x86Operand(:double)}"
        when "bdeq"
            isUnordered = LocalLabel.unique("bdeq")
            $asm.puts "ucomisd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:double)}"
            $asm.puts "jp #{LabelReference.new(codeOrigin, isUnordered).asmLabel}"
            $asm.puts "je #{LabelReference.new(codeOrigin, operands[2]).asmLabel}"
            isUnordered.lower("X86")
        when "bdneq"
            handleX86DoubleBranch("jne", :normal)
        when "bdgt"
            handleX86DoubleBranch("ja", :normal)
        when "bdgteq"
            handleX86DoubleBranch("jae", :normal)
        when "bdlt"
            handleX86DoubleBranch("ja", :reverse)
        when "bdlteq"
            handleX86DoubleBranch("jae", :reverse)
        when "bdequn"
            handleX86DoubleBranch("je", :normal)
        when "bdnequn"
            isUnordered = LocalLabel.unique("bdnequn")
            isEqual = LocalLabel.unique("bdnequn")
            $asm.puts "ucomisd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:double)}"
            $asm.puts "jp #{LabelReference.new(codeOrigin, isUnordered).asmLabel}"
            $asm.puts "je #{LabelReference.new(codeOrigin, isEqual).asmLabel}"
            isUnordered.lower("X86")
            $asm.puts "jmp #{operands[2].asmLabel}"
            isEqual.lower("X86")
        when "bdgtun"
            handleX86DoubleBranch("jb", :reverse)
        when "bdgtequn"
            handleX86DoubleBranch("jbe", :reverse)
        when "bdltun"
            handleX86DoubleBranch("jb", :normal)
        when "bdltequn"
            handleX86DoubleBranch("jbe", :normal)
        when "btd2i"
            $asm.puts "cvttsd2si #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:int)}"
            $asm.puts "cmpl $0x80000000 #{operands[1].x86Operand(:int)}"
            $asm.puts "je #{operands[2].asmLabel}"
        when "td2i"
            $asm.puts "cvttsd2si #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:int)}"
        when "bcd2i"
            $asm.puts "cvttsd2si #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:int)}"
            $asm.puts "testl #{operands[1].x86Operand(:int)}, #{operands[1].x86Operand(:int)}"
            $asm.puts "je #{operands[2].asmLabel}"
            $asm.puts "cvtsi2sd #{operands[1].x86Operand(:int)}, %xmm7"
            $asm.puts "ucomisd #{operands[0].x86Operand(:double)}, %xmm7"
            $asm.puts "jp #{operands[2].asmLabel}"
            $asm.puts "jne #{operands[2].asmLabel}"
        when "movdz"
            $asm.puts "xorpd #{operands[0].x86Operand(:double)}, #{operands[0].x86Operand(:double)}"
        when "pop"
            $asm.puts "pop #{operands[0].x86Operand(:ptr)}"
        when "push"
            $asm.puts "push #{operands[0].x86Operand(:ptr)}"
        when "move"
            handleMove
        when "sxi2p"
            if isX64
                $asm.puts "movslq #{operands[0].x86Operand(:int)}, #{operands[1].x86Operand(:ptr)}"
            else
                handleMove
            end
        when "zxi2p"
            if isX64
                $asm.puts "movl #{operands[0].x86Operand(:int)}, #{operands[1].x86Operand(:int)}"
            else
                handleMove
            end
        when "nop"
            $asm.puts "nop"
        when "bieq"
            handleX86IntBranch("je", :int)
        when "bpeq"
            handleX86IntBranch("je", :ptr)
        when "bineq"
            handleX86IntBranch("jne", :int)
        when "bpneq"
            handleX86IntBranch("jne", :ptr)
        when "bia"
            handleX86IntBranch("ja", :int)
        when "bpa"
            handleX86IntBranch("ja", :ptr)
        when "biaeq"
            handleX86IntBranch("jae", :int)
        when "bpaeq"
            handleX86IntBranch("jae", :ptr)
        when "bib"
            handleX86IntBranch("jb", :int)
        when "bpb"
            handleX86IntBranch("jb", :ptr)
        when "bibeq"
            handleX86IntBranch("jbe", :int)
        when "bpbeq"
            handleX86IntBranch("jbe", :ptr)
        when "bigt"
            handleX86IntBranch("jg", :int)
        when "bpgt"
            handleX86IntBranch("jg", :ptr)
        when "bigteq"
            handleX86IntBranch("jge", :int)
        when "bpgteq"
            handleX86IntBranch("jge", :ptr)
        when "bilt"
            handleX86IntBranch("jl", :int)
        when "bplt"
            handleX86IntBranch("jl", :ptr)
        when "bilteq"
            handleX86IntBranch("jle", :int)
        when "bplteq"
            handleX86IntBranch("jle", :ptr)
        when "bbeq"
            handleX86IntBranch("je", :byte)
        when "bbneq"
            handleX86IntBranch("jne", :byte)
        when "bba"
            handleX86IntBranch("ja", :byte)
        when "bbaeq"
            handleX86IntBranch("jae", :byte)
        when "bbb"
            handleX86IntBranch("jb", :byte)
        when "bbbeq"
            handleX86IntBranch("jbe", :byte)
        when "bbgt"
            handleX86IntBranch("jg", :byte)
        when "bbgteq"
            handleX86IntBranch("jge", :byte)
        when "bblt"
            handleX86IntBranch("jl", :byte)
        when "bblteq"
            handleX86IntBranch("jlteq", :byte)
        when "btio"
            handleX86BranchTest("jo", :int)
        when "btpo"
            handleX86BranchTest("jo", :ptr)
        when "btis"
            handleX86BranchTest("js", :int)
        when "btps"
            handleX86BranchTest("js", :ptr)
        when "btiz"
            handleX86BranchTest("jz", :int)
        when "btpz"
            handleX86BranchTest("jz", :ptr)
        when "btinz"
            handleX86BranchTest("jnz", :int)
        when "btpnz"
            handleX86BranchTest("jnz", :ptr)
        when "btbo"
            handleX86BranchTest("jo", :byte)
        when "btbs"
            handleX86BranchTest("js", :byte)
        when "btbz"
            handleX86BranchTest("jz", :byte)
        when "btbnz"
            handleX86BranchTest("jnz", :byte)
        when "jmp"
            $asm.puts "jmp #{operands[0].x86CallOperand(:ptr)}"
        when "baddio"
            handleX86OpBranch("addl", "jo", :int)
        when "baddpo"
            handleX86OpBranch("add#{x86Suffix(:ptr)}", "jo", :ptr)
        when "baddis"
            handleX86OpBranch("addl", "js", :int)
        when "baddps"
            handleX86OpBranch("add#{x86Suffix(:ptr)}", "js", :ptr)
        when "baddiz"
            handleX86OpBranch("addl", "jz", :int)
        when "baddpz"
            handleX86OpBranch("add#{x86Suffix(:ptr)}", "jz", :ptr)
        when "baddinz"
            handleX86OpBranch("addl", "jnz", :int)
        when "baddpnz"
            handleX86OpBranch("add#{x86Suffix(:ptr)}", "jnz", :ptr)
        when "bsubio"
            handleX86SubBranch("jo", :int)
        when "bsubis"
            handleX86SubBranch("js", :int)
        when "bsubiz"
            handleX86SubBranch("jz", :int)
        when "bsubinz"
            handleX86SubBranch("jnz", :int)
        when "bmulio"
            handleX86OpBranch("imull", "jo", :int)
        when "bmulis"
            handleX86OpBranch("imull", "js", :int)
        when "bmuliz"
            handleX86OpBranch("imull", "jz", :int)
        when "bmulinz"
            handleX86OpBranch("imull", "jnz", :int)
        when "borio"
            handleX86OpBranch("orl", "jo", :int)
        when "boris"
            handleX86OpBranch("orl", "js", :int)
        when "boriz"
            handleX86OpBranch("orl", "jz", :int)
        when "borinz"
            handleX86OpBranch("orl", "jnz", :int)
        when "break"
            $asm.puts "int $3"
        when "call"
            $asm.puts "call #{operands[0].x86CallOperand(:ptr)}"
        when "ret"
            $asm.puts "ret"
        when "cieq"
            handleX86IntCompareSet("sete", :int)
        when "cbeq"
            handleX86IntCompareSet("sete", :byte)
        when "cpeq"
            handleX86IntCompareSet("sete", :ptr)
        when "cineq"
            handleX86IntCompareSet("setne", :int)
        when "cbneq"
            handleX86IntCompareSet("setne", :byte)
        when "cpneq"
            handleX86IntCompareSet("setne", :ptr)
        when "cia"
            handleX86IntCompareSet("seta", :int)
        when "cba"
            handleX86IntCompareSet("seta", :byte)
        when "cpa"
            handleX86IntCompareSet("seta", :ptr)
        when "ciaeq"
            handleX86IntCompareSet("setae", :int)
        when "cbaeq"
            handleX86IntCompareSet("setae", :byte)
        when "cpaeq"
            handleX86IntCompareSet("setae", :ptr)
        when "cib"
            handleX86IntCompareSet("setb", :int)
        when "cbb"
            handleX86IntCompareSet("setb", :byte)
        when "cpb"
            handleX86IntCompareSet("setb", :ptr)
        when "cibeq"
            handleX86IntCompareSet("setbe", :int)
        when "cbbeq"
            handleX86IntCompareSet("setbe", :byte)
        when "cpbeq"
            handleX86IntCompareSet("setbe", :ptr)
        when "cigt"
            handleX86IntCompareSet("setg", :int)
        when "cbgt"
            handleX86IntCompareSet("setg", :byte)
        when "cpgt"
            handleX86IntCompareSet("setg", :ptr)
        when "cigteq"
            handleX86IntCompareSet("setge", :int)
        when "cbgteq"
            handleX86IntCompareSet("setge", :byte)
        when "cpgteq"
            handleX86IntCompareSet("setge", :ptr)
        when "cilt"
            handleX86IntCompareSet("setl", :int)
        when "cblt"
            handleX86IntCompareSet("setl", :byte)
        when "cplt"
            handleX86IntCompareSet("setl", :ptr)
        when "cilteq"
            handleX86IntCompareSet("setle", :int)
        when "cblteq"
            handleX86IntCompareSet("setle", :byte)
        when "cplteq"
            handleX86IntCompareSet("setle", :ptr)
        when "tio"
            handleX86SetTest("seto", :int)
        when "tis"
            handleX86SetTest("sets", :int)
        when "tiz"
            handleX86SetTest("setz", :int)
        when "tinz"
            handleX86SetTest("setnz", :int)
        when "tpo"
            handleX86SetTest("seto", :ptr)
        when "tps"
            handleX86SetTest("sets", :ptr)
        when "tpz"
            handleX86SetTest("setz", :ptr)
        when "tpnz"
            handleX86SetTest("setnz", :ptr)
        when "tbo"
            handleX86SetTest("seto", :byte)
        when "tbs"
            handleX86SetTest("sets", :byte)
        when "tbz"
            handleX86SetTest("setz", :byte)
        when "tbnz"
            handleX86SetTest("setnz", :byte)
        when "peek"
            sp = RegisterID.new(nil, "sp")
            $asm.puts "mov#{x86Suffix(:ptr)} #{operands[0].value * x86Bytes(:ptr)}(#{sp.x86Operand(:ptr)}), #{operands[1].x86Operand(:ptr)}"
        when "poke"
            sp = RegisterID.new(nil, "sp")
            $asm.puts "mov#{x86Suffix(:ptr)} #{operands[0].x86Operand(:ptr)}, #{operands[1].value * x86Bytes(:ptr)}(#{sp.x86Operand(:ptr)})"
        when "cdqi"
            $asm.puts "cdq"
        when "idivi"
            $asm.puts "idivl #{operands[0].x86Operand(:int)}"
        when "fii2d"
            $asm.puts "movd #{operands[0].x86Operand(:int)}, #{operands[2].x86Operand(:double)}"
            $asm.puts "movd #{operands[1].x86Operand(:int)}, %xmm7"
            $asm.puts "psllq $32, %xmm7"
            $asm.puts "por %xmm7, #{operands[2].x86Operand(:double)}"
        when "fd2ii"
            $asm.puts "movd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:int)}"
            $asm.puts "movsd #{operands[0].x86Operand(:double)}, %xmm7"
            $asm.puts "psrlq $32, %xmm7"
            $asm.puts "movsd %xmm7, #{operands[2].x86Operand(:int)}"
        when "fp2d"
            $asm.puts "movd #{operands[0].x86Operand(:ptr)}, #{operands[1].x86Operand(:double)}"
        when "fd2p"
            $asm.puts "movd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:ptr)}"
        when "bo"
            $asm.puts "jo #{operands[0].asmLabel}"
        when "bs"
            $asm.puts "js #{operands[0].asmLabel}"
        when "bz"
            $asm.puts "jz #{operands[0].asmLabel}"
        when "bnz"
            $asm.puts "jnz #{operands[0].asmLabel}"
        when "leai"
            $asm.puts "leal #{operands[0].x86AddressOperand(:int)}, #{operands[1].x86Operand(:int)}"
        when "leap"
            $asm.puts "lea#{x86Suffix(:ptr)} #{operands[0].x86AddressOperand(:ptr)}, #{operands[1].x86Operand(:ptr)}"
        else
            raise "Bad opcode: #{opcode}"
        end
    end
end

