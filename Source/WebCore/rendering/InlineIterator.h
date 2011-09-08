/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2006, 2007, 2008, 2009, 2010 Apple Inc. All right reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef InlineIterator_h
#define InlineIterator_h

#include "BidiRun.h"
#include "RenderBlock.h"
#include "RenderText.h"
#include <wtf/AlwaysInline.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

// This class is used to RenderInline subtrees, stepping by character within the
// text children. InlineIterator will use bidiNext to find the next RenderText
// optionally notifying a BidiResolver every time it steps into/out of a RenderInline.
class InlineIterator {
public:
    InlineIterator()
        : m_root(0)
        , m_obj(0)
        , m_pos(0)
        , m_nextBreakablePosition(-1)
    {
    }

    InlineIterator(RenderObject* root, RenderObject* o, unsigned p)
        : m_root(root)
        , m_obj(o)
        , m_pos(p)
        , m_nextBreakablePosition(-1)
    {
    }

    void clear() { moveTo(0, 0); }

    void moveToStartOf(RenderObject* object)
    {
        moveTo(object, 0);
    }

    void moveTo(RenderObject* object, unsigned offset, int nextBreak = -1)
    {
        m_obj = object;
        m_pos = offset;
        m_nextBreakablePosition = nextBreak;
    }

    RenderObject* object() const { return m_obj; }
    unsigned offset() const { return m_pos; }
    RenderObject* root() const { return m_root; }

    void fastIncrementInTextNode();
    void increment(InlineBidiResolver* = 0);
    bool atEnd() const;

    inline bool atTextParagraphSeparator()
    {
        return m_obj && m_obj->preservesNewline() && m_obj->isText() && toRenderText(m_obj)->textLength()
            && !toRenderText(m_obj)->isWordBreak() && toRenderText(m_obj)->characters()[m_pos] == '\n';
    }
    
    inline bool atParagraphSeparator()
    {
        return (m_obj && m_obj->isBR()) || atTextParagraphSeparator();
    }

    UChar current() const;
    UChar previousInSameNode() const;
    ALWAYS_INLINE WTF::Unicode::Direction direction() const;

private:
    RenderObject* m_root;

    // FIXME: These should be private.
public:
    RenderObject* m_obj;
    unsigned m_pos;
    int m_nextBreakablePosition;
};

inline bool operator==(const InlineIterator& it1, const InlineIterator& it2)
{
    return it1.m_pos == it2.m_pos && it1.m_obj == it2.m_obj;
}

inline bool operator!=(const InlineIterator& it1, const InlineIterator& it2)
{
    return it1.m_pos != it2.m_pos || it1.m_obj != it2.m_obj;
}

static inline WTF::Unicode::Direction embedCharFromDirection(TextDirection dir, EUnicodeBidi unicodeBidi)
{
    using namespace WTF::Unicode;
    if (unicodeBidi == Embed)
        return dir == RTL ? RightToLeftEmbedding : LeftToRightEmbedding;
    return dir == RTL ? RightToLeftOverride : LeftToRightOverride;
}

template <class Observer>
static inline void notifyObserverEnteredObject(Observer* observer, RenderObject* object)
{
    if (!observer || !object || !object->isRenderInline())
        return;

    RenderStyle* style = object->style();
    EUnicodeBidi unicodeBidi = style->unicodeBidi();
    if (unicodeBidi == UBNormal) {
        // http://dev.w3.org/csswg/css3-writing-modes/#unicode-bidi
        // "The element does not open an additional level of embedding with respect to the bidirectional algorithm."
        // Thus we ignore any possible dir= attribute on the span.
        return;
    }
    if (unicodeBidi == Isolate) {
        observer->enterIsolate();
        // Embedding/Override characters implied by dir= are handled when
        // we process the isolated span, not when laying out the "parent" run.
        return;
    }

    // FIXME: Should unicode-bidi: plaintext really be embedding override/embed characters here?
    observer->embed(embedCharFromDirection(style->direction(), unicodeBidi), FromStyleOrDOM);
}

template <class Observer>
static inline void notifyObserverWillExitObject(Observer* observer, RenderObject* object)
{
    if (!observer || !object || !object->isRenderInline())
        return;

    EUnicodeBidi unicodeBidi = object->style()->unicodeBidi();
    if (unicodeBidi == UBNormal)
        return; // Nothing to do for unicode-bidi: normal
    if (unicodeBidi == Isolate) {
        observer->exitIsolate();
        return;
    }

    // Otherwise we pop any embed/override character we added when we opened this tag.
    observer->embed(WTF::Unicode::PopDirectionalFormat, FromStyleOrDOM);
}

static inline bool isIteratorTarget(RenderObject* object)
{
    ASSERT(object); // The iterator will of course return 0, but its not an expected argument to this function.
    return object->isText() || object->isFloating() || object->isPositioned() || object->isReplaced();
}

// This enum is only used for bidiNextShared()
enum EmptyInlineBehavior {
    SkipEmptyInlines,
    IncludeEmptyInlines,
};

// FIXME: This function is misleadingly named. It has little to do with bidi.
// This function will iterate over inlines within a block, optionally notifying
// a bidi resolver as it enters/exits inlines (so it can push/pop embedding levels).
template <class Observer>
static inline RenderObject* bidiNextShared(RenderObject* root, RenderObject* current, Observer* observer = 0, EmptyInlineBehavior emptyInlineBehavior = SkipEmptyInlines, bool* endOfInlinePtr = 0)
{
    RenderObject* next = 0;
    // oldEndOfInline denotes if when we last stopped iterating if we were at the end of an inline.
    bool oldEndOfInline = endOfInlinePtr ? *endOfInlinePtr : false;
    bool endOfInline = false;

    while (current) {
        next = 0;
        if (!oldEndOfInline && !isIteratorTarget(current)) {
            next = current->firstChild();
            notifyObserverEnteredObject(observer, next);
        }

        // We hit this when either current has no children, or when current is not a renderer we care about.
        if (!next) {
            // If it is a renderer we care about, and we're doing our inline-walk, return it.
            if (emptyInlineBehavior == IncludeEmptyInlines && !oldEndOfInline && current->isRenderInline()) {
                next = current;
                endOfInline = true;
                break;
            }

            while (current && current != root) {
                notifyObserverWillExitObject(observer, current);

                next = current->nextSibling();
                if (next) {
                    notifyObserverEnteredObject(observer, next);
                    break;
                }

                current = current->parent();
                if (emptyInlineBehavior == IncludeEmptyInlines && current && current != root && current->isRenderInline()) {
                    next = current;
                    endOfInline = true;
                    break;
                }
            }
        }

        if (!next)
            break;

        if (isIteratorTarget(next)
            || ((emptyInlineBehavior == IncludeEmptyInlines || !next->firstChild()) // Always return EMPTY inlines.
                && next->isRenderInline()))
            break;
        current = next;
    }

    if (endOfInlinePtr)
        *endOfInlinePtr = endOfInline;

    return next;
}

template <class Observer>
static inline RenderObject* bidiNextSkippingEmptyInlines(RenderObject* root, RenderObject* current, Observer* observer)
{
    // The SkipEmptyInlines callers never care about endOfInlinePtr.
    return bidiNextShared(root, current, observer, SkipEmptyInlines);
}

// This makes callers cleaner as they don't have to specify a type for the observer when not providing one.
static inline RenderObject* bidiNextSkippingEmptyInlines(RenderObject* root, RenderObject* current)
{
    InlineBidiResolver* observer = 0;
    return bidiNextSkippingEmptyInlines(root, current, observer);
}

static inline RenderObject* bidiNextIncludingEmptyInlines(RenderObject* root, RenderObject* current, bool* endOfInlinePtr = 0)
{
    InlineBidiResolver* observer = 0; // Callers who include empty inlines, never use an observer.
    return bidiNextShared(root, current, observer, IncludeEmptyInlines, endOfInlinePtr);
}

static inline RenderObject* bidiFirstSkippingEmptyInlines(RenderObject* root, InlineBidiResolver* resolver)
{
    ASSERT(resolver);
    RenderObject* o = root->firstChild();
    if (!o)
        return 0;

    if (o->isRenderInline()) {
        notifyObserverEnteredObject(resolver, o);
        if (o->firstChild())
            o = bidiNextSkippingEmptyInlines(root, o, resolver);
        else {
            // Never skip empty inlines.
            if (resolver)
                resolver->commitExplicitEmbedding();
            return o; 
        }
    }

    // FIXME: Unify this with the bidiNext call above.
    if (o && !isIteratorTarget(o))
        o = bidiNextSkippingEmptyInlines(root, o, resolver);

    resolver->commitExplicitEmbedding();
    return o;
}

// FIXME: This method needs to be renamed when bidiNext finds a good name.
static inline RenderObject* bidiFirstIncludingEmptyInlines(RenderObject* root)
{
    RenderObject* o = root->firstChild();
    // If either there are no children to walk, or the first one is correct
    // then just return it.
    if (!o || o->isRenderInline() || isIteratorTarget(o))
        return o;

    return bidiNextIncludingEmptyInlines(root, o);
}

inline void InlineIterator::fastIncrementInTextNode()
{
    ASSERT(m_obj);
    ASSERT(m_obj->isText());
    ASSERT(m_pos <= toRenderText(m_obj)->textLength());
    m_pos++;
}

// FIXME: This is used by RenderBlock for simplified layout, and has nothing to do with bidi
// it shouldn't use functions called bidiFirst and bidiNext.
class InlineWalker {
public:
    InlineWalker(RenderObject* root)
        : m_root(root)
        , m_current(0)
        , m_atEndOfInline(false)
    {
        // FIXME: This class should be taught how to do the SkipEmptyInlines codepath as well.
        m_current = bidiFirstIncludingEmptyInlines(m_root);
    }

    RenderObject* root() { return m_root; }
    RenderObject* current() { return m_current; }

    bool atEndOfInline() { return m_atEndOfInline; }
    bool atEnd() const { return !m_current; }

    RenderObject* advance()
    {
        // FIXME: Support SkipEmptyInlines and observer parameters.
        m_current = bidiNextIncludingEmptyInlines(m_root, m_current, &m_atEndOfInline);
        return m_current;
    }
private:
    RenderObject* m_root;
    RenderObject* m_current;
    bool m_atEndOfInline;
};

inline void InlineIterator::increment(InlineBidiResolver* resolver)
{
    if (!m_obj)
        return;
    if (m_obj->isText()) {
        fastIncrementInTextNode();
        if (m_pos < toRenderText(m_obj)->textLength())
            return;
    }
    // bidiNext can return 0, so use moveTo instead of moveToStartOf
    moveTo(bidiNextSkippingEmptyInlines(m_root, m_obj, resolver), 0);
}

inline bool InlineIterator::atEnd() const
{
    return !m_obj;
}

inline UChar InlineIterator::current() const
{
    if (!m_obj || !m_obj->isText())
        return 0;

    RenderText* text = toRenderText(m_obj);
    if (m_pos >= text->textLength())
        return 0;

    return text->characters()[m_pos];
}

inline UChar InlineIterator::previousInSameNode() const
{
    if (!m_obj || !m_obj->isText() || !m_pos)
        return 0;

    RenderText* text = toRenderText(m_obj);
    return text->characters()[m_pos - 1];
}

ALWAYS_INLINE WTF::Unicode::Direction InlineIterator::direction() const
{
    if (UChar c = current())
        return WTF::Unicode::direction(c);

    if (m_obj && m_obj->isListMarker())
        return m_obj->style()->isLeftToRightDirection() ? WTF::Unicode::LeftToRight : WTF::Unicode::RightToLeft;

    return WTF::Unicode::OtherNeutral;
}

template<>
inline void InlineBidiResolver::increment()
{
    m_current.increment(this);
}

static inline bool isIsolatedInline(RenderObject* object)
{
    ASSERT(object);
    return object->isRenderInline() && object->style()->unicodeBidi() == Isolate;
}

static inline RenderObject* containingIsolate(RenderObject* object, RenderObject* root)
{
    ASSERT(object);
    while (object && object != root) {
        if (isIsolatedInline(object))
            return object;
        object = object->parent();
    }
    return 0;
}

// FIXME: This belongs on InlineBidiResolver, except it's a template specialization
// of BidiResolver which knows nothing about RenderObjects.
static inline void addPlaceholderRunForIsolatedInline(InlineBidiResolver& resolver, RenderObject* isolatedInline)
{
    ASSERT(isolatedInline);
    BidiRun* isolatedRun = new (isolatedInline->renderArena()) BidiRun(0, 0, isolatedInline, resolver.context(), resolver.dir());
    resolver.runs().addRun(isolatedRun);
    // FIXME: isolatedRuns() could be a hash of object->run and then we could cheaply
    // ASSERT here that we didn't create multiple objects for the same inline.
    resolver.isolatedRuns().append(isolatedRun);
}

class IsolateTracker {
public:
    explicit IsolateTracker(bool inIsolate)
        : m_nestedIsolateCount(inIsolate ? 1 : 0)
        , m_haveAddedFakeRunForRootIsolate(false)
    {
    }

    void enterIsolate() { m_nestedIsolateCount++; }
    void exitIsolate()
    {
        ASSERT(m_nestedIsolateCount >= 1);
        m_nestedIsolateCount--;
        if (!inIsolate())
            m_haveAddedFakeRunForRootIsolate = false;
    }
    bool inIsolate() const { return m_nestedIsolateCount; }

    // We don't care if we encounter bidi directional overrides.
    void embed(WTF::Unicode::Direction, BidiEmbeddingSource) { }

    void addFakeRunIfNecessary(RenderObject* obj, InlineBidiResolver& resolver)
    {
        // We only need to lookup the root isolated span and add a fake run
        // for it once, but we'll be called for every span inside the isolated span
        // so we just ignore the call.
        if (m_haveAddedFakeRunForRootIsolate)
            return;
        m_haveAddedFakeRunForRootIsolate = true;

        // FIXME: position() could be outside the run and may be the wrong call here.
        // If we were passed an InlineIterator instead that would have the right root().
        RenderObject* isolatedInline = containingIsolate(obj, resolver.position().root());
        // FIXME: Because enterIsolate is not passed a RenderObject, we have to crawl up the
        // tree to see which parent inline is the isolate. We could change enterIsolate
        // to take a RenderObject and do this logic there, but that would be a layering
        // violation for BidiResolver (which knows nothing about RenderObject).
        addPlaceholderRunForIsolatedInline(resolver, isolatedInline);
    }

private:
    unsigned m_nestedIsolateCount;
    bool m_haveAddedFakeRunForRootIsolate;
};

template <>
inline void InlineBidiResolver::appendRun()
{
    if (!m_emptyRun && !m_eor.atEnd()) {
        // Keep track of when we enter/leave "unicode-bidi: isolate" inlines.
        // Initialize our state depending on if we're starting in the middle of such an inline.
        // FIXME: Could this initialize from this->inIsolate() instead of walking up the render tree?
        IsolateTracker isolateTracker(containingIsolate(m_sor.m_obj, m_sor.root()));
        int start = m_sor.m_pos;
        RenderObject* obj = m_sor.m_obj;
        while (obj && obj != m_eor.m_obj && obj != endOfLine.m_obj) {
            if (isolateTracker.inIsolate())
                isolateTracker.addFakeRunIfNecessary(obj, *this);
            else
                RenderBlock::appendRunsForObject(m_runs, start, obj->length(), obj, *this);
            // FIXME: start/obj should be an InlineIterator instead of two separate variables.
            start = 0;
            obj = bidiNextSkippingEmptyInlines(m_sor.root(), obj, &isolateTracker);
        }
        if (obj) {
            unsigned pos = obj == m_eor.m_obj ? m_eor.m_pos : UINT_MAX;
            if (obj == endOfLine.m_obj && endOfLine.m_pos <= pos) {
                m_reachedEndOfLine = true;
                pos = endOfLine.m_pos;
            }
            // It's OK to add runs for zero-length RenderObjects, just don't make the run larger than it should be
            int end = obj->length() ? pos + 1 : 0;
            if (isolateTracker.inIsolate())
                isolateTracker.addFakeRunIfNecessary(obj, *this);
            else
                RenderBlock::appendRunsForObject(m_runs, start, end, obj, *this);
        }

        m_eor.increment();
        m_sor = m_eor;
    }

    m_direction = WTF::Unicode::OtherNeutral;
    m_status.eor = WTF::Unicode::OtherNeutral;
}

}

#endif // InlineIterator_h
