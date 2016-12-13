/*
 *  Copyright (C) 2016 Igalia S.L
 *  Copyright (C) 2016 Metrological Group B.V.
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
 */

#include "config.h"
#include "GstAllocatorFastMalloc.h"

#if USE(GSTREAMER)

#include <cstring>
#include <wtf/FastMalloc.h>

#define WEBKIT_IS_VIDEO_SINK(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_VIDEO_SINK))

G_DEFINE_TYPE(GstAllocatorFastMalloc, gst_allocator_fast_malloc, GST_TYPE_ALLOCATOR);

static GstMemory* gst_allocator_fast_malloc_alloc(GstAllocator* allocator, gsize size, GstAllocationParams* params)
{
    ASSERT(G_TYPE_CHECK_INSTANCE_TYPE(allocator, gst_allocator_fast_malloc_get_type()));

    // alignment should be a (power-of-two - 1).
    gsize alignment = params->align | gst_memory_alignment;
    ASSERT(!((alignment + 1) & alignment));

    gsize headerSize = (sizeof(GstMemoryFastMalloc) + alignment) & ~alignment;
    gsize allocationSize = params->prefix + size + params->padding;

    GstMemoryFastMalloc* mem = static_cast<GstMemoryFastMalloc*>(fastAlignedMalloc(alignment + 1, headerSize + allocationSize));
    if (!mem)
        return nullptr;

    mem->data = reinterpret_cast<uint8_t*>(mem) + headerSize;

    if (params->prefix && (params->flags & GST_MEMORY_FLAG_ZERO_PREFIXED))
        std::memset(mem->data, 0, params->prefix);
    if (params->padding && (params->flags & GST_MEMORY_FLAG_ZERO_PADDED))
        std::memset(mem->data + params->prefix + size, 0, params->padding);

    gst_memory_init(GST_MEMORY_CAST(mem), params->flags, allocator, nullptr,
        allocationSize, alignment, params->prefix, size);

    return GST_MEMORY_CAST(mem);
}

static void gst_allocator_fast_malloc_free(GstAllocator*, GstMemory* mem)
{
    ASSERT(G_TYPE_CHECK_INSTANCE_TYPE(allocator, gst_allocator_fast_malloc_get_type()));

    fastFree(mem);
}

static gpointer gst_allocator_fast_malloc_mem_map(GstMemoryFastMalloc* mem, gsize, GstMapFlags)
{
    return mem->data;
}

static gboolean gst_allocator_fast_malloc_mem_unmap(GstMemoryFastMalloc*)
{
    return TRUE;
}

static GstMemoryFastMalloc* gst_allocator_fast_malloc_mem_copy(GstMemoryFastMalloc* mem, gssize offset, gsize size)
{
    gsize alignment = mem->base.align | gst_memory_alignment;
    ASSERT(!((alignment + 1) & alignment));

    gsize headerSize = (sizeof(GstMemoryFastMalloc) + alignment) & ~alignment;

    gsize allocationSize = size;
    if (allocationSize == static_cast<gsize>(-1)) {
        allocationSize = 0;
        if (offset < 0 || (mem->base.size > static_cast<gsize>(offset)))
            allocationSize = mem->base.size - offset;
    }

    GstMemoryFastMalloc* copy = static_cast<GstMemoryFastMalloc*>(fastAlignedMalloc(alignment + 1, headerSize + allocationSize));
    if (!copy)
        return nullptr;

    gst_memory_init(GST_MEMORY_CAST(copy), static_cast<GstMemoryFlags>(0),
        mem->base.allocator, nullptr, allocationSize, alignment, 0, allocationSize);

    copy->data = reinterpret_cast<uint8_t*>(copy) + headerSize;
    std::memcpy(copy->data, mem->data + mem->base.offset + offset, allocationSize);

    return nullptr;
}

static GstMemoryFastMalloc* gst_allocator_fast_malloc_mem_share(GstMemoryFastMalloc* mem, gssize offset, gsize size)
{
    GstMemory* parent = mem->base.parent;
    if (!parent)
        parent = GST_MEMORY_CAST(mem);

    if (size == static_cast<gsize>(-1))
        size = mem->base.size - offset;

    GstMemoryFastMalloc* sharedMem = static_cast<GstMemoryFastMalloc*>(fastMalloc(sizeof(GstMemoryFastMalloc)));
    gst_memory_init(GST_MEMORY_CAST(sharedMem),
        static_cast<GstMemoryFlags>(GST_MINI_OBJECT_FLAGS(parent) | GST_MINI_OBJECT_FLAG_LOCK_READONLY),
        mem->base.allocator, parent, mem->base.maxsize, mem->base.align,
        mem->base.offset + offset, size);

    sharedMem->data = mem->data;

    return sharedMem;
}

static gboolean gst_allocator_fast_malloc_mem_is_span(GstMemoryFastMalloc* mem1, GstMemoryFastMalloc* mem2, gsize* offset)
{
    if (offset) {
        auto* parent = reinterpret_cast<GstMemoryFastMalloc*>(mem1->base.parent);
        ASSERT(parent);
        *offset = mem1->base.offset - parent->base.offset;
    }

    return mem1->data + mem1->base.offset + mem1->base.size == mem2->data + mem2->base.offset;
}

static void gst_allocator_fast_malloc_finalize(GObject*)
{
}

static void gst_allocator_fast_malloc_class_init(GstAllocatorFastMallocClass* klass)
{
    auto* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = gst_allocator_fast_malloc_finalize;

    auto* gstAllocatorClass = GST_ALLOCATOR_CLASS(klass);
    gstAllocatorClass->alloc = gst_allocator_fast_malloc_alloc;
    gstAllocatorClass->free = gst_allocator_fast_malloc_free;
}

static void gst_allocator_fast_malloc_init(GstAllocatorFastMalloc* allocator)
{
    GstAllocator* baseAllocator = GST_ALLOCATOR_CAST(allocator);

    baseAllocator->mem_type = "FastMalloc";
    baseAllocator->mem_map = reinterpret_cast<GstMemoryMapFunction>(gst_allocator_fast_malloc_mem_map);
    baseAllocator->mem_unmap = reinterpret_cast<GstMemoryUnmapFunction>(gst_allocator_fast_malloc_mem_unmap);
    baseAllocator->mem_copy = reinterpret_cast<GstMemoryCopyFunction>(gst_allocator_fast_malloc_mem_copy);
    baseAllocator->mem_share = reinterpret_cast<GstMemoryShareFunction>(gst_allocator_fast_malloc_mem_share);
    baseAllocator->mem_is_span = reinterpret_cast<GstMemoryIsSpanFunction>(gst_allocator_fast_malloc_mem_is_span);
}

#endif // USE(GSTREAMER)
