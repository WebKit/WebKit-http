/*
 * Copyright (C) 2008, 2009, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
 * Copyright (C) 2012 Igalia, S.L.
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

#ifndef BytecodeGenerator_h
#define BytecodeGenerator_h

#include "CodeBlock.h"
#include <wtf/HashTraits.h>
#include "Instruction.h"
#include "Label.h"
#include "LabelScope.h"
#include "Interpreter.h"
#include "RegisterID.h"
#include "SymbolTable.h"
#include "Debugger.h"
#include "Nodes.h"
#include "UnlinkedCodeBlock.h"
#include <wtf/PassRefPtr.h>
#include <wtf/SegmentedVector.h>
#include <wtf/Vector.h>

namespace JSC {

    class Identifier;
    class Label;
    class JSScope;

    enum ExpectedFunction {
        NoExpectedFunction,
        ExpectObjectConstructor,
        ExpectArrayConstructor
    };

    class CallArguments {
    public:
        CallArguments(BytecodeGenerator& generator, ArgumentsNode* argumentsNode);

        RegisterID* thisRegister() { return m_argv[0].get(); }
        RegisterID* argumentRegister(unsigned i) { return m_argv[i + 1].get(); }
        unsigned registerOffset() { return m_argv.last()->index() + CallFrame::offsetFor(argumentCountIncludingThis()); }
        unsigned argumentCountIncludingThis() { return m_argv.size(); }
        RegisterID* profileHookRegister() { return m_profileHookRegister.get(); }
        ArgumentsNode* argumentsNode() { return m_argumentsNode; }

    private:
        void newArgument(BytecodeGenerator&);

        RefPtr<RegisterID> m_profileHookRegister;
        ArgumentsNode* m_argumentsNode;
        Vector<RefPtr<RegisterID>, 8> m_argv;
    };

    struct FinallyContext {
        StatementNode* finallyBlock;
        unsigned scopeContextStackSize;
        unsigned switchContextStackSize;
        unsigned forInContextStackSize;
        unsigned tryContextStackSize;
        unsigned labelScopesSize;
        int finallyDepth;
        int dynamicScopeDepth;
    };

    struct ControlFlowContext {
        bool isFinallyBlock;
        FinallyContext finallyContext;
    };

    struct ForInContext {
        RefPtr<RegisterID> expectedSubscriptRegister;
        RefPtr<RegisterID> iterRegister;
        RefPtr<RegisterID> indexRegister;
        RefPtr<RegisterID> propertyRegister;
    };

    struct TryData {
        RefPtr<Label> target;
        unsigned targetScopeDepth;
    };

    struct TryContext {
        RefPtr<Label> start;
        TryData* tryData;
    };

    struct TryRange {
        RefPtr<Label> start;
        RefPtr<Label> end;
        TryData* tryData;
    };

    class ResolveResult {
    public:
        enum Flags {
            // The property is locally bound, in a register.
            RegisterFlag = 0x1,
            // We need to traverse the scope chain at runtime, checking for
            // non-strict eval and/or `with' nodes.
            DynamicFlag = 0x2,
            // The resolved binding is immutable.
            ReadOnlyFlag = 0x4,
        };

        enum Type {
            // The property is local, and stored in a register.
            Register = RegisterFlag,
            // A read-only local, created by "const".
            ReadOnlyRegister = RegisterFlag | ReadOnlyFlag,
            // Any form of non-local lookup
            Dynamic = DynamicFlag,
        };

        static ResolveResult registerResolve(RegisterID *local, unsigned flags)
        {
            return ResolveResult(Register | flags, local);
        }
        static ResolveResult dynamicResolve()
        {
            return ResolveResult(Dynamic, 0);
        }
        unsigned type() const { return m_type; }

        // Returns the register corresponding to a local variable, or 0 if no
        // such register exists. Registers returned by ResolveResult::local() do
        // not require explicit reference counting.
        RegisterID* local() const { return m_local; }

        bool isRegister() const { return m_type & RegisterFlag; }
        bool isDynamic() const { return m_type & DynamicFlag; }
        bool isReadOnly() const { return (m_type & ReadOnlyFlag) && !isDynamic(); }

    private:
        ResolveResult(unsigned type, RegisterID* local)
            : m_type(type)
            , m_local(local)
        {
#ifndef NDEBUG
            checkValidity();
#endif
        }

#ifndef NDEBUG
        void checkValidity();
#endif

        unsigned m_type;
        RegisterID* m_local; // Local register, if RegisterFlag is set
    };

    struct NonlocalResolveInfo {
        friend class BytecodeGenerator;
        NonlocalResolveInfo()
            : m_state(Unused)
        {
        }
        ~NonlocalResolveInfo()
        {
            ASSERT(m_state == Put);
        }
    private:
        void resolved(uint32_t putToBaseIndex)
        {
            ASSERT(putToBaseIndex);
            ASSERT(m_state == Unused);
            m_state = Resolved;
            m_putToBaseIndex = putToBaseIndex;
        }
        uint32_t put()
        {
            ASSERT(m_state == Resolved);
            m_state = Put;
            return m_putToBaseIndex;
        }
        enum State { Unused, Resolved, Put };
        State m_state;
        uint32_t m_putToBaseIndex;
    };

    class BytecodeGenerator {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        typedef DeclarationStacks::VarStack VarStack;
        typedef DeclarationStacks::FunctionStack FunctionStack;

        BytecodeGenerator(JSGlobalData&, ProgramNode*, UnlinkedProgramCodeBlock*, DebuggerMode, ProfilerMode);
        BytecodeGenerator(JSGlobalData&, FunctionBodyNode*, UnlinkedFunctionCodeBlock*, DebuggerMode, ProfilerMode);
        BytecodeGenerator(JSGlobalData&, EvalNode*, UnlinkedEvalCodeBlock*, DebuggerMode, ProfilerMode);

        ~BytecodeGenerator();
        
        JSGlobalData* globalData() const { return m_globalData; }
        const CommonIdentifiers& propertyNames() const { return *m_globalData->propertyNames; }

        bool isConstructor() { return m_codeBlock->isConstructor(); }

        ParserError generate();

        bool isArgumentNumber(const Identifier&, int);

        void setIsNumericCompareFunction(bool isNumericCompareFunction);

        bool willResolveToArguments(const Identifier&);
        RegisterID* uncheckedRegisterForArguments();

        // Resolve an identifier, given the current compile-time scope chain.
        ResolveResult resolve(const Identifier&);
        // Behaves as resolve does, but ignores dynamic scope as
        // dynamic scope should not interfere with const initialisation
        ResolveResult resolveConstDecl(const Identifier&);

        // Returns the register storing "this"
        RegisterID* thisRegister() { return &m_thisRegister; }

        // Returns the next available temporary register. Registers returned by
        // newTemporary require a modified form of reference counting: any
        // register with a refcount of 0 is considered "available", meaning that
        // the next instruction may overwrite it.
        RegisterID* newTemporary();

        // The same as newTemporary(), but this function returns "suggestion" if
        // "suggestion" is a temporary. This function is helpful in situations
        // where you've put "suggestion" in a RefPtr, but you'd like to allow
        // the next instruction to overwrite it anyway.
        RegisterID* newTemporaryOr(RegisterID* suggestion) { return suggestion->isTemporary() ? suggestion : newTemporary(); }

        // Functions for handling of dst register

        RegisterID* ignoredResult() { return &m_ignoredResultRegister; }

        // Returns a place to write intermediate values of an operation
        // which reuses dst if it is safe to do so.
        RegisterID* tempDestination(RegisterID* dst)
        {
            return (dst && dst != ignoredResult() && dst->isTemporary()) ? dst : newTemporary();
        }

        // Returns the place to write the final output of an operation.
        RegisterID* finalDestination(RegisterID* originalDst, RegisterID* tempDst = 0)
        {
            if (originalDst && originalDst != ignoredResult())
                return originalDst;
            ASSERT(tempDst != ignoredResult());
            if (tempDst && tempDst->isTemporary())
                return tempDst;
            return newTemporary();
        }

        // Returns the place to write the final output of an operation.
        RegisterID* finalDestinationOrIgnored(RegisterID* originalDst, RegisterID* tempDst = 0)
        {
            if (originalDst)
                return originalDst;
            ASSERT(tempDst != ignoredResult());
            if (tempDst && tempDst->isTemporary())
                return tempDst;
            return newTemporary();
        }

        RegisterID* destinationForAssignResult(RegisterID* dst)
        {
            if (dst && dst != ignoredResult() && m_codeBlock->needsFullScopeChain())
                return dst->isTemporary() ? dst : newTemporary();
            return 0;
        }

        // Moves src to dst if dst is not null and is different from src, otherwise just returns src.
        RegisterID* moveToDestinationIfNeeded(RegisterID* dst, RegisterID* src)
        {
            return dst == ignoredResult() ? 0 : (dst && dst != src) ? emitMove(dst, src) : src;
        }

        PassRefPtr<LabelScope> newLabelScope(LabelScope::Type, const Identifier* = 0);
        PassRefPtr<Label> newLabel();

        // The emitNode functions are just syntactic sugar for calling
        // Node::emitCode. These functions accept a 0 for the register,
        // meaning that the node should allocate a register, or ignoredResult(),
        // meaning that the node need not put the result in a register.
        // Other emit functions do not accept 0 or ignoredResult().
        RegisterID* emitNode(RegisterID* dst, Node* n)
        {
            // Node::emitCode assumes that dst, if provided, is either a local or a referenced temporary.
            ASSERT(!dst || dst == ignoredResult() || !dst->isTemporary() || dst->refCount());
            addLineInfo(n->lineNo());
            return m_stack.isSafeToRecurse()
                ? n->emitBytecode(*this, dst)
                : emitThrowExpressionTooDeepException();
        }

        RegisterID* emitNode(Node* n)
        {
            return emitNode(0, n);
        }

        void emitNodeInConditionContext(ExpressionNode* n, Label* trueTarget, Label* falseTarget, bool fallThroughMeansTrue)
        {
            addLineInfo(n->lineNo());
            if (m_stack.isSafeToRecurse())
                n->emitBytecodeInConditionContext(*this, trueTarget, falseTarget, fallThroughMeansTrue);
            else
                emitThrowExpressionTooDeepException();
        }

        void emitExpressionInfo(unsigned divot, unsigned startOffset, unsigned endOffset)
        {
            divot -= m_scopeNode->source().startOffset();
            if (divot > ExpressionRangeInfo::MaxDivot) {
                // Overflow has occurred, we can only give line number info for errors for this region
                divot = 0;
                startOffset = 0;
                endOffset = 0;
            } else if (startOffset > ExpressionRangeInfo::MaxOffset) {
                // If the start offset is out of bounds we clear both offsets
                // so we only get the divot marker.  Error message will have to be reduced
                // to line and column number.
                startOffset = 0;
                endOffset = 0;
            } else if (endOffset > ExpressionRangeInfo::MaxOffset) {
                // The end offset is only used for additional context, and is much more likely
                // to overflow (eg. function call arguments) so we are willing to drop it without
                // dropping the rest of the range.
                endOffset = 0;
            }
            
            ExpressionRangeInfo info;
            info.instructionOffset = instructions().size();
            info.divotPoint = divot;
            info.startOffset = startOffset;
            info.endOffset = endOffset;
            m_codeBlock->addExpressionInfo(info);
        }

        ALWAYS_INLINE bool leftHandSideNeedsCopy(bool rightHasAssignments, bool rightIsPure)
        {
            return (m_codeType != FunctionCode || m_codeBlock->needsFullScopeChain() || rightHasAssignments) && !rightIsPure;
        }

        ALWAYS_INLINE PassRefPtr<RegisterID> emitNodeForLeftHandSide(ExpressionNode* n, bool rightHasAssignments, bool rightIsPure)
        {
            if (leftHandSideNeedsCopy(rightHasAssignments, rightIsPure)) {
                PassRefPtr<RegisterID> dst = newTemporary();
                emitNode(dst.get(), n);
                return dst;
            }

            return emitNode(n);
        }

        RegisterID* emitLoad(RegisterID* dst, bool);
        RegisterID* emitLoad(RegisterID* dst, double);
        RegisterID* emitLoad(RegisterID* dst, const Identifier&);
        RegisterID* emitLoad(RegisterID* dst, JSValue);

        RegisterID* emitUnaryOp(OpcodeID, RegisterID* dst, RegisterID* src);
        RegisterID* emitBinaryOp(OpcodeID, RegisterID* dst, RegisterID* src1, RegisterID* src2, OperandTypes);
        RegisterID* emitEqualityOp(OpcodeID, RegisterID* dst, RegisterID* src1, RegisterID* src2);
        RegisterID* emitUnaryNoDstOp(OpcodeID, RegisterID* src);

        RegisterID* emitNewObject(RegisterID* dst);
        RegisterID* emitNewArray(RegisterID* dst, ElementNode*, unsigned length); // stops at first elision

        RegisterID* emitNewFunction(RegisterID* dst, FunctionBodyNode* body);
        RegisterID* emitLazyNewFunction(RegisterID* dst, FunctionBodyNode* body);
        RegisterID* emitNewFunctionInternal(RegisterID* dst, unsigned index, bool shouldNullCheck);
        RegisterID* emitNewFunctionExpression(RegisterID* dst, FuncExprNode* func);
        RegisterID* emitNewRegExp(RegisterID* dst, RegExp*);

        RegisterID* emitMove(RegisterID* dst, RegisterID* src);

        RegisterID* emitToJSNumber(RegisterID* dst, RegisterID* src) { return emitUnaryOp(op_to_jsnumber, dst, src); }
        RegisterID* emitPreInc(RegisterID* srcDst);
        RegisterID* emitPreDec(RegisterID* srcDst);
        RegisterID* emitPostInc(RegisterID* dst, RegisterID* srcDst);
        RegisterID* emitPostDec(RegisterID* dst, RegisterID* srcDst);

        void emitCheckHasInstance(RegisterID* dst, RegisterID* value, RegisterID* base, Label* target);
        RegisterID* emitInstanceOf(RegisterID* dst, RegisterID* value, RegisterID* basePrototype);
        RegisterID* emitTypeOf(RegisterID* dst, RegisterID* src) { return emitUnaryOp(op_typeof, dst, src); }
        RegisterID* emitIn(RegisterID* dst, RegisterID* property, RegisterID* base) { return emitBinaryOp(op_in, dst, property, base, OperandTypes()); }

        RegisterID* emitGetLocalVar(RegisterID* dst, const ResolveResult&, const Identifier&);
        RegisterID* emitInitGlobalConst(const Identifier&, RegisterID* value);

        RegisterID* emitResolve(RegisterID* dst, const ResolveResult&, const Identifier& property);
        RegisterID* emitResolveBase(RegisterID* dst, const ResolveResult&, const Identifier& property);
        RegisterID* emitResolveBaseForPut(RegisterID* dst, const ResolveResult&, const Identifier& property, NonlocalResolveInfo&);
        RegisterID* emitResolveWithBaseForPut(RegisterID* baseDst, RegisterID* propDst, const ResolveResult&, const Identifier& property, NonlocalResolveInfo&);
        RegisterID* emitResolveWithThis(RegisterID* baseDst, RegisterID* propDst, const ResolveResult&, const Identifier& property);

        RegisterID* emitPutToBase(RegisterID* base, const Identifier&, RegisterID* value, NonlocalResolveInfo&);

        RegisterID* emitGetById(RegisterID* dst, RegisterID* base, const Identifier& property);
        RegisterID* emitGetArgumentsLength(RegisterID* dst, RegisterID* base);
        RegisterID* emitPutById(RegisterID* base, const Identifier& property, RegisterID* value);
        RegisterID* emitDirectPutById(RegisterID* base, const Identifier& property, RegisterID* value);
        RegisterID* emitDeleteById(RegisterID* dst, RegisterID* base, const Identifier&);
        RegisterID* emitGetByVal(RegisterID* dst, RegisterID* base, RegisterID* property);
        RegisterID* emitGetArgumentByVal(RegisterID* dst, RegisterID* base, RegisterID* property);
        RegisterID* emitPutByVal(RegisterID* base, RegisterID* property, RegisterID* value);
        RegisterID* emitDeleteByVal(RegisterID* dst, RegisterID* base, RegisterID* property);
        RegisterID* emitPutByIndex(RegisterID* base, unsigned index, RegisterID* value);
        void emitPutGetterSetter(RegisterID* base, const Identifier& property, RegisterID* getter, RegisterID* setter);
        
        ExpectedFunction expectedFunctionForIdentifier(const Identifier&);
        RegisterID* emitCall(RegisterID* dst, RegisterID* func, ExpectedFunction, CallArguments&, unsigned divot, unsigned startOffset, unsigned endOffset);
        RegisterID* emitCallEval(RegisterID* dst, RegisterID* func, CallArguments&, unsigned divot, unsigned startOffset, unsigned endOffset);
        RegisterID* emitCallVarargs(RegisterID* dst, RegisterID* func, RegisterID* thisRegister, RegisterID* arguments, RegisterID* firstFreeRegister, RegisterID* profileHookRegister, unsigned divot, unsigned startOffset, unsigned endOffset);
        RegisterID* emitLoadVarargs(RegisterID* argCountDst, RegisterID* thisRegister, RegisterID* args);

        RegisterID* emitReturn(RegisterID* src);
        RegisterID* emitEnd(RegisterID* src) { return emitUnaryNoDstOp(op_end, src); }

        RegisterID* emitConstruct(RegisterID* dst, RegisterID* func, ExpectedFunction, CallArguments&, unsigned divot, unsigned startOffset, unsigned endOffset);
        RegisterID* emitStrcat(RegisterID* dst, RegisterID* src, int count);
        void emitToPrimitive(RegisterID* dst, RegisterID* src);

        PassRefPtr<Label> emitLabel(Label*);
        void emitLoopHint();
        PassRefPtr<Label> emitJump(Label* target);
        PassRefPtr<Label> emitJumpIfTrue(RegisterID* cond, Label* target);
        PassRefPtr<Label> emitJumpIfFalse(RegisterID* cond, Label* target);
        PassRefPtr<Label> emitJumpIfNotFunctionCall(RegisterID* cond, Label* target);
        PassRefPtr<Label> emitJumpIfNotFunctionApply(RegisterID* cond, Label* target);
        PassRefPtr<Label> emitJumpScopes(Label* target, int targetScopeDepth);

        RegisterID* emitGetPropertyNames(RegisterID* dst, RegisterID* base, RegisterID* i, RegisterID* size, Label* breakTarget);
        RegisterID* emitNextPropertyName(RegisterID* dst, RegisterID* base, RegisterID* i, RegisterID* size, RegisterID* iter, Label* target);

        void emitReadOnlyExceptionIfNeeded();

        // Start a try block. 'start' must have been emitted.
        TryData* pushTry(Label* start);
        // End a try block. 'end' must have been emitted.
        RegisterID* popTryAndEmitCatch(TryData*, RegisterID* targetRegister, Label* end);

        void emitThrow(RegisterID* exc)
        { 
            m_usesExceptions = true;
            emitUnaryNoDstOp(op_throw, exc);
        }

        void emitThrowReferenceError(const String& message);

        void emitPushNameScope(const Identifier& property, RegisterID* value, unsigned attributes);

        RegisterID* emitPushWithScope(RegisterID* scope);
        void emitPopScope();

        void emitDebugHook(DebugHookID, int firstLine, int lastLine, int column);

        int scopeDepth() { return m_dynamicScopeDepth + m_finallyDepth; }
        bool hasFinaliser() { return m_finallyDepth != 0; }

        void pushFinallyContext(StatementNode* finallyBlock);
        void popFinallyContext();

        void pushOptimisedForIn(RegisterID* expectedBase, RegisterID* iter, RegisterID* index, RegisterID* propertyRegister)
        {
            ForInContext context = { expectedBase, iter, index, propertyRegister };
            m_forInContextStack.append(context);
        }

        void popOptimisedForIn()
        {
            m_forInContextStack.removeLast();
        }

        LabelScope* breakTarget(const Identifier&);
        LabelScope* continueTarget(const Identifier&);

        void beginSwitch(RegisterID*, SwitchInfo::SwitchType);
        void endSwitch(uint32_t clauseCount, RefPtr<Label>*, ExpressionNode**, Label* defaultLabel, int32_t min, int32_t range);

        CodeType codeType() const { return m_codeType; }

        bool shouldEmitProfileHooks() { return m_shouldEmitProfileHooks; }
        
        bool isStrictMode() const { return m_codeBlock->isStrictMode(); }

    private:
        friend class Label;
        
#if ENABLE(BYTECODE_COMMENTS)
        // Record a comment in the CodeBlock's comments list for the current
        // opcode that is about to be emitted.
        void emitComment();
        // Register a comment to be associated with the next opcode that will
        // be emitted.
        void prependComment(const char* string);
#else
        ALWAYS_INLINE void emitComment() { }
        ALWAYS_INLINE void prependComment(const char*) { }
#endif

        void emitOpcode(OpcodeID);
        UnlinkedArrayAllocationProfile newArrayAllocationProfile();
        UnlinkedArrayProfile newArrayProfile();
        UnlinkedValueProfile emitProfiledOpcode(OpcodeID);
        void retrieveLastBinaryOp(int& dstIndex, int& src1Index, int& src2Index);
        void retrieveLastUnaryOp(int& dstIndex, int& srcIndex);
        ALWAYS_INLINE void rewindBinaryOp();
        ALWAYS_INLINE void rewindUnaryOp();

        PassRefPtr<Label> emitComplexJumpScopes(Label* target, ControlFlowContext* topScope, ControlFlowContext* bottomScope);

        typedef HashMap<double, JSValue> NumberMap;
        typedef HashMap<StringImpl*, JSString*, IdentifierRepHash> IdentifierStringMap;
        typedef struct {
            int resolveOperations;
            int putOperations;
        } ResolveCacheEntry;
        typedef HashMap<StringImpl*, ResolveCacheEntry, IdentifierRepHash> IdentifierResolvePutMap;
        typedef HashMap<StringImpl*, uint32_t, IdentifierRepHash> IdentifierResolveMap;
        
        // Helper for emitCall() and emitConstruct(). This works because the set of
        // expected functions have identical behavior for both call and construct
        // (i.e. "Object()" is identical to "new Object()").
        ExpectedFunction emitExpectedFunctionSnippet(RegisterID* dst, RegisterID* func, ExpectedFunction, CallArguments&, Label* done);
        
        RegisterID* emitCall(OpcodeID, RegisterID* dst, RegisterID* func, ExpectedFunction, CallArguments&, unsigned divot, unsigned startOffset, unsigned endOffset);

        RegisterID* newRegister();

        // Adds a var slot and maps it to the name ident in symbolTable().
        RegisterID* addVar(const Identifier& ident, bool isConstant)
        {
            RegisterID* local;
            addVar(ident, isConstant, local);
            return local;
        }

        // Ditto. Returns true if a new RegisterID was added, false if a pre-existing RegisterID was re-used.
        bool addVar(const Identifier&, bool isConstant, RegisterID*&);
        
        // Adds an anonymous var slot. To give this slot a name, add it to symbolTable().
        RegisterID* addVar()
        {
            ++m_codeBlock->m_numVars;
            return newRegister();
        }

        // Returns the index of the added var.
        void addParameter(const Identifier&, int parameterIndex);
        RegisterID* resolveCallee(FunctionBodyNode*);
        void addCallee(FunctionBodyNode*, RegisterID*);

        void preserveLastVar();
        bool shouldAvoidResolveGlobal();

        RegisterID& registerFor(int index)
        {
            if (index >= 0)
                return m_calleeRegisters[index];

            if (index == JSStack::Callee)
                return m_calleeRegister;

            ASSERT(m_parameters.size());
            return m_parameters[index + m_parameters.size() + JSStack::CallFrameHeaderSize];
        }

        unsigned addConstant(const Identifier&);
        RegisterID* addConstantValue(JSValue);
        RegisterID* addConstantEmptyValue();
        unsigned addRegExp(RegExp*);

        unsigned addConstantBuffer(unsigned length);
        
        UnlinkedFunctionExecutable* makeFunction(FunctionBodyNode* body)
        {
            return UnlinkedFunctionExecutable::create(m_globalData, m_scopeNode->source(), body);
        }

        JSString* addStringConstant(const Identifier&);

        void addLineInfo(unsigned lineNo)
        {
            m_codeBlock->addLineInfo(instructions().size(), lineNo - m_scopeNode->firstLine());
        }

        RegisterID* emitInitLazyRegister(RegisterID*);

    public:
        Vector<UnlinkedInstruction>& instructions() { return m_instructions; }

        SharedSymbolTable& symbolTable() { return *m_symbolTable; }
#if ENABLE(BYTECODE_COMMENTS)
        Vector<Comment>& comments() { return m_comments; }
#endif

        bool shouldOptimizeLocals()
        {
            if (m_dynamicScopeDepth)
                return false;

            if (m_codeType != FunctionCode)
                return false;

            return true;
        }

        bool canOptimizeNonLocals()
        {
            if (m_dynamicScopeDepth)
                return false;

            if (m_codeType == EvalCode)
                return false;

            if (m_codeType == FunctionCode && m_codeBlock->usesEval())
                return false;

            return true;
        }

        RegisterID* emitThrowExpressionTooDeepException();

        void createArgumentsIfNecessary();
        void createActivationIfNecessary();
        RegisterID* createLazyRegisterIfNecessary(RegisterID*);
        
        Vector<UnlinkedInstruction> m_instructions;

        bool m_shouldEmitDebugHooks;
        bool m_shouldEmitProfileHooks;

        SharedSymbolTable* m_symbolTable;

#if ENABLE(BYTECODE_COMMENTS)
        Vector<Comment> m_comments;
        const char *m_currentCommentString;
#endif

        ScopeNode* m_scopeNode;
        Strong<UnlinkedCodeBlock> m_codeBlock;

        // Some of these objects keep pointers to one another. They are arranged
        // to ensure a sane destruction order that avoids references to freed memory.
        HashSet<RefPtr<StringImpl>, IdentifierRepHash> m_functions;
        RegisterID m_ignoredResultRegister;
        RegisterID m_thisRegister;
        RegisterID m_calleeRegister;
        RegisterID* m_activationRegister;
        RegisterID* m_emptyValueRegister;
        SegmentedVector<RegisterID, 32> m_constantPoolRegisters;
        SegmentedVector<RegisterID, 32> m_calleeRegisters;
        SegmentedVector<RegisterID, 32> m_parameters;
        SegmentedVector<Label, 32> m_labels;
        SegmentedVector<LabelScope, 8> m_labelScopes;
        RefPtr<RegisterID> m_lastVar;
        int m_finallyDepth;
        int m_dynamicScopeDepth;
        CodeType m_codeType;

        Vector<ControlFlowContext> m_scopeContextStack;
        Vector<SwitchInfo> m_switchContextStack;
        Vector<ForInContext> m_forInContextStack;
        Vector<TryContext> m_tryContextStack;
        
        Vector<TryRange> m_tryRanges;
        SegmentedVector<TryData, 8> m_tryData;

        int m_firstConstantIndex;
        int m_nextConstantOffset;
        unsigned m_globalConstantIndex;

        int m_globalVarStorageOffset;

        bool m_hasCreatedActivation;
        int m_firstLazyFunction;
        int m_lastLazyFunction;
        HashMap<unsigned int, FunctionBodyNode*, WTF::IntHash<unsigned int>, WTF::UnsignedWithZeroKeyHashTraits<unsigned int> > m_lazyFunctions;
        typedef HashMap<FunctionBodyNode*, unsigned> FunctionOffsetMap;
        FunctionOffsetMap m_functionOffsets;
        
        // Constant pool
        IdentifierMap m_identifierMap;
        JSValueMap m_jsValueMap;
        NumberMap m_numberMap;
        IdentifierStringMap m_stringMap;

        uint32_t getResolveOperations(const Identifier& property)
        {
            if (m_dynamicScopeDepth)
                return m_codeBlock->addResolve();
            IdentifierResolveMap::AddResult result = m_resolveCacheMap.add(property.impl(), 0);
            if (result.isNewEntry)
                result.iterator->value = m_codeBlock->addResolve();
            return result.iterator->value;
        }

        uint32_t getResolveWithThisOperations(const Identifier& property)
        {
            if (m_dynamicScopeDepth)
                return m_codeBlock->addResolve();
            IdentifierResolveMap::AddResult result = m_resolveWithThisCacheMap.add(property.impl(), 0);
            if (result.isNewEntry)
                result.iterator->value = m_codeBlock->addResolve();
            return result.iterator->value;
        }

        uint32_t getResolveBaseOperations(IdentifierResolvePutMap& map, const Identifier& property, uint32_t& putToBaseOperation)
        {
            if (m_dynamicScopeDepth) {
                putToBaseOperation = m_codeBlock->addPutToBase();
                return m_codeBlock->addResolve();
            }
            ResolveCacheEntry entry = {-1, -1};
            IdentifierResolvePutMap::AddResult result = map.add(property.impl(), entry);
            if (result.isNewEntry)
                result.iterator->value.resolveOperations = m_codeBlock->addResolve();
            if (result.iterator->value.putOperations == -1)
                result.iterator->value.putOperations = getPutToBaseOperation(property);
            putToBaseOperation = result.iterator->value.putOperations;
            return result.iterator->value.resolveOperations;
        }

        uint32_t getResolveBaseOperations(const Identifier& property)
        {
            uint32_t scratch;
            return getResolveBaseOperations(m_resolveBaseMap, property, scratch);
        }

        uint32_t getResolveBaseForPutOperations(const Identifier& property, uint32_t& putToBaseOperation)
        {
            return getResolveBaseOperations(m_resolveBaseForPutMap, property, putToBaseOperation);
        }

        uint32_t getResolveWithBaseForPutOperations(const Identifier& property, uint32_t& putToBaseOperation)
        {
            return getResolveBaseOperations(m_resolveWithBaseForPutMap, property, putToBaseOperation);
        }

        uint32_t getPutToBaseOperation(const Identifier& property)
        {
            if (m_dynamicScopeDepth)
                return m_codeBlock->addPutToBase();
            IdentifierResolveMap::AddResult result = m_putToBaseMap.add(property.impl(), 0);
            if (result.isNewEntry)
                result.iterator->value = m_codeBlock->addPutToBase();
            return result.iterator->value;
        }

        IdentifierResolveMap m_putToBaseMap;
        IdentifierResolveMap m_resolveCacheMap;
        IdentifierResolveMap m_resolveWithThisCacheMap;
        IdentifierResolvePutMap m_resolveBaseMap;
        IdentifierResolvePutMap m_resolveBaseForPutMap;
        IdentifierResolvePutMap m_resolveWithBaseForPutMap;

        JSGlobalData* m_globalData;

        OpcodeID m_lastOpcodeID;
#ifndef NDEBUG
        size_t m_lastOpcodePosition;
#endif

        StackBounds m_stack;

        bool m_usesExceptions;
        bool m_expressionTooDeep;
    };

}

#endif // BytecodeGenerator_h
