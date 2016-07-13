/*
* Copyright (c) 2016, Comcast
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR OR; PROFITS BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY OF THEORY LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef Proxy_h
#define Proxy_h

#include <wtf/text/WTFString.h>

namespace WebCore {

struct Proxy {
	Proxy() {}

	Proxy(const String& pattern, const String& proxy)
		: pattern(pattern), proxy(proxy) {}

	template<class Encoder> void encode(Encoder&) const;
	template<class Decoder> static bool decode(Decoder&, Proxy&);

	String pattern;
	String proxy;
};

template<class Encoder>
void Proxy::encode(Encoder& encoder) const {
    encoder << pattern;
    encoder << proxy;
}

template<class Decoder>
bool Proxy::decode(Decoder& decoder, Proxy& proxy) {
    if (!decoder.decode(proxy.pattern))
        return false;
    if (!decoder.decode(proxy.proxy))
        return false;

    return true;
}

}

#endif
