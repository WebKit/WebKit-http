/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#import "config.h"
#import "RemoteLayerTreeTransaction.h"

#import "ArgumentCoders.h"
#import "MessageDecoder.h"
#import "MessageEncoder.h"
#import "PlatformCALayerRemote.h"
#import "WebCoreArgumentCoders.h"
#import <WebCore/TextStream.h>
#import <wtf/text/CString.h>
#import <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace WebKit {

RemoteLayerTreeTransaction::LayerCreationProperties::LayerCreationProperties()
{
}

void RemoteLayerTreeTransaction::LayerCreationProperties::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << layerID;
    encoder.encodeEnum(type);
}

bool RemoteLayerTreeTransaction::LayerCreationProperties::decode(CoreIPC::ArgumentDecoder& decoder, LayerCreationProperties& result)
{
    if (!decoder.decode(result.layerID))
        return false;

    if (!decoder.decodeEnum(result.type))
        return false;

    return true;
}

RemoteLayerTreeTransaction::LayerProperties::LayerProperties()
    : changedProperties(NoChange)
{
}

void RemoteLayerTreeTransaction::LayerProperties::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder.encodeEnum(changedProperties);

    if (changedProperties & NameChanged)
        encoder << name;

    if (changedProperties & ChildrenChanged)
        encoder << children;

    if (changedProperties & PositionChanged)
        encoder << position;

    if (changedProperties & SizeChanged)
        encoder << size;

    if (changedProperties & BackgroundColorChanged)
        encoder << backgroundColor;

    if (changedProperties & AnchorPointChanged)
        encoder << anchorPoint;

    if (changedProperties & BorderWidthChanged)
        encoder << borderWidth;

    if (changedProperties & BorderColorChanged)
        encoder << borderColor;

    if (changedProperties & OpacityChanged)
        encoder << opacity;

    if (changedProperties & TransformChanged)
        encoder << transform;

    if (changedProperties & SublayerTransformChanged)
        encoder << sublayerTransform;

    if (changedProperties & HiddenChanged)
        encoder << hidden;

    if (changedProperties & GeometryFlippedChanged)
        encoder << geometryFlipped;

    if (changedProperties & DoubleSidedChanged)
        encoder << doubleSided;

    if (changedProperties & MasksToBoundsChanged)
        encoder << masksToBounds;

    if (changedProperties & OpaqueChanged)
        encoder << opaque;

    if (changedProperties & MaskLayerChanged)
        encoder << maskLayer;

    if (changedProperties & ContentsRectChanged)
        encoder << contentsRect;

    if (changedProperties & ContentsScaleChanged)
        encoder << contentsScale;

    if (changedProperties & MinificationFilterChanged)
        encoder.encodeEnum(minificationFilter);

    if (changedProperties & MagnificationFilterChanged)
        encoder.encodeEnum(magnificationFilter);

    if (changedProperties & SpeedChanged)
        encoder << speed;

    if (changedProperties & TimeOffsetChanged)
        encoder << timeOffset;

    if (changedProperties & BackingStoreChanged)
        encoder << backingStore;

    if (changedProperties & FiltersChanged)
        encoder << filters;

    if (changedProperties & EdgeAntialiasingMaskChanged)
        encoder << edgeAntialiasingMask;
}

bool RemoteLayerTreeTransaction::LayerProperties::decode(CoreIPC::ArgumentDecoder& decoder, LayerProperties& result)
{
    if (!decoder.decodeEnum(result.changedProperties))
        return false;

    if (result.changedProperties & NameChanged) {
        if (!decoder.decode(result.name))
            return false;
    }

    if (result.changedProperties & ChildrenChanged) {
        if (!decoder.decode(result.children))
            return false;

        for (auto layerID : result.children) {
            if (!layerID)
                return false;
        }
    }

    if (result.changedProperties & PositionChanged) {
        if (!decoder.decode(result.position))
            return false;
    }

    if (result.changedProperties & SizeChanged) {
        if (!decoder.decode(result.size))
            return false;
    }

    if (result.changedProperties & BackgroundColorChanged) {
        if (!decoder.decode(result.backgroundColor))
            return false;
    }

    if (result.changedProperties & AnchorPointChanged) {
        if (!decoder.decode(result.anchorPoint))
            return false;
    }

    if (result.changedProperties & BorderWidthChanged) {
        if (!decoder.decode(result.borderWidth))
            return false;
    }

    if (result.changedProperties & BorderColorChanged) {
        if (!decoder.decode(result.borderColor))
            return false;
    }

    if (result.changedProperties & OpacityChanged) {
        if (!decoder.decode(result.opacity))
            return false;
    }

    if (result.changedProperties & TransformChanged) {
        if (!decoder.decode(result.transform))
            return false;
    }

    if (result.changedProperties & SublayerTransformChanged) {
        if (!decoder.decode(result.sublayerTransform))
            return false;
    }

    if (result.changedProperties & HiddenChanged) {
        if (!decoder.decode(result.hidden))
            return false;
    }

    if (result.changedProperties & GeometryFlippedChanged) {
        if (!decoder.decode(result.geometryFlipped))
            return false;
    }

    if (result.changedProperties & DoubleSidedChanged) {
        if (!decoder.decode(result.doubleSided))
            return false;
    }

    if (result.changedProperties & MasksToBoundsChanged) {
        if (!decoder.decode(result.masksToBounds))
            return false;
    }

    if (result.changedProperties & OpaqueChanged) {
        if (!decoder.decode(result.opaque))
            return false;
    }

    if (result.changedProperties & MaskLayerChanged) {
        if (!decoder.decode(result.maskLayer))
            return false;
    }

    if (result.changedProperties & ContentsRectChanged) {
        if (!decoder.decode(result.contentsRect))
            return false;
    }

    if (result.changedProperties & ContentsScaleChanged) {
        if (!decoder.decode(result.contentsScale))
            return false;
    }

    if (result.changedProperties & MinificationFilterChanged) {
        if (!decoder.decodeEnum(result.minificationFilter))
            return false;
    }

    if (result.changedProperties & MagnificationFilterChanged) {
        if (!decoder.decodeEnum(result.magnificationFilter))
            return false;
    }

    if (result.changedProperties & SpeedChanged) {
        if (!decoder.decode(result.speed))
            return false;
    }

    if (result.changedProperties & TimeOffsetChanged) {
        if (!decoder.decode(result.timeOffset))
            return false;
    }

    if (result.changedProperties & BackingStoreChanged) {
        if (!decoder.decode(result.backingStore))
            return false;
    }

    if (result.changedProperties & FiltersChanged) {
        if (!decoder.decode(result.filters))
            return false;
    }

    if (result.changedProperties & EdgeAntialiasingMaskChanged) {
        if (!decoder.decode(result.edgeAntialiasingMask))
            return false;
    }

    return true;
}

RemoteLayerTreeTransaction::RemoteLayerTreeTransaction()
{
}

RemoteLayerTreeTransaction::~RemoteLayerTreeTransaction()
{
}

void RemoteLayerTreeTransaction::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << m_rootLayerID;
    encoder << m_createdLayers;
    encoder << m_changedLayerProperties;
    encoder << m_destroyedLayerIDs;
}

bool RemoteLayerTreeTransaction::decode(CoreIPC::ArgumentDecoder& decoder, RemoteLayerTreeTransaction& result)
{
    if (!decoder.decode(result.m_rootLayerID))
        return false;
    if (!result.m_rootLayerID)
        return false;

    if (!decoder.decode(result.m_createdLayers))
        return false;

    if (!decoder.decode(result.m_changedLayerProperties))
        return false;

    if (!decoder.decode(result.m_destroyedLayerIDs))
        return false;
    for (LayerID layerID : result.m_destroyedLayerIDs) {
        if (!layerID)
            return false;
    }

    return true;
}

void RemoteLayerTreeTransaction::setRootLayerID(LayerID rootLayerID)
{
    ASSERT_ARG(rootLayerID, rootLayerID);

    m_rootLayerID = rootLayerID;
}

void RemoteLayerTreeTransaction::layerPropertiesChanged(PlatformCALayerRemote* remoteLayer, RemoteLayerTreeTransaction::LayerProperties& properties)
{
    m_changedLayerProperties.set(remoteLayer->layerID(), properties);
}

void RemoteLayerTreeTransaction::setCreatedLayers(Vector<LayerCreationProperties> createdLayers)
{
    m_createdLayers = std::move(createdLayers);
}

void RemoteLayerTreeTransaction::setDestroyedLayerIDs(Vector<LayerID> destroyedLayerIDs)
{
    m_destroyedLayerIDs = std::move(destroyedLayerIDs);
}

#if !defined(NDEBUG)

class RemoteLayerTreeTextStream : public TextStream
{
public:
    using TextStream::operator<<;

    RemoteLayerTreeTextStream()
        : m_indent(0)
    {
    }

    RemoteLayerTreeTextStream& operator<<(const TransformationMatrix&);
    RemoteLayerTreeTextStream& operator<<(PlatformCALayer::FilterType);
    RemoteLayerTreeTextStream& operator<<(FloatPoint3D);
    RemoteLayerTreeTextStream& operator<<(Color);
    RemoteLayerTreeTextStream& operator<<(FloatRect);
    RemoteLayerTreeTextStream& operator<<(const Vector<RemoteLayerTreeTransaction::LayerID>& layers);
    RemoteLayerTreeTextStream& operator<<(const FilterOperations&);

    void increaseIndent() { ++m_indent; }
    void decreaseIndent() { --m_indent; ASSERT(m_indent >= 0); }

    void writeIndent();

private:
    int m_indent;
};

RemoteLayerTreeTextStream& RemoteLayerTreeTextStream::operator<<(const TransformationMatrix& transform)
{
    RemoteLayerTreeTextStream& ts = *this;
    ts << "\n";
    ts.increaseIndent();
    ts.writeIndent();
    ts << "[" << transform.m11() << " " << transform.m12() << " " << transform.m13() << " " << transform.m14() << "]\n";
    ts.writeIndent();
    ts << "[" << transform.m21() << " " << transform.m22() << " " << transform.m23() << " " << transform.m24() << "]\n";
    ts.writeIndent();
    ts << "[" << transform.m31() << " " << transform.m32() << " " << transform.m33() << " " << transform.m34() << "]\n";
    ts.writeIndent();
    ts << "[" << transform.m41() << " " << transform.m42() << " " << transform.m43() << " " << transform.m44() << "]";
    ts.decreaseIndent();
    return ts;
}

RemoteLayerTreeTextStream& RemoteLayerTreeTextStream::operator<<(PlatformCALayer::FilterType filterType)
{
    RemoteLayerTreeTextStream& ts = *this;
    switch (filterType) {
    case PlatformCALayer::Linear:
        ts << "linear";
        break;
    case PlatformCALayer::Nearest:
        ts << "nearest";
        break;
    case PlatformCALayer::Trilinear:
        ts << "trilinear";
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
    return ts;
}

RemoteLayerTreeTextStream& RemoteLayerTreeTextStream::operator<<(const FilterOperations& filters)
{
    RemoteLayerTreeTextStream& ts = *this;
    for (size_t i = 0; i < filters.size(); ++i) {
        const auto filter = filters.at(i);
        switch (filter->type()) {
        case FilterOperation::REFERENCE:
            ts << "reference";
            break;
        case FilterOperation::GRAYSCALE:
            ts << "grayscale";
            break;
        case FilterOperation::SEPIA:
            ts << "sepia";
            break;
        case FilterOperation::SATURATE:
            ts << "saturate";
            break;
        case FilterOperation::HUE_ROTATE:
            ts << "hue rotate";
            break;
        case FilterOperation::INVERT:
            ts << "invert";
            break;
        case FilterOperation::OPACITY:
            ts << "opacity";
            break;
        case FilterOperation::BRIGHTNESS:
            ts << "brightness";
            break;
        case FilterOperation::CONTRAST:
            ts << "contrast";
            break;
        case FilterOperation::BLUR:
            ts << "blur";
            break;
        case FilterOperation::DROP_SHADOW:
            ts << "drop shadow";
            break;
#if ENABLE(CSS_SHADERS)
        case FilterOperation::CUSTOM:
            ts << "custom";
            break;
        case FilterOperation::VALIDATED_CUSTOM:
            ts << "custom (validated)";
            break;
#endif
        case FilterOperation::PASSTHROUGH:
            ts << "passthrough";
            break;
        case FilterOperation::NONE:
            ts << "none";
            break;
        }

        if (i < filters.size() - 1)
            ts << " ";
    }
    return ts;
}

RemoteLayerTreeTextStream& RemoteLayerTreeTextStream::operator<<(FloatPoint3D point)
{
    RemoteLayerTreeTextStream& ts = *this;
    ts << point.x() << " " << point.y() << " " << point.z();
    return ts;
}

RemoteLayerTreeTextStream& RemoteLayerTreeTextStream::operator<<(Color color)
{
    RemoteLayerTreeTextStream& ts = *this;
    ts << color.serialized();
    return ts;
}

RemoteLayerTreeTextStream& RemoteLayerTreeTextStream::operator<<(FloatRect rect)
{
    RemoteLayerTreeTextStream& ts = *this;
    ts << rect.x() << " " << rect.y() << " " << rect.width() << " " << rect.height();
    return ts;
}

RemoteLayerTreeTextStream& RemoteLayerTreeTextStream::operator<<(const Vector<RemoteLayerTreeTransaction::LayerID>& layers)
{
    RemoteLayerTreeTextStream& ts = *this;

    for (size_t i = 0; i < layers.size(); ++i) {
        if (i)
            ts << " ";
        ts << layers[i];
    }

    return ts;
}

void RemoteLayerTreeTextStream::writeIndent()
{
    for (int i = 0; i < m_indent; ++i)
        *this << "  ";
}

template <class T>
static void dumpProperty(RemoteLayerTreeTextStream& ts, String name, T value)
{
    ts << "\n";
    ts.increaseIndent();
    ts.writeIndent();
    ts << "(" << name << " ";
    ts << value << ")";
    ts.decreaseIndent();
}

static void dumpChangedLayers(RemoteLayerTreeTextStream& ts, const HashMap<RemoteLayerTreeTransaction::LayerID, RemoteLayerTreeTransaction::LayerProperties>& changedLayerProperties)
{
    if (changedLayerProperties.isEmpty())
        return;

    ts << "\n";
    ts.writeIndent();
    ts << "(changed-layers";

    // Dump the layer properties sorted by layer ID.
    Vector<RemoteLayerTreeTransaction::LayerID> layerIDs;
    copyKeysToVector(changedLayerProperties, layerIDs);
    std::sort(layerIDs.begin(), layerIDs.end());

    for (auto layerID : layerIDs) {
        const RemoteLayerTreeTransaction::LayerProperties& layerProperties = changedLayerProperties.get(layerID);

        ts << "\n";
        ts.increaseIndent();
        ts.writeIndent();
        ts << "(layer " << layerID;

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::NameChanged)
            dumpProperty<String>(ts, "name", layerProperties.name);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::ChildrenChanged)
            dumpProperty<Vector<RemoteLayerTreeTransaction::LayerID>>(ts, "children", layerProperties.children);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::PositionChanged)
            dumpProperty<FloatPoint3D>(ts, "position", layerProperties.position);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::SizeChanged)
            dumpProperty<FloatSize>(ts, "size", layerProperties.size);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::AnchorPointChanged)
            dumpProperty<FloatPoint3D>(ts, "anchorPoint", layerProperties.anchorPoint);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::BackgroundColorChanged)
            dumpProperty<Color>(ts, "backgroundColor", layerProperties.backgroundColor);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::BorderColorChanged)
            dumpProperty<Color>(ts, "borderColor", layerProperties.borderColor);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::BorderWidthChanged)
            dumpProperty<float>(ts, "borderWidth", layerProperties.borderWidth);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::OpacityChanged)
            dumpProperty<float>(ts, "opacity", layerProperties.opacity);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::TransformChanged)
            dumpProperty<TransformationMatrix>(ts, "transform", layerProperties.transform);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::SublayerTransformChanged)
            dumpProperty<TransformationMatrix>(ts, "sublayerTransform", layerProperties.sublayerTransform);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::HiddenChanged)
            dumpProperty<bool>(ts, "hidden", layerProperties.hidden);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::GeometryFlippedChanged)
            dumpProperty<bool>(ts, "geometryFlipped", layerProperties.geometryFlipped);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::DoubleSidedChanged)
            dumpProperty<bool>(ts, "doubleSided", layerProperties.doubleSided);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::MasksToBoundsChanged)
            dumpProperty<bool>(ts, "masksToBounds", layerProperties.masksToBounds);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::OpaqueChanged)
            dumpProperty<bool>(ts, "opaque", layerProperties.opaque);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::MaskLayerChanged)
            dumpProperty<RemoteLayerTreeTransaction::LayerID>(ts, "maskLayer", layerProperties.maskLayer);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::ContentsRectChanged)
            dumpProperty<FloatRect>(ts, "contentsRect", layerProperties.contentsRect);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::ContentsScaleChanged)
            dumpProperty<float>(ts, "contentsScale", layerProperties.contentsScale);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::MinificationFilterChanged)
            dumpProperty<PlatformCALayer::FilterType>(ts, "minificationFilter", layerProperties.minificationFilter);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::MagnificationFilterChanged)
            dumpProperty<PlatformCALayer::FilterType>(ts, "magnificationFilter", layerProperties.magnificationFilter);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::SpeedChanged)
            dumpProperty<float>(ts, "speed", layerProperties.speed);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::TimeOffsetChanged)
            dumpProperty<double>(ts, "timeOffset", layerProperties.timeOffset);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::BackingStoreChanged)
            dumpProperty<IntSize>(ts, "backingStore", layerProperties.backingStore.size());

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::FiltersChanged)
            dumpProperty<FilterOperations>(ts, "filters", layerProperties.filters);

        if (layerProperties.changedProperties & RemoteLayerTreeTransaction::EdgeAntialiasingMaskChanged)
            dumpProperty<unsigned>(ts, "edgeAntialiasingMask", layerProperties.edgeAntialiasingMask);

        ts << ")";

        ts.decreaseIndent();
    }

    ts.decreaseIndent();
}

void RemoteLayerTreeTransaction::dump() const
{
    RemoteLayerTreeTextStream ts;

    ts << "(\n";
    ts.increaseIndent();
    ts.writeIndent();
    ts << "(root-layer " << m_rootLayerID << ")";

    if (!m_createdLayers.isEmpty()) {
        ts << "\n";
        ts.writeIndent();
        ts << "(created-layers";
        ts.increaseIndent();
        for (const auto& createdLayer : m_createdLayers) {
            ts << "\n";
            ts.writeIndent();
            ts << "(";
            switch (createdLayer.type) {
            case PlatformCALayer::LayerTypeLayer:
            case PlatformCALayer::LayerTypeWebLayer:
            case PlatformCALayer::LayerTypeSimpleLayer:
                ts << "layer";
                break;
            case PlatformCALayer::LayerTypeTransformLayer:
                ts << "transform-layer";
                break;
            case PlatformCALayer::LayerTypeWebTiledLayer:
                ts << "tiled-layer";
                break;
            case PlatformCALayer::LayerTypeTiledBackingLayer:
                ts << "tiled-backing-layer";
                break;
            case PlatformCALayer::LayerTypePageTiledBackingLayer:
                ts << "page-tiled-backing-layer";
                break;
            case PlatformCALayer::LayerTypeTiledBackingTileLayer:
                ts << "tiled-backing-tile";
                break;
            case PlatformCALayer::LayerTypeRootLayer:
                ts << "root-layer";
                break;
            case PlatformCALayer::LayerTypeAVPlayerLayer:
                ts << "av-player-layer";
                break;
            case PlatformCALayer::LayerTypeCustom:
                ts << "custom-layer";
                break;
            }
            ts << " " << createdLayer.layerID << ")";
        }
        ts << ")";
        ts.decreaseIndent();
    }

    dumpChangedLayers(ts, m_changedLayerProperties);

    if (!m_destroyedLayerIDs.isEmpty())
        dumpProperty<Vector<RemoteLayerTreeTransaction::LayerID>>(ts, "destroyed-layers", m_destroyedLayerIDs);

    ts << ")\n";

    fprintf(stderr, "%s", ts.release().utf8().data());
}

#endif // NDEBUG

} // namespace WebKit
