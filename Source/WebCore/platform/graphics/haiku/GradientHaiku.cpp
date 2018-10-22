/*
 * Copyright (C) 2008 Kevin Ollivier <kevino@theolliviers.com>  All rights reserved.
 * Copyright (C) 2009 Maxime Simon <simon.maxime@theolliviers.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Gradient.h"

#include "GraphicsContext.h"
#include <GradientConic.h>
#include <GradientLinear.h>
#include <GradientRadialFocus.h>
#include <View.h>


namespace WebCore {

void Gradient::platformDestroy()
{
    delete m_gradient;
    m_gradient = NULL;
}

PlatformGradient Gradient::platformGradient()
{
	if (m_gradient)
		return m_gradient;

	m_gradient = WTF::switchOn(m_data,
		[&] (const RadialData& data) -> BGradient* {
			return new BGradientRadialFocus(data.point1, data.endRadius, data.point0);
		},
		[&] (const LinearData& data) -> BGradient* {
			return new BGradientLinear(data.point0, data.point1);
		},
		[&] (const ConicData& data) -> BGradient* {
			return new BGradientConic(data.point0, data.angleRadians);
		}
		);

    size_t size = m_stops.size();
    for (size_t i = 0; i < size; i++) {
        const ColorStop& stop = m_stops[i];
        rgb_color color(stop.color);
        m_gradient->AddColor(color, stop.offset * 255);

        // TODO handle m_spreadMethod (pad/reflect/repeat)
    }
    return m_gradient;
}

PlatformGradient Gradient::createPlatformGradient(float globalAlpha)
{
	return platformGradient();
}

void Gradient::fill(GraphicsContext& context, const FloatRect& rect)
{
    context.platformContext()->FillRect(rect, *platformGradient());
}

} // namespace WebCore

