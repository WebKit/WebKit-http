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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DictionaryPopupInfo.h"

#include "WebCoreArgumentCoders.h"

#if PLATFORM(MAC)
#include "ArgumentCodersCF.h"
#endif

namespace WebKit {

void DictionaryPopupInfo::encode(CoreIPC::ArgumentEncoder* encoder) const
{
    encoder->encode(origin);
    encoder->encode(fontInfo);
    encoder->encodeEnum(type);

#if PLATFORM(MAC) && !defined(BUILDING_ON_SNOW_LEOPARD)
    bool hadOptions = options;
    encoder->encodeBool(hadOptions);
    if (hadOptions)
        CoreIPC::encode(encoder, options.get());
#endif
}

bool DictionaryPopupInfo::decode(CoreIPC::ArgumentDecoder* decoder, DictionaryPopupInfo& result)
{
    if (!decoder->decode(result.origin))
        return false;
    if (!decoder->decode(result.fontInfo))
        return false;
    if (!decoder->decodeEnum(result.type))
        return false;
#if PLATFORM(MAC) && !defined(BUILDING_ON_SNOW_LEOPARD)
    bool hadOptions;
    if (!decoder->decodeBool(hadOptions))
        return false;
    if (hadOptions) {
        if (!CoreIPC::decode(decoder, result.options))
            return false;
    }
#endif
    return true;
}

} // namespace WebKit
