/*
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
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

#include "config.h"
#include "Debugger.h"

#include "Error.h"
#include "HeapIterationScope.h"
#include "Interpreter.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "Operations.h"
#include "Parser.h"
#include "Protect.h"

namespace {

using namespace JSC;

class Recompiler : public MarkedBlock::VoidFunctor {
public:
    Recompiler(JSC::Debugger*);
    ~Recompiler();
    void operator()(JSCell*);

private:
    typedef HashSet<FunctionExecutable*> FunctionExecutableSet;
    typedef HashMap<SourceProvider*, ExecState*> SourceProviderMap;
    
    JSC::Debugger* m_debugger;
    FunctionExecutableSet m_functionExecutables;
    SourceProviderMap m_sourceProviders;
};

inline Recompiler::Recompiler(JSC::Debugger* debugger)
    : m_debugger(debugger)
{
}

inline Recompiler::~Recompiler()
{
    // Call sourceParsed() after reparsing all functions because it will execute
    // JavaScript in the inspector.
    SourceProviderMap::const_iterator end = m_sourceProviders.end();
    for (SourceProviderMap::const_iterator iter = m_sourceProviders.begin(); iter != end; ++iter)
        m_debugger->sourceParsed(iter->value, iter->key, -1, String());
}

inline void Recompiler::operator()(JSCell* cell)
{
    if (!cell->inherits(JSFunction::info()))
        return;

    JSFunction* function = jsCast<JSFunction*>(cell);
    if (function->executable()->isHostFunction())
        return;

    FunctionExecutable* executable = function->jsExecutable();

    // Check if the function is already in the set - if so,
    // we've already retranslated it, nothing to do here.
    if (!m_functionExecutables.add(executable).isNewEntry)
        return;

    ExecState* exec = function->scope()->globalObject()->JSGlobalObject::globalExec();
    executable->clearCodeIfNotCompiling();
    executable->clearUnlinkedCodeForRecompilationIfNotCompiling();
    if (m_debugger == function->scope()->globalObject()->debugger())
        m_sourceProviders.add(executable->source().provider(), exec);
}

} // namespace

namespace JSC {

Debugger::Debugger()
    : m_needsExceptionCallbacks(false)
    , m_needsOpDebugCallbacks(false)
    , m_shouldPause(false)
    , m_numberOfBreakpoints(0)
{
}

Debugger::~Debugger()
{
    HashSet<JSGlobalObject*>::iterator end = m_globalObjects.end();
    for (HashSet<JSGlobalObject*>::iterator it = m_globalObjects.begin(); it != end; ++it)
        (*it)->setDebugger(0);
}

void Debugger::attach(JSGlobalObject* globalObject)
{
    ASSERT(!globalObject->debugger());
    globalObject->setDebugger(this);
    m_globalObjects.add(globalObject);
}

void Debugger::detach(JSGlobalObject* globalObject)
{
    ASSERT(m_globalObjects.contains(globalObject));
    m_globalObjects.remove(globalObject);
    globalObject->setDebugger(0);
}

void Debugger::setNeedsExceptionCallbacks(bool value)
{
    m_needsExceptionCallbacks = value;
}

void Debugger::setShouldPause(bool value)
{
    m_shouldPause = value;
    updateNeedForOpDebugCallbacks();
}

void Debugger::recompileAllJSFunctions(VM* vm)
{
    // If JavaScript is running, it's not safe to recompile, since we'll end
    // up throwing away code that is live on the stack.
    ASSERT(!vm->dynamicGlobalObject);
    if (vm->dynamicGlobalObject)
        return;
    
    vm->prepareToDiscardCode();

    Recompiler recompiler(this);
    HeapIterationScope iterationScope(vm->heap);
    vm->heap.objectSpace().forEachLiveCell(iterationScope, recompiler);
}

void Debugger::updateNumberOfBreakpoints(int numberOfBreakpoints)
{
    ASSERT(numberOfBreakpoints >= 0);
    m_numberOfBreakpoints = numberOfBreakpoints;
    updateNeedForOpDebugCallbacks();
}

void Debugger::updateNeedForOpDebugCallbacks()
{
    m_needsOpDebugCallbacks = m_shouldPause || m_numberOfBreakpoints;
}

} // namespace JSC
