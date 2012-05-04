/*
 * Copyright (C) 1998-2000 Netscape Communications Corporation.
 * Copyright (C) 2003-6 Apple Computer
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * Other contributors:
 *   Nick Blievers <nickb@adacel.com.au>
 *   Jeff Hostetler <jeff@nerdone.com>
 *   Tom Rini <trini@kernel.crashing.org>
 *   Raffaele Sena <raff@netwinder.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#ifndef Arena_h
#define Arena_h

// FIXME: We'd always like to use AllocAlignmentInteger for Arena alignment
// but there is concern over the memory growth this may cause.
#ifdef WTF_USE_ARENA_ALLOC_ALIGNMENT_INTEGER
#define ARENA_ALIGN_MASK (sizeof(WTF::AllocAlignmentInteger) - 1)
#else
#define ARENA_ALIGN_MASK 3
#endif

namespace WebCore {

typedef uintptr_t uword;

struct Arena {
    Arena* next;        // next arena
    uword base;         // aligned base address
    uword limit;        // end of arena (1+last byte)
    uword avail;        // points to next available byte in arena
};

struct ArenaPool {
    Arena first;        // first arena in pool list.
    Arena* current;     // current arena.
    unsigned int arenasize;
    uword mask;         // Mask (power-of-2 - 1)
};

void InitArenaPool(ArenaPool*, const char* name, unsigned int size, unsigned int align);
void FinishArenaPool(ArenaPool*);
void* ArenaAllocate(ArenaPool*, unsigned int numBytes, unsigned int& bytesAllocated);

#define ARENA_ALIGN(n) (((uword)(n) + ARENA_ALIGN_MASK) & ~ARENA_ALIGN_MASK)
#define INIT_ARENA_POOL(pool, name, size) InitArenaPool(pool, name, size, ARENA_ALIGN_MASK + 1)
#define ARENA_ALLOCATE(p, pool, nb, bytesAllocated) \
    Arena* _a = (pool)->current; \
    unsigned int _nb = ARENA_ALIGN(nb); \
    uword _p = _a->avail; \
    uword _q = _p + _nb; \
    if (_q > _a->limit) \
        _p = (uword)ArenaAllocate(pool, _nb, *bytesAllocated); \
    else \
        _a->avail = _q; \
    p = (void*)_p;

}

#endif
