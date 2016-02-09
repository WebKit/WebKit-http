/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "JSModuleRecord.h"

#include "Error.h"
#include "Executable.h"
#include "IdentifierInlines.h"
#include "JSCJSValueInlines.h"
#include "JSCellInlines.h"
#include "JSMap.h"
#include "JSModuleEnvironment.h"
#include "JSModuleNamespaceObject.h"
#include "SlotVisitorInlines.h"
#include "StructureInlines.h"

namespace JSC {

const ClassInfo JSModuleRecord::s_info = { "ModuleRecord", &Base::s_info, 0, CREATE_METHOD_TABLE(JSModuleRecord) };

void JSModuleRecord::destroy(JSCell* cell)
{
    JSModuleRecord* thisObject = jsCast<JSModuleRecord*>(cell);
    thisObject->JSModuleRecord::~JSModuleRecord();
}

void JSModuleRecord::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    putDirect(vm, Identifier::fromString(&vm, ASCIILiteral("registryEntry")), jsUndefined());
    putDirect(vm, Identifier::fromString(&vm, ASCIILiteral("evaluated")), jsBoolean(false));

    m_dependenciesMap.set(vm, this, JSMap::create(vm, globalObject()->mapStructure()));
    putDirect(vm, Identifier::fromString(&vm, ASCIILiteral("dependenciesMap")), m_dependenciesMap.get());
}

void JSModuleRecord::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSModuleRecord* thisObject = jsCast<JSModuleRecord*>(cell);
    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_moduleEnvironment);
    visitor.append(&thisObject->m_moduleNamespaceObject);
    visitor.append(&thisObject->m_moduleProgramExecutable);
    visitor.append(&thisObject->m_dependenciesMap);
}

void JSModuleRecord::appendRequestedModule(const Identifier& moduleName)
{
    m_requestedModules.add(moduleName.impl());
}

void JSModuleRecord::addStarExportEntry(const Identifier& moduleName)
{
    m_starExportEntries.add(moduleName.impl());
}

void JSModuleRecord::addImportEntry(const ImportEntry& entry)
{
    bool isNewEntry = m_importEntries.add(entry.localName.impl(), entry).isNewEntry;
    ASSERT_UNUSED(isNewEntry, isNewEntry); // This is guaranteed by the parser.
}

void JSModuleRecord::addExportEntry(const ExportEntry& entry)
{
    bool isNewEntry = m_exportEntries.add(entry.exportName.impl(), entry).isNewEntry;
    ASSERT_UNUSED(isNewEntry, isNewEntry); // This is guaranteed by the parser.
}

auto JSModuleRecord::tryGetImportEntry(UniquedStringImpl* localName) -> Optional<ImportEntry>
{
    const auto iterator = m_importEntries.find(localName);
    if (iterator == m_importEntries.end())
        return Nullopt;
    return Optional<ImportEntry>(iterator->value);
}

auto JSModuleRecord::tryGetExportEntry(UniquedStringImpl* exportName) -> Optional<ExportEntry>
{
    const auto iterator = m_exportEntries.find(exportName);
    if (iterator == m_exportEntries.end())
        return Nullopt;
    return Optional<ExportEntry>(iterator->value);
}

auto JSModuleRecord::ExportEntry::createLocal(const Identifier& exportName, const Identifier& localName, const VariableEnvironmentEntry& variable) -> ExportEntry
{
    return ExportEntry { Type::Local, exportName, Identifier(), Identifier(), localName, variable };
}

auto JSModuleRecord::ExportEntry::createNamespace(const Identifier& exportName, const Identifier& moduleName) -> ExportEntry
{
    return ExportEntry { Type::Namespace, exportName, moduleName, Identifier(), Identifier(), VariableEnvironmentEntry() };
}

auto JSModuleRecord::ExportEntry::createIndirect(const Identifier& exportName, const Identifier& importName, const Identifier& moduleName) -> ExportEntry
{
    return ExportEntry { Type::Indirect, exportName, moduleName, importName, Identifier(), VariableEnvironmentEntry() };
}

auto JSModuleRecord::Resolution::notFound() -> Resolution
{
    return Resolution { Type::NotFound, nullptr, Identifier() };
}

auto JSModuleRecord::Resolution::error() -> Resolution
{
    return Resolution { Type::Error, nullptr, Identifier() };
}

auto JSModuleRecord::Resolution::ambiguous() -> Resolution
{
    return Resolution { Type::Ambiguous, nullptr, Identifier() };
}

static JSValue identifierToJSValue(ExecState* exec, const Identifier& identifier)
{
    if (identifier.isSymbol())
        return Symbol::create(exec->vm(), static_cast<SymbolImpl&>(*identifier.impl()));
    return jsString(&exec->vm(), identifier.impl());
}

JSModuleRecord* JSModuleRecord::hostResolveImportedModule(ExecState* exec, const Identifier& moduleName)
{
    JSValue moduleNameValue = identifierToJSValue(exec, moduleName);
    JSValue pair = m_dependenciesMap->JSMap::get(exec, moduleNameValue);
    return jsCast<JSModuleRecord*>(pair.get(exec, Identifier::fromString(exec, "value")));
}

auto JSModuleRecord::resolveImport(ExecState* exec, const Identifier& localName) -> Resolution
{
    Optional<ImportEntry> optionalImportEntry = tryGetImportEntry(localName.impl());
    if (!optionalImportEntry)
        return Resolution::notFound();

    const ImportEntry& importEntry = *optionalImportEntry;
    if (importEntry.isNamespace(exec->vm()))
        return Resolution::notFound();

    JSModuleRecord* importedModule = hostResolveImportedModule(exec, importEntry.moduleRequest);
    return importedModule->resolveExport(exec, importEntry.importName);
}

struct JSModuleRecord::ResolveQuery {
    struct Hash {
        static unsigned hash(const ResolveQuery&);
        static bool equal(const ResolveQuery&, const ResolveQuery&);
        static const bool safeToCompareToEmptyOrDeleted = true;
    };

    ResolveQuery(JSModuleRecord* moduleRecord, UniquedStringImpl* exportName)
        : moduleRecord(moduleRecord)
        , exportName(exportName)
    {
    }

    ResolveQuery(JSModuleRecord* moduleRecord, const Identifier& exportName)
        : ResolveQuery(moduleRecord, exportName.impl())
    {
    }

    enum EmptyValueTag { EmptyValue };
    ResolveQuery(EmptyValueTag)
    {
    }

    enum DeletedValueTag { DeletedValue };
    ResolveQuery(DeletedValueTag)
        : moduleRecord(nullptr)
        , exportName(WTF::HashTableDeletedValue)
    {
    }

    bool isEmptyValue() const
    {
        return !exportName;
    }

    bool isDeletedValue() const
    {
        return exportName.isHashTableDeletedValue();
    }

    // The module record is not marked from the GC. But these records are reachable from the JSGlobalObject.
    // So we don't care the reachability to this record.
    JSModuleRecord* moduleRecord;
    RefPtr<UniquedStringImpl> exportName;
};

inline unsigned JSModuleRecord::ResolveQuery::Hash::hash(const ResolveQuery& query)
{
    return WTF::PtrHash<JSModuleRecord*>::hash(query.moduleRecord) + IdentifierRepHash::hash(query.exportName);
}

inline bool JSModuleRecord::ResolveQuery::Hash::equal(const ResolveQuery& lhs, const ResolveQuery& rhs)
{
    return lhs.moduleRecord == rhs.moduleRecord && lhs.exportName == rhs.exportName;
}

auto JSModuleRecord::tryGetCachedResolution(UniquedStringImpl* exportName) -> Optional<Resolution>
{
    const auto iterator = m_resolutionCache.find(exportName);
    if (iterator == m_resolutionCache.end())
        return Nullopt;
    return Optional<Resolution>(iterator->value);
}

void JSModuleRecord::cacheResolution(UniquedStringImpl* exportName, const Resolution& resolution)
{
    m_resolutionCache.add(exportName, resolution);
}

auto JSModuleRecord::resolveExportImpl(ExecState* exec, const ResolveQuery& root) -> Resolution
{
    // http://www.ecma-international.org/ecma-262/6.0/#sec-resolveexport

    // How to avoid C++ recursion in this function:
    // This function avoids C++ recursion of the naive ResolveExport implementation.
    // Flatten the recursion to the loop with the task queue and frames.
    //
    // 1. pendingTasks
    //     We enqueue the recursive resolveExport call to this queue to avoid recursive calls in C++.
    //     The task has 3 types. (1) Query, (2) IndirectFallback and (3) GatherStars.
    //     (1) Query
    //         Querying the resolution to the current module.
    //     (2) IndirectFallback
    //         Examine the result of the indirect export resolution. Only when the indirect export resolution fails,
    //         we look into the star exports. (step 5-a-vi).
    //     (3) GatherStars
    //         Examine the result of the star export resolutions.
    //
    // 2. frames
    //     When the spec calls the resolveExport recursively, instead we append the frame
    //     (that holds the result resolution) to the frames and enqueue the task to the pendingTasks.
    //     The entry in the frames means the *local* resolution result of the specific recursive resolveExport.
    //
    // We should maintain the local resolution result instead of holding the global resolution result only.
    // For example,
    //
    //     star
    // (1) ---> (2) "Resolve"
    //      |
    //      |
    //      +-> (3) "NotFound"
    //      |
    //      |       star
    //      +-> (4) ---> (5) "Resolve" [here]
    //               |
    //               |
    //               +-> (6) "Error"
    //
    // Consider the above graph. The numbers represents the modules. Now we are [here].
    // If we only hold the global resolution result during the resolveExport operation, [here],
    // we decide the entire result of resolveExport is "Ambiguous", because there are multiple
    // "Reslove" (in module (2) and (5)). However, this should become "Error" because (6) will
    // propagate "Error" state to the (4), (4) will become "Error" and then, (1) will become
    // "Error". We should aggregate the results at the star exports point ((4) and (1)).
    //
    // Usually, both "Error" and "Ambiguous" states will throw the syntax error. So except for the content of the
    // error message, there are no difference. (And if we fix the (6) that raises "Error", next, it will produce
    // the "Ambiguous" error due to (5). Anyway, user need to fix the both. So which error should be raised at first
    // doesn't matter so much.
    //
    // However, this may become the problem under the module namespace creation.
    // http://www.ecma-international.org/ecma-262/6.0/#sec-getmodulenamespace
    // section 15.2.1.18, step 3-d-ii
    // Here, we distinguish "Ambiguous" and "Error". When "Error" state is produced, we need to throw the propagated error.
    // But if "Ambiguous" state comes, we just ignore the result.
    // To follow the requirement strictly, in this implementation, we keep the local resolution result to produce the
    // correct result under the above complex cases.

    // Caching strategy:
    // The resolveExport operation is frequently called. So caching results is important.
    // We observe the following aspects and based on them construct the caching strategy.
    // Here, we attempt to cache the resolution by constructing the map in module records.
    // That means  Module -> ExportName -> Maybe<Resolution>.
    // Technically, all the JSModuleRecords have the Map<ExportName, Resolution> for caching.
    //
    // The important observations are that,
    //
    //  - *cacheable* means that traversing to this node from a path will produce the same results as starting from this node.
    //
    //    Here, we define the resovling route. We represent [?] as the module that has the local binding.
    //    And (?) as the module without the local binding.
    //
    //      @ -> (A) -> (B) -> [C]
    //
    //    We list the resolving route for each node.
    //
    //    (A): (A) -> (B) -> [C]
    //    (B): (B) -> [C]
    //    [C]: [C]
    //
    //    In this case, if we start the tracing from (B), the resolving route becomes (B) -> [C].
    //    So this is the same. At that time, we can say (B) is cacheable in the first tracing.
    //
    //  - The cache ability of a node depends on the resolving route from this node.
    //
    // 1. The starting point is always cacheable.
    //
    // 2. A module that has resolved a local binding is always cacheable.
    //
    //  @ -> (A) -> [B]
    //
    //  In the above case, we can see the [B] as cacheable.
    //  This is because when starting from [B] node, we immediately resolve with the local binding.
    //  So the resolving route from [B] does not depend on the starting point.
    //
    // 3. If we don't follow any star links during the resolution, we can see all the traced nodes are cacheable.
    //
    //  If there are non star links, it means that there is *no branch* in the module dependency graph.
    //  This *no branch* feature makes all the modules cachable.
    //
    //  I.e, if we traverse one star link (even if we successfully resolve that star link),
    //  we must still traverse all other star links. I would also explain we don't run into
    //  this when resolving a local/indirect link. When resolving a local/indirect link,
    //  we won't traverse any star links.
    //  And since the module can hold only one local/indirect link for the specific export name (if there
    //  are multiple local/indirect links that has the same export name, it should be syntax error in the
    //  parsing phase.), there is no multiple outgoing links from a module.
    //
    //  @ -> (A) --> (B) -> [C] -> (D) -> (E) -+
    //                ^                        |
    //                |                        |
    //                +------------------------+
    //
    //  When starting from @, [C] will be found as the module resolving the given binding.
    //  In this case, (B) can cache this resolution. Since the resolving route is the same to the one when
    //  starting from (B). After caching the above result, we attempt to resolve the same binding from (D).
    //
    //                              @
    //                              |
    //                              v
    //  @ -> (A) --> (B) -> [C] -> (D) -> (E) -+
    //                ^                        |
    //                |                        |
    //                +------------------------+
    //
    //  In this case, we can use the (B)'s cached result. And (E) can be cached.
    //
    //    (E): The resolving route is now (E) -> (B) -> [C]. That is the same when starting from (E).
    //
    //  No branching makes that the problematic *once-visited* node cannot be seen.
    //  The *once-visited* node makes the resolving route changed since when we see the *once-visited* node,
    //  we stop tracing this.
    //
    //  If there is no star links and if we look *once-visited* node under no branching graph, *once-visited*
    //  node cannot resolve the requested binding. If the *once-visited* node can resolve the binding, we
    //  should have already finished the resolution before reaching this *once-visited* node.
    //
    // 4. Once we follow star links, we should not retrieve the result from the cache and should not cache.
    //
    //  Star links are only the way to introduce branch.
    //  Once we follow the star links during the resolution, we cannot cache naively.
    //  This is because the cacheability depends on the resolving route. And branching produces the problematic *once-visited*
    //  nodes. Since we don't follow the *once-visited* node, the resolving route from the node becomes different from
    //  the resolving route when starting from this node.
    //
    //  The following example explains when we should not retrieve the cache and cache the result.
    //
    //               +----> (D) ------+
    //               |                |
    //               |                v
    //      (A) *----+----> (B) ---> [C]
    //                       ^
    //                       |
    //                       @
    //
    //  When starting from (B), we find [C]. In this resolving route, we don't find any star link.
    //  And by definition, (B) and [C] are cachable. (B) is the starting point. And [C] has the local binding.
    //
    //               +----> (D) ------+
    //               |                |
    //               |                v
    //  @-> (A) *----+----> (B) ---> [C]
    //
    //  But when starting from (A), we should not get the value from the cache. Because,
    //
    //    1. When looking (D), we reach [C] and make both resolved.
    //    2. When looking (B), if we retrieved the last cache from (B), (B) becomes resolved.
    //    3. But actually, (B) is not-found in this trial because (C) is already *once-visited*.
    //    4. If we accidentally make (B) resolved, (A) becomes ambiguous. But the correct answer is resolved.
    //
    //  Why is this problem caused? This is because the *once-visited* node makes the result not-found.
    //  In the second trial, (B) -> [C] result is changed from resolved to not-found.
    //
    //  When does this become a problem? If the status of the *once-visited* node group is resolved,
    //  changing the result to not-found makes the result changed.
    //
    //  This problem does not happen when we don't see any star link yet. Now, consider the minimum case.
    //
    //  @-> (A) -> [ some graph ]
    //       ^            |
    //       |            |
    //       +------------+
    //
    //  In (A), we don't see any star link yet. So we can say that all the visited nodes does not have any local
    //  resolution. Because if they had a local/indirect resolution, we should have already finished the tracing.
    //
    //  And even if the some graph will see the *once-visited* node (in this case, (A)), that does not affect the
    //  result of the resolution. Because even if we follow the link to (A) or not follow the link to (A), the status
    //  of the link is always not-found since (A) does not have any local resolution.
    //  In the above case, we can use the result of the [some graph].
    //
    // 5. Once we see star links, even if we have not yet traversed that star link path, we should disable caching.
    //
    //  Here is the reason why:
    //
    //       +-------------+
    //       |             |
    //       v             |
    //      (A) -> (B) -> (C) *-> [E]
    //       *             ^
    //       |             |
    //       v             @
    //      [D]
    //
    //  In the above case, (C) will be resolved with [D].
    //  (C) will see (A) and (A) gives up in (A) -> (B) -> (C) route. So, (A) will fallback to [D].
    //
    //       +-------------+
    //       |             |
    //       v             |
    //  @-> (A) -> (B) -> (C) *-> [E]
    //       *
    //       |
    //       v
    //      [D]
    //
    //  But in this case, (A) will be resolved with [E] (not [D]).
    //  (C) will attempt to follow the link to (A), but it fails.
    //  So (C) will fallback to the star link and found [E]. In this senario,
    //  (C) is now resolved with [E]'s result.
    //
    //  The cause of this problem is also the same to 4.
    //  In the latter case, when looking (C), we cannot use the cached result in (C).
    //  Because the cached result of (C) depends on the *once-visited* node (A) and
    //  (A) has the fallback system with the star link.
    //  In the latter trial, we now assume that (A)'s status is not-found.
    //  But, actually, in the former trial, (A)'s status becomes resolved due to the fallback to the [D].
    //
    // To summarize the observations.
    //
    //  1. The starting point is always cacheable.
    //  2. A module that has resolved a local binding is always cacheable.
    //  3. If we don't follow any star links during the resolution, we can see all the traced nodes are cacheable.
    //  4. Once we follow star links, we should not retrieve the result from the cache and should not cache the result.
    //  5. Once we see star links, even if we have not yet traversed that star link path, we should disable caching.

    typedef WTF::HashSet<ResolveQuery, ResolveQuery::Hash, WTF::CustomHashTraits<ResolveQuery>> ResolveSet;
    enum class Type { Query, IndirectFallback, GatherStars };
    struct Task {
        ResolveQuery query;
        Type type;
    };

    Vector<Task, 8> pendingTasks;
    ResolveSet resolveSet;
    HashSet<JSModuleRecord*> starSet;

    Vector<Resolution, 8> frames;

    bool foundStarLinks = false;

    frames.append(Resolution::notFound());

    // Call when the query is not resolved in the current module.
    // It will enqueue the star resolution requests. Return "false" if the error occurs.
    auto resolveNonLocal = [&](const ResolveQuery& query) -> bool {
        // http://www.ecma-international.org/ecma-262/6.0/#sec-resolveexport
        // section 15.2.1.16.3, step 6
        // If the "default" name is not resolved in the current module, we need to throw an error and stop resolution immediately,
        // Rationale to this error: A default export cannot be provided by an export *.
        if (query.exportName == exec->propertyNames().defaultKeyword.impl())
            return false;

        // step 7, If exportStarSet contains module, then return null.
        if (!starSet.add(query.moduleRecord).isNewEntry)
            return true;

        // Enqueue the task to gather the results of the stars.
        // And append the new Resolution frame to gather the local result of the stars.
        pendingTasks.append(Task { query, Type::GatherStars });
        foundStarLinks = true;
        frames.append(Resolution::notFound());


        // Enqueue the tasks in reverse order.
        for (auto iterator = query.moduleRecord->starExportEntries().rbegin(), end = query.moduleRecord->starExportEntries().rend(); iterator != end; ++iterator) {
            const RefPtr<UniquedStringImpl>& starModuleName = *iterator;
            JSModuleRecord* importedModuleRecord = query.moduleRecord->hostResolveImportedModule(exec, Identifier::fromUid(exec, starModuleName.get()));
            pendingTasks.append(Task { ResolveQuery(importedModuleRecord, query.exportName.get()), Type::Query });
        }
        return true;
    };

    // Return the current resolution value of the top frame.
    auto currentTop = [&] () -> Resolution& {
        ASSERT(!frames.isEmpty());
        return frames.last();
    };

    // Merge the given resolution to the current resolution value of the top frame.
    // If there is ambiguity, return "false". When the "false" is returned, we should make the result "ambiguous".
    auto mergeToCurrentTop = [&] (const Resolution& resolution) -> bool {
        if (resolution.type == Resolution::Type::NotFound)
            return true;

        if (currentTop().type == Resolution::Type::NotFound) {
            currentTop() = resolution;
            return true;
        }

        if (currentTop().moduleRecord != resolution.moduleRecord || currentTop().localName != resolution.localName)
            return false;

        return true;
    };

    auto cacheResolutionForQuery = [] (const ResolveQuery& query, const Resolution& resolution) {
        ASSERT(resolution.type == Resolution::Type::Resolved);
        query.moduleRecord->cacheResolution(query.exportName.get(), resolution);
    };

    pendingTasks.append(Task { root, Type::Query });
    while (!pendingTasks.isEmpty()) {
        const Task task = pendingTasks.takeLast();
        const ResolveQuery& query = task.query;

        switch (task.type) {
        case Type::Query: {
            JSModuleRecord* moduleRecord = query.moduleRecord;

            if (!resolveSet.add(task.query).isNewEntry)
                continue;

            //  5. Once we see star links, even if we have not yet traversed that star link path, we should disable caching.
            if (!moduleRecord->starExportEntries().isEmpty())
                foundStarLinks = true;

            //  4. Once we follow star links, we should not retrieve the result from the cache and should not cache the result.
            if (!foundStarLinks) {
                if (Optional<Resolution> cachedResolution = moduleRecord->tryGetCachedResolution(query.exportName.get())) {
                    if (!mergeToCurrentTop(*cachedResolution))
                        return Resolution::ambiguous();
                    continue;
                }
            }

            const Optional<ExportEntry> optionalExportEntry = moduleRecord->tryGetExportEntry(query.exportName.get());
            if (!optionalExportEntry) {
                // If there is no matched exported binding in the current module,
                // we need to look into the stars.
                if (!resolveNonLocal(task.query))
                    return Resolution::error();
                continue;
            }

            const ExportEntry& exportEntry = *optionalExportEntry;
            switch (exportEntry.type) {
            case ExportEntry::Type::Local:
            case ExportEntry::Type::Namespace: {
                Resolution resolution { Resolution::Type::Resolved, moduleRecord, exportEntry.localName };
                //  2. A module that has resolved a local binding is always cacheable.
                cacheResolutionForQuery(query, resolution);
                if (!mergeToCurrentTop(resolution))
                    return Resolution::ambiguous();
                continue;
            }

            case ExportEntry::Type::Indirect: {
                JSModuleRecord* importedModuleRecord = moduleRecord->hostResolveImportedModule(exec, exportEntry.moduleName);

                // When the imported module does not produce any resolved binding, we need to look into the stars in the *current*
                // module. To do this, we append the `IndirectFallback` task to the task queue.
                pendingTasks.append(Task { query, Type::IndirectFallback });
                // And append the new Resolution frame to check the indirect export will be resolved or not.
                frames.append(Resolution::notFound());
                pendingTasks.append(Task { ResolveQuery(importedModuleRecord, exportEntry.importName), Type::Query });
                continue;
            }
            }
            break;
        }

        case Type::IndirectFallback: {
            Resolution resolution = frames.takeLast();

            if (resolution.type == Resolution::Type::NotFound) {
                // Indirect export entry does not produce any resolved binding.
                // So we will investigate the stars.
                if (!resolveNonLocal(task.query))
                    return Resolution::error();
                continue;
            }

            ASSERT_WITH_MESSAGE(resolution.type == Resolution::Type::Resolved, "When we see Error and Ambiguous, we immediately return from this loop. So here, only Resolved comes.");

            //  3. If we don't follow any star links during the resolution, we can see all the traced nodes are cacheable.
            //  4. Once we follow star links, we should not retrieve the result from the cache and should not cache the result.
            if (!foundStarLinks)
                cacheResolutionForQuery(query, resolution);

            // If indirect export entry produces Resolved, we should merge it to the upper frame.
            // And do not investigate the stars of the current module.
            if (!mergeToCurrentTop(resolution))
                return Resolution::ambiguous();
            break;
        }

        case Type::GatherStars: {
            Resolution resolution = frames.takeLast();
            ASSERT_WITH_MESSAGE(resolution.type == Resolution::Type::Resolved || resolution.type == Resolution::Type::NotFound, "When we see Error and Ambiguous, we immediately return from this loop. So here, only Resolved and NotFound comes.");

            // Merge the star resolution to the upper frame.
            if (!mergeToCurrentTop(resolution))
                return Resolution::ambiguous();
            break;
        }
        }
    }

    ASSERT(frames.size() == 1);
    //  1. The starting point is always cacheable.
    if (frames[0].type == Resolution::Type::Resolved)
        cacheResolutionForQuery(root, frames[0]);
    return frames[0];
}

auto JSModuleRecord::resolveExport(ExecState* exec, const Identifier& exportName) -> Resolution
{
    // Look up the cached resolution first before entering the resolving loop, since the loop setup takes some cost.
    if (Optional<Resolution> cachedResolution = tryGetCachedResolution(exportName.impl()))
        return *cachedResolution;
    return resolveExportImpl(exec, ResolveQuery(this, exportName.impl()));
}

static void getExportedNames(ExecState* exec, JSModuleRecord* root, IdentifierSet& exportedNames)
{
    HashSet<JSModuleRecord*> exportStarSet;
    Vector<JSModuleRecord*, 8> pendingModules;

    pendingModules.append(root);

    while (!pendingModules.isEmpty()) {
        JSModuleRecord* moduleRecord = pendingModules.takeLast();
        if (exportStarSet.contains(moduleRecord))
            continue;
        exportStarSet.add(moduleRecord);

        for (const auto& pair : moduleRecord->exportEntries()) {
            const JSModuleRecord::ExportEntry& exportEntry = pair.value;
            switch (exportEntry.type) {
            case JSModuleRecord::ExportEntry::Type::Local:
            case JSModuleRecord::ExportEntry::Type::Indirect:
                if (moduleRecord == root || exec->propertyNames().defaultKeyword != exportEntry.exportName)
                    exportedNames.add(exportEntry.exportName.impl());
                break;

            case JSModuleRecord::ExportEntry::Type::Namespace:
                break;
            }
        }

        for (const auto& starModuleName : moduleRecord->starExportEntries()) {
            JSModuleRecord* requestedModuleRecord = moduleRecord->hostResolveImportedModule(exec, Identifier::fromUid(exec, starModuleName.get()));
            pendingModules.append(requestedModuleRecord);
        }
    }
}

JSModuleNamespaceObject* JSModuleRecord::getModuleNamespace(ExecState* exec)
{
    // http://www.ecma-international.org/ecma-262/6.0/#sec-getmodulenamespace
    if (m_moduleNamespaceObject)
        return m_moduleNamespaceObject.get();

    JSGlobalObject* globalObject = exec->lexicalGlobalObject();
    IdentifierSet exportedNames;
    getExportedNames(exec, this, exportedNames);

    IdentifierSet unambiguousNames;
    for (auto& name : exportedNames) {
        const JSModuleRecord::Resolution resolution = resolveExport(exec, Identifier::fromUid(exec, name.get()));
        switch (resolution.type) {
        case Resolution::Type::NotFound:
            throwSyntaxError(exec, makeString("Exported binding name '", String(name.get()), "' is not found."));
            return nullptr;

        case Resolution::Type::Error:
            throwSyntaxError(exec, makeString("Exported binding name 'default' cannot be resolved by star export entries."));
            return nullptr;

        case Resolution::Type::Ambiguous:
            break;

        case Resolution::Type::Resolved:
            unambiguousNames.add(name);
            break;
        }
    }

    m_moduleNamespaceObject.set(exec->vm(), this, JSModuleNamespaceObject::create(exec, globalObject, globalObject->moduleNamespaceObjectStructure(), this, unambiguousNames));
    return m_moduleNamespaceObject.get();
}

void JSModuleRecord::link(ExecState* exec)
{
    ModuleProgramExecutable* executable = ModuleProgramExecutable::create(exec, sourceCode());
    if (!executable) {
        throwSyntaxError(exec);
        return;
    }
    m_moduleProgramExecutable.set(exec->vm(), this, executable);
    instantiateDeclarations(exec, executable);
}

void JSModuleRecord::instantiateDeclarations(ExecState* exec, ModuleProgramExecutable* moduleProgramExecutable)
{
    // http://www.ecma-international.org/ecma-262/6.0/#sec-moduledeclarationinstantiation

    SymbolTable* symbolTable = moduleProgramExecutable->moduleEnvironmentSymbolTable();
    JSModuleEnvironment* moduleEnvironment = JSModuleEnvironment::create(exec->vm(), exec->lexicalGlobalObject(), exec->lexicalGlobalObject(), symbolTable, jsTDZValue(), this);

    VM& vm = exec->vm();

    // http://www.ecma-international.org/ecma-262/6.0/#sec-moduledeclarationinstantiation
    // section 15.2.1.16.4 step 9.
    // Ensure all the indirect exports are correctly resolved to unique bindings.
    // Even if we avoided duplicate exports in the parser, still ambiguous exports occur due to the star export (`export * from "mod"`).
    // When we see this type of ambiguity for the indirect exports here, throw a syntax error.
    for (const auto& pair : m_exportEntries) {
        const ExportEntry& exportEntry = pair.value;
        if (exportEntry.type == JSModuleRecord::ExportEntry::Type::Indirect) {
            Resolution resolution = resolveExport(exec, exportEntry.exportName);
            switch (resolution.type) {
            case Resolution::Type::NotFound:
                throwSyntaxError(exec, makeString("Indirectly exported binding name '", String(exportEntry.exportName.impl()), "' is not found."));
                return;

            case Resolution::Type::Ambiguous:
                throwSyntaxError(exec, makeString("Indirectly exported binding name '", String(exportEntry.exportName.impl()), "' cannot be resolved due to ambiguous multiple bindings."));
                return;

            case Resolution::Type::Error:
                throwSyntaxError(exec, makeString("Indirectly exported binding name 'default' cannot be resolved by star export entries."));
                return;

            case Resolution::Type::Resolved:
                break;
            }
        }
    }

    // http://www.ecma-international.org/ecma-262/6.0/#sec-moduledeclarationinstantiation
    // section 15.2.1.16.4 step 12.
    // Instantiate namespace objects and initialize the bindings with them if required.
    // And ensure that all the imports correctly resolved to unique bindings.
    for (const auto& pair : m_importEntries) {
        const ImportEntry& importEntry = pair.value;
        JSModuleRecord* importedModule = hostResolveImportedModule(exec, importEntry.moduleRequest);
        if (importEntry.isNamespace(vm)) {
            JSModuleNamespaceObject* namespaceObject = importedModule->getModuleNamespace(exec);
            if (exec->hadException())
                return;
            symbolTablePutTouchWatchpointSet(moduleEnvironment, exec, importEntry.localName, namespaceObject, /* shouldThrowReadOnlyError */ false, /* ignoreReadOnlyErrors */ true);
        } else {
            Resolution resolution = importedModule->resolveExport(exec, importEntry.importName);
            switch (resolution.type) {
            case Resolution::Type::NotFound:
                throwSyntaxError(exec, makeString("Importing binding name '", String(importEntry.importName.impl()), "' is not found."));
                return;

            case Resolution::Type::Ambiguous:
                throwSyntaxError(exec, makeString("Importing binding name '", String(importEntry.importName.impl()), "' cannot be resolved due to ambiguous multiple bindings."));
                return;

            case Resolution::Type::Error:
                throwSyntaxError(exec, makeString("Importing binding name 'default' cannot be resolved by star export entries."));
                return;

            case Resolution::Type::Resolved:
                break;
            }
        }
    }

    // http://www.ecma-international.org/ecma-262/6.0/#sec-moduledeclarationinstantiation
    // section 15.2.1.16.4 step 14.
    // Module environment contains the heap allocated "var", "function", "let", "const", and "class".
    // When creating the environment, we initialized all the slots with empty, it's ok for lexical values.
    // But for "var" and "function", we should initialize it with undefined. They are contained in the declared variables.
    for (const auto& variable : m_declaredVariables) {
        SymbolTableEntry entry = symbolTable->get(variable.key.get());
        VarOffset offset = entry.varOffset();
        if (!offset.isStack())
            symbolTablePutTouchWatchpointSet(moduleEnvironment, exec, Identifier::fromUid(exec, variable.key.get()), jsUndefined(), /* shouldThrowReadOnlyError */ false, /* ignoreReadOnlyErrors */ true);
    }

    // http://www.ecma-international.org/ecma-262/6.0/#sec-moduledeclarationinstantiation
    // section 15.2.1.16.4 step 16-a-iv.
    // Initialize heap allocated function declarations.
    // They can be called before the body of the module is executed under circular dependencies.
    UnlinkedModuleProgramCodeBlock* unlinkedCodeBlock = moduleProgramExecutable->unlinkedModuleProgramCodeBlock();
    for (size_t i = 0, numberOfFunctions = unlinkedCodeBlock->numberOfFunctionDecls(); i < numberOfFunctions; ++i) {
        UnlinkedFunctionExecutable* unlinkedFunctionExecutable = unlinkedCodeBlock->functionDecl(i);
        SymbolTableEntry entry = symbolTable->get(unlinkedFunctionExecutable->name().impl());
        VarOffset offset = entry.varOffset();
        if (!offset.isStack()) {
            ASSERT(!unlinkedFunctionExecutable->name().isEmpty());
            if (vm.typeProfiler() || vm.controlFlowProfiler()) {
                vm.functionHasExecutedCache()->insertUnexecutedRange(moduleProgramExecutable->sourceID(),
                    unlinkedFunctionExecutable->typeProfilingStartOffset(),
                    unlinkedFunctionExecutable->typeProfilingEndOffset());
            }
            JSFunction* function = JSFunction::create(vm, unlinkedFunctionExecutable->link(vm, moduleProgramExecutable->source()), moduleEnvironment);
            symbolTablePutTouchWatchpointSet(moduleEnvironment, exec, unlinkedFunctionExecutable->name(), function, /* shouldThrowReadOnlyError */ false, /* ignoreReadOnlyErrors */ true);
        }
    }

    setModuleEnvironment(vm, moduleEnvironment);
}

void JSModuleRecord::setModuleEnvironment(VM& vm, JSModuleEnvironment* moduleEnvironment) {
    m_moduleEnvironment.set(vm, this, moduleEnvironment);
}

JSValue JSModuleRecord::evaluate(ExecState* exec)
{
    if (!m_moduleProgramExecutable)
        return jsUndefined();
    JSValue result = exec->interpreter()->execute(m_moduleProgramExecutable.get(), exec, m_moduleEnvironment.get());
    m_moduleProgramExecutable.clear();
    return result;
}

static String printableName(const RefPtr<UniquedStringImpl>& uid)
{
    if (uid->isSymbol())
        return uid.get();
    return WTF::makeString("'", String(uid.get()), "'");
}

static String printableName(const Identifier& ident)
{
    return printableName(ident.impl());
}

void JSModuleRecord::dump()
{
    dataLog("\nAnalyzing ModuleRecord key(", printableName(m_moduleKey), ")\n");

    dataLog("    Dependencies: ", m_requestedModules.size(), " modules\n");
    for (const auto& moduleName : m_requestedModules)
        dataLog("      module(", printableName(moduleName), ")\n");

    dataLog("    Import: ", m_importEntries.size(), " entries\n");
    for (const auto& pair : m_importEntries) {
        const ImportEntry& importEntry = pair.value;
        dataLog("      import(", printableName(importEntry.importName), "), local(", printableName(importEntry.localName), "), module(", printableName(importEntry.moduleRequest), ")\n");
    }

    dataLog("    Export: ", m_exportEntries.size(), " entries\n");
    for (const auto& pair : m_exportEntries) {
        const ExportEntry& exportEntry = pair.value;
        switch (exportEntry.type) {
        case ExportEntry::Type::Local:
            dataLog("      [Local] ", "export(", printableName(exportEntry.exportName), "), local(", printableName(exportEntry.localName), ")\n");
            break;

        case ExportEntry::Type::Namespace:
            dataLog("      [Namespace] ", "export(", printableName(exportEntry.exportName), "), module(", printableName(exportEntry.moduleName), ")\n");
            break;

        case ExportEntry::Type::Indirect:
            dataLog("      [Indirect] ", "export(", printableName(exportEntry.exportName), "), import(", printableName(exportEntry.importName), "), module(", printableName(exportEntry.moduleName), ")\n");
            break;
        }
    }
    for (const auto& moduleName : m_starExportEntries)
        dataLog("      [Star] module(", printableName(moduleName.get()), ")\n");
}

} // namespace JSC
