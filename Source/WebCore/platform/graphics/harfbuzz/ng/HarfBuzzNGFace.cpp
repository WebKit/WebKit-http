/*
 * Copyright (c) 2012 Google Inc. All rights reserved.
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

#include "config.h"
#include "HarfBuzzNGFace.h"

#include "FontPlatformData.h"
#include "hb-ot.h"
#include "hb.h"

namespace WebCore {

const hb_tag_t HarfBuzzNGFace::vertTag = HB_TAG('v', 'e', 'r', 't');
const hb_tag_t HarfBuzzNGFace::vrt2Tag = HB_TAG('v', 'r', 't', '2');

// Though we have FontCache class, which provides the cache mechanism for
// WebKit's font objects, we also need additional caching layer for HarfBuzz
// to reduce the memory consumption because hb_face_t should be associated with
// underling font data (e.g. CTFontRef, FTFace).

class FaceCacheEntry : public RefCounted<FaceCacheEntry> {
public:
    static PassRefPtr<FaceCacheEntry> create(hb_face_t* face)
    {
        ASSERT(face);
        return adoptRef(new FaceCacheEntry(face));
    }
    ~FaceCacheEntry()
    {
        hb_face_destroy(m_face);
    }

    hb_face_t* face() { return m_face; }
    HashMap<uint32_t, uint16_t>* glyphCache() { return &m_glyphCache; }

private:
    explicit FaceCacheEntry(hb_face_t* face)
        : m_face(face)
    { }

    hb_face_t* m_face;
    HashMap<uint32_t, uint16_t> m_glyphCache;
};

typedef HashMap<uint64_t, RefPtr<FaceCacheEntry>, WTF::IntHash<uint64_t>, WTF::UnsignedWithZeroKeyHashTraits<uint64_t> > HarfBuzzNGFaceCache;

static HarfBuzzNGFaceCache* harfbuzzFaceCache()
{
    DEFINE_STATIC_LOCAL(HarfBuzzNGFaceCache, s_harfbuzzFaceCache, ());
    return &s_harfbuzzFaceCache;
}

HarfBuzzNGFace::HarfBuzzNGFace(FontPlatformData* platformData, uint64_t uniqueID)
    : m_platformData(platformData)
    , m_uniqueID(uniqueID)
    , m_scriptForVerticalText(HB_SCRIPT_INVALID)
{
    HarfBuzzNGFaceCache::AddResult result = harfbuzzFaceCache()->add(m_uniqueID, 0);
    if (result.isNewEntry)
        result.iterator->value = FaceCacheEntry::create(createFace());
    result.iterator->value->ref();
    m_face = result.iterator->value->face();
    m_glyphCacheForFaceCacheEntry = result.iterator->value->glyphCache();
}

HarfBuzzNGFace::~HarfBuzzNGFace()
{
    HarfBuzzNGFaceCache::iterator result = harfbuzzFaceCache()->find(m_uniqueID);
    ASSERT(result != harfbuzzFaceCache()->end());
    ASSERT(result.get()->value->refCount() > 1);
    result.get()->value->deref();
    if (result.get()->value->refCount() == 1)
        harfbuzzFaceCache()->remove(m_uniqueID);
}

static hb_script_t findScriptForVerticalGlyphSubstitution(hb_face_t* face)
{
    static const unsigned maxCount = 32;

    unsigned scriptCount = maxCount;
    hb_tag_t scriptTags[maxCount];
    hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, &scriptCount, scriptTags);
    for (unsigned scriptIndex = 0; scriptIndex < scriptCount; ++scriptIndex) {
        unsigned languageCount = maxCount;
        hb_tag_t languageTags[maxCount];
        hb_ot_layout_script_get_language_tags(face, HB_OT_TAG_GSUB, scriptIndex, 0, &languageCount, languageTags);
        for (unsigned languageIndex = 0; languageIndex < languageCount; ++languageIndex) {
            unsigned featureIndex;
            if (hb_ot_layout_language_find_feature(face, HB_OT_TAG_GSUB, scriptIndex, languageIndex, HarfBuzzNGFace::vertTag, &featureIndex)
                || hb_ot_layout_language_find_feature(face, HB_OT_TAG_GSUB, scriptIndex, languageIndex, HarfBuzzNGFace::vrt2Tag, &featureIndex))
                return hb_ot_tag_to_script(scriptTags[scriptIndex]);
        }
    }
    return HB_SCRIPT_INVALID;
}

void HarfBuzzNGFace::setScriptForVerticalGlyphSubstitution(hb_buffer_t* buffer)
{
    if (m_scriptForVerticalText == HB_SCRIPT_INVALID)
        m_scriptForVerticalText = findScriptForVerticalGlyphSubstitution(m_face);
    hb_buffer_set_script(buffer, m_scriptForVerticalText);
}

} // namespace WebCore
