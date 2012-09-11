/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
 */

#include "config.h"

#if ENABLE(FILTERS)
#include "FEComponentTransfer.h"

#include "Filter.h"
#include "GraphicsContext.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Uint8ClampedArray.h>

namespace WebCore {

typedef void (*TransferType)(unsigned char*, const ComponentTransferFunction&);

FEComponentTransfer::FEComponentTransfer(Filter* filter, const ComponentTransferFunction& redFunc, const ComponentTransferFunction& greenFunc,
                                         const ComponentTransferFunction& blueFunc, const ComponentTransferFunction& alphaFunc)
    : FilterEffect(filter)
    , m_redFunc(redFunc)
    , m_greenFunc(greenFunc)
    , m_blueFunc(blueFunc)
    , m_alphaFunc(alphaFunc)
{
}

PassRefPtr<FEComponentTransfer> FEComponentTransfer::create(Filter* filter, const ComponentTransferFunction& redFunc, 
    const ComponentTransferFunction& greenFunc, const ComponentTransferFunction& blueFunc, const ComponentTransferFunction& alphaFunc)
{
    return adoptRef(new FEComponentTransfer(filter, redFunc, greenFunc, blueFunc, alphaFunc));
}

ComponentTransferFunction FEComponentTransfer::redFunction() const
{
    return m_redFunc;
}

void FEComponentTransfer::setRedFunction(const ComponentTransferFunction& func)
{
    m_redFunc = func;
}

ComponentTransferFunction FEComponentTransfer::greenFunction() const
{
    return m_greenFunc;
}

void FEComponentTransfer::setGreenFunction(const ComponentTransferFunction& func)
{
    m_greenFunc = func;
}

ComponentTransferFunction FEComponentTransfer::blueFunction() const
{
    return m_blueFunc;
}

void FEComponentTransfer::setBlueFunction(const ComponentTransferFunction& func)
{
    m_blueFunc = func;
}

ComponentTransferFunction FEComponentTransfer::alphaFunction() const
{
    return m_alphaFunc;
}

void FEComponentTransfer::setAlphaFunction(const ComponentTransferFunction& func)
{
    m_alphaFunc = func;
}

static void identity(unsigned char*, const ComponentTransferFunction&)
{
}

static void table(unsigned char* values, const ComponentTransferFunction& transferFunction)
{
    const Vector<float>& tableValues = transferFunction.tableValues;
    unsigned n = tableValues.size();
    if (n < 1)
        return;            
    for (unsigned i = 0; i < 256; ++i) {
        double c = i / 255.0;                
        unsigned k = static_cast<unsigned>(c * (n - 1));
        double v1 = tableValues[k];
        double v2 = tableValues[std::min((k + 1), (n - 1))];
        double val = 255.0 * (v1 + (c * (n - 1) - k) * (v2 - v1));
        val = std::max(0.0, std::min(255.0, val));
        values[i] = static_cast<unsigned char>(val);
    }
}

static void discrete(unsigned char* values, const ComponentTransferFunction& transferFunction)
{
    const Vector<float>& tableValues = transferFunction.tableValues;
    unsigned n = tableValues.size();
    if (n < 1)
        return;
    for (unsigned i = 0; i < 256; ++i) {
        unsigned k = static_cast<unsigned>((i * n) / 255.0);
        k = std::min(k, n - 1);
        double val = 255 * tableValues[k];
        val = std::max(0.0, std::min(255.0, val));
        values[i] = static_cast<unsigned char>(val);
    }
}

static void linear(unsigned char* values, const ComponentTransferFunction& transferFunction)
{
    for (unsigned i = 0; i < 256; ++i) {
        double val = transferFunction.slope * i + 255 * transferFunction.intercept;
        val = std::max(0.0, std::min(255.0, val));
        values[i] = static_cast<unsigned char>(val);
    }
}

static void gamma(unsigned char* values, const ComponentTransferFunction& transferFunction)
{
    for (unsigned i = 0; i < 256; ++i) {
        double exponent = transferFunction.exponent; // RCVT doesn't like passing a double and a float to pow, so promote this to double
        double val = 255.0 * (transferFunction.amplitude * pow((i / 255.0), exponent) + transferFunction.offset);
        val = std::max(0.0, std::min(255.0, val));
        values[i] = static_cast<unsigned char>(val);
    }
}

void FEComponentTransfer::platformApplySoftware()
{
    FilterEffect* in = inputEffect(0);

    Uint8ClampedArray* pixelArray = createUnmultipliedImageResult();
    if (!pixelArray)
        return;

    unsigned char rValues[256], gValues[256], bValues[256], aValues[256];
    getValues(rValues, gValues, bValues, aValues);
    unsigned char* tables[] = { rValues, gValues, bValues, aValues };

    IntRect drawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
    in->copyUnmultipliedImage(pixelArray, drawingRect);

    unsigned pixelArrayLength = pixelArray->length();
    for (unsigned pixelOffset = 0; pixelOffset < pixelArrayLength; pixelOffset += 4) {
        for (unsigned channel = 0; channel < 4; ++channel) {
            unsigned char c = pixelArray->item(pixelOffset + channel);
            pixelArray->set(pixelOffset + channel, tables[channel][c]);
        }
    }
}

void FEComponentTransfer::getValues(unsigned char rValues[256], unsigned char gValues[256], unsigned char bValues[256], unsigned char aValues[256])
{
    for (unsigned i = 0; i < 256; ++i)
        rValues[i] = gValues[i] = bValues[i] = aValues[i] = i;
    unsigned char* tables[] = { rValues, gValues, bValues, aValues };
    ComponentTransferFunction transferFunction[] = {m_redFunc, m_greenFunc, m_blueFunc, m_alphaFunc};
    TransferType callEffect[] = {identity, identity, table, discrete, linear, gamma};

    for (unsigned channel = 0; channel < 4; channel++) {
        ASSERT(static_cast<size_t>(transferFunction[channel].type) < WTF_ARRAY_LENGTH(callEffect));
        (*callEffect[transferFunction[channel].type])(tables[channel], transferFunction[channel]);
    }
}

void FEComponentTransfer::dump()
{
}

static TextStream& operator<<(TextStream& ts, const ComponentTransferType& type)
{
    switch (type) {
    case FECOMPONENTTRANSFER_TYPE_UNKNOWN:
        ts << "UNKNOWN";
        break;
    case FECOMPONENTTRANSFER_TYPE_IDENTITY:
        ts << "IDENTITY";
        break;
    case FECOMPONENTTRANSFER_TYPE_TABLE:
        ts << "TABLE";
        break;
    case FECOMPONENTTRANSFER_TYPE_DISCRETE:
        ts << "DISCRETE";
        break;
    case FECOMPONENTTRANSFER_TYPE_LINEAR:
        ts << "LINEAR";
        break;
    case FECOMPONENTTRANSFER_TYPE_GAMMA:
        ts << "GAMMA";
        break;
    }
    return ts;
}

static TextStream& operator<<(TextStream& ts, const ComponentTransferFunction& function)
{
    ts << "type=\"" << function.type 
       << "\" slope=\"" << function.slope
       << "\" intercept=\"" << function.intercept
       << "\" amplitude=\"" << function.amplitude
       << "\" exponent=\"" << function.exponent
       << "\" offset=\"" << function.offset << "\"";
    return ts;
}

TextStream& FEComponentTransfer::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feComponentTransfer";
    FilterEffect::externalRepresentation(ts);
    ts << " \n";
    writeIndent(ts, indent + 2);
    ts << "{red: " << m_redFunc << "}\n";
    writeIndent(ts, indent + 2);
    ts << "{green: " << m_greenFunc << "}\n";
    writeIndent(ts, indent + 2);
    ts << "{blue: " << m_blueFunc << "}\n";    
    writeIndent(ts, indent + 2);
    ts << "{alpha: " << m_alphaFunc << "}]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);
    return ts;
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
