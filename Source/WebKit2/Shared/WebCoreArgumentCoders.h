/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef WebCoreArgumentCoders_h
#define WebCoreArgumentCoders_h

#include "ArgumentCoders.h"

namespace WebCore {
class AffineTransform;
class AuthenticationChallenge;
class BlobPart;
class CertificateInfo;
class Color;
class Credential;
class CubicBezierTimingFunction;
class Cursor;
class DatabaseDetails;
class FilterOperation;
class FilterOperations;
class FloatPoint;
class FloatPoint3D;
class FloatRect;
class FloatSize;
class FixedPositionViewportConstraints;
class HTTPHeaderMap;
class IDBKeyPath;
class IntPoint;
class IntRect;
class IntSize;
class KeyframeValueList;
class LinearTimingFunction;
class Notification;
class ProtectionSpace;
class Region;
class ResourceError;
class ResourceRequest;
class ResourceResponse;
class SessionID;
class StepsTimingFunction;
class StickyPositionViewportConstraints;
class TextCheckingRequestData;
class TransformationMatrix;
class UserStyleSheet;
class UserScript;
class URL;
struct CompositionUnderline;
struct Cookie;
struct DictationAlternative;
struct FileChooserSettings;
struct IDBDatabaseMetadata;
struct IDBGetResult;
struct IDBIndexMetadata;
struct IDBKeyData;
struct IDBKeyRangeData;
struct IDBObjectStoreMetadata;
struct Length;
struct GrammarDetail;
struct MimeClassInfo;
struct PasteboardImage;
struct PasteboardWebContent;
struct PluginInfo;
struct ScrollableAreaParameters;
struct TextCheckingResult;
struct ViewportAttributes;
struct WindowFeatures;
}

#if PLATFORM(COCOA)
namespace WebCore {
struct KeypressCommand;
}
#endif

#if PLATFORM(IOS)
namespace WebCore {
class FloatQuad;
class SelectionRect;
struct Highlight;
struct PasteboardImage;
struct PasteboardWebContent;
struct ViewportArguments;
}
#endif

#if ENABLE(CONTENT_FILTERING)
namespace WebCore {
class ContentFilter;
}
#endif

namespace IPC {

template<> struct ArgumentCoder<WebCore::AffineTransform> {
    static void encode(ArgumentEncoder&, const WebCore::AffineTransform&);
    static bool decode(ArgumentDecoder&, WebCore::AffineTransform&);
};

template<> struct ArgumentCoder<WebCore::TransformationMatrix> {
    static void encode(ArgumentEncoder&, const WebCore::TransformationMatrix&);
    static bool decode(ArgumentDecoder&, WebCore::TransformationMatrix&);
};

template<> struct ArgumentCoder<WebCore::LinearTimingFunction> {
    static void encode(ArgumentEncoder&, const WebCore::LinearTimingFunction&);
    static bool decode(ArgumentDecoder&, WebCore::LinearTimingFunction&);
};

template<> struct ArgumentCoder<WebCore::CubicBezierTimingFunction> {
    static void encode(ArgumentEncoder&, const WebCore::CubicBezierTimingFunction&);
    static bool decode(ArgumentDecoder&, WebCore::CubicBezierTimingFunction&);
};

template<> struct ArgumentCoder<WebCore::StepsTimingFunction> {
    static void encode(ArgumentEncoder&, const WebCore::StepsTimingFunction&);
    static bool decode(ArgumentDecoder&, WebCore::StepsTimingFunction&);
};

template<> struct ArgumentCoder<WebCore::CertificateInfo> {
    static void encode(ArgumentEncoder&, const WebCore::CertificateInfo&);
    static bool decode(ArgumentDecoder&, WebCore::CertificateInfo&);
};

template<> struct ArgumentCoder<WebCore::FloatPoint> {
    static void encode(ArgumentEncoder&, const WebCore::FloatPoint&);
    static bool decode(ArgumentDecoder&, WebCore::FloatPoint&);
};

template<> struct ArgumentCoder<WebCore::FloatPoint3D> {
    static void encode(ArgumentEncoder&, const WebCore::FloatPoint3D&);
    static bool decode(ArgumentDecoder&, WebCore::FloatPoint3D&);
};

template<> struct ArgumentCoder<WebCore::FloatRect> {
    static void encode(ArgumentEncoder&, const WebCore::FloatRect&);
    static bool decode(ArgumentDecoder&, WebCore::FloatRect&);
};

template<> struct ArgumentCoder<WebCore::FloatSize> {
    static void encode(ArgumentEncoder&, const WebCore::FloatSize&);
    static bool decode(ArgumentDecoder&, WebCore::FloatSize&);
};

#if PLATFORM(IOS)
template<> struct ArgumentCoder<WebCore::FloatQuad> {
    static void encode(ArgumentEncoder&, const WebCore::FloatQuad&);
    static bool decode(ArgumentDecoder&, WebCore::FloatQuad&);
};

template<> struct ArgumentCoder<WebCore::ViewportArguments> {
    static void encode(ArgumentEncoder&, const WebCore::ViewportArguments&);
    static bool decode(ArgumentDecoder&, WebCore::ViewportArguments&);
};
#endif // PLATFORM(IOS)

template<> struct ArgumentCoder<WebCore::IntPoint> {
    static void encode(ArgumentEncoder&, const WebCore::IntPoint&);
    static bool decode(ArgumentDecoder&, WebCore::IntPoint&);
};

template<> struct ArgumentCoder<WebCore::IntRect> {
    static void encode(ArgumentEncoder&, const WebCore::IntRect&);
    static bool decode(ArgumentDecoder&, WebCore::IntRect&);
};

template<> struct ArgumentCoder<WebCore::IntSize> {
    static void encode(ArgumentEncoder&, const WebCore::IntSize&);
    static bool decode(ArgumentDecoder&, WebCore::IntSize&);
};

template<> struct ArgumentCoder<WebCore::Region> {
    static void encode(ArgumentEncoder&, const WebCore::Region&);
    static bool decode(ArgumentDecoder&, WebCore::Region&);
};

template<> struct ArgumentCoder<WebCore::Length> {
    static void encode(ArgumentEncoder&, const WebCore::Length&);
    static bool decode(ArgumentDecoder&, WebCore::Length&);
};

template<> struct ArgumentCoder<WebCore::ViewportAttributes> {
    static void encode(ArgumentEncoder&, const WebCore::ViewportAttributes&);
    static bool decode(ArgumentDecoder&, WebCore::ViewportAttributes&);
};

template<> struct ArgumentCoder<WebCore::MimeClassInfo> {
    static void encode(ArgumentEncoder&, const WebCore::MimeClassInfo&);
    static bool decode(ArgumentDecoder&, WebCore::MimeClassInfo&);
};

template<> struct ArgumentCoder<WebCore::PluginInfo> {
    static void encode(ArgumentEncoder&, const WebCore::PluginInfo&);
    static bool decode(ArgumentDecoder&, WebCore::PluginInfo&);
};

template<> struct ArgumentCoder<WebCore::HTTPHeaderMap> {
    static void encode(ArgumentEncoder&, const WebCore::HTTPHeaderMap&);
    static bool decode(ArgumentDecoder&, WebCore::HTTPHeaderMap&);
};

template<> struct ArgumentCoder<WebCore::AuthenticationChallenge> {
    static void encode(ArgumentEncoder&, const WebCore::AuthenticationChallenge&);
    static bool decode(ArgumentDecoder&, WebCore::AuthenticationChallenge&);
};

template<> struct ArgumentCoder<WebCore::ProtectionSpace> {
    static void encode(ArgumentEncoder&, const WebCore::ProtectionSpace&);
    static bool decode(ArgumentDecoder&, WebCore::ProtectionSpace&);
    static void encodePlatformData(ArgumentEncoder&, const WebCore::ProtectionSpace&);
    static bool decodePlatformData(ArgumentDecoder&, WebCore::ProtectionSpace&);
};

template<> struct ArgumentCoder<WebCore::Credential> {
    static void encode(ArgumentEncoder&, const WebCore::Credential&);
    static bool decode(ArgumentDecoder&, WebCore::Credential&);
    static void encodePlatformData(ArgumentEncoder&, const WebCore::Credential&);
    static bool decodePlatformData(ArgumentDecoder&, WebCore::Credential&);
};

template<> struct ArgumentCoder<WebCore::Cursor> {
    static void encode(ArgumentEncoder&, const WebCore::Cursor&);
    static bool decode(ArgumentDecoder&, WebCore::Cursor&);
};

template<> struct ArgumentCoder<WebCore::ResourceRequest> {
    static void encode(ArgumentEncoder&, const WebCore::ResourceRequest&);
    static bool decode(ArgumentDecoder&, WebCore::ResourceRequest&);
    static void encodePlatformData(ArgumentEncoder&, const WebCore::ResourceRequest&);
    static bool decodePlatformData(ArgumentDecoder&, WebCore::ResourceRequest&);
};

template<> struct ArgumentCoder<WebCore::ResourceError> {
    static void encode(ArgumentEncoder&, const WebCore::ResourceError&);
    static bool decode(ArgumentDecoder&, WebCore::ResourceError&);
    static void encodePlatformData(ArgumentEncoder&, const WebCore::ResourceError&);
    static bool decodePlatformData(ArgumentDecoder&, WebCore::ResourceError&);
};

template<> struct ArgumentCoder<WebCore::WindowFeatures> {
    static void encode(ArgumentEncoder&, const WebCore::WindowFeatures&);
    static bool decode(ArgumentDecoder&, WebCore::WindowFeatures&);
};

template<> struct ArgumentCoder<WebCore::Color> {
    static void encode(ArgumentEncoder&, const WebCore::Color&);
    static bool decode(ArgumentDecoder&, WebCore::Color&);
};

#if PLATFORM(COCOA)
template<> struct ArgumentCoder<WebCore::KeypressCommand> {
    static void encode(ArgumentEncoder&, const WebCore::KeypressCommand&);
    static bool decode(ArgumentDecoder&, WebCore::KeypressCommand&);
};
#endif

#if PLATFORM(IOS)
template<> struct ArgumentCoder<WebCore::SelectionRect> {
    static void encode(ArgumentEncoder&, const WebCore::SelectionRect&);
    static bool decode(ArgumentDecoder&, WebCore::SelectionRect&);
};

template<> struct ArgumentCoder<WebCore::Highlight> {
    static void encode(ArgumentEncoder&, const WebCore::Highlight&);
    static bool decode(ArgumentDecoder&, WebCore::Highlight&);
};

template<> struct ArgumentCoder<WebCore::PasteboardWebContent> {
    static void encode(ArgumentEncoder&, const WebCore::PasteboardWebContent&);
    static bool decode(ArgumentDecoder&, WebCore::PasteboardWebContent&);
};

template<> struct ArgumentCoder<WebCore::PasteboardImage> {
    static void encode(ArgumentEncoder&, const WebCore::PasteboardImage&);
    static bool decode(ArgumentDecoder&, WebCore::PasteboardImage&);
};
#endif

template<> struct ArgumentCoder<WebCore::CompositionUnderline> {
    static void encode(ArgumentEncoder&, const WebCore::CompositionUnderline&);
    static bool decode(ArgumentDecoder&, WebCore::CompositionUnderline&);
};

template<> struct ArgumentCoder<WebCore::Cookie> {
    static void encode(ArgumentEncoder&, const WebCore::Cookie&);
    static bool decode(ArgumentDecoder&, WebCore::Cookie&);
};

template<> struct ArgumentCoder<WebCore::DatabaseDetails> {
    static void encode(ArgumentEncoder&, const WebCore::DatabaseDetails&);
    static bool decode(ArgumentDecoder&, WebCore::DatabaseDetails&);
};

template<> struct ArgumentCoder<WebCore::DictationAlternative> {
    static void encode(ArgumentEncoder&, const WebCore::DictationAlternative&);
    static bool decode(ArgumentDecoder&, WebCore::DictationAlternative&);
};

template<> struct ArgumentCoder<WebCore::FileChooserSettings> {
    static void encode(ArgumentEncoder&, const WebCore::FileChooserSettings&);
    static bool decode(ArgumentDecoder&, WebCore::FileChooserSettings&);
};

template<> struct ArgumentCoder<WebCore::GrammarDetail> {
    static void encode(ArgumentEncoder&, const WebCore::GrammarDetail&);
    static bool decode(ArgumentDecoder&, WebCore::GrammarDetail&);
};

template<> struct ArgumentCoder<WebCore::TextCheckingRequestData> {
    static void encode(ArgumentEncoder&, const WebCore::TextCheckingRequestData&);
    static bool decode(ArgumentDecoder&, WebCore::TextCheckingRequestData&);
};

template<> struct ArgumentCoder<WebCore::TextCheckingResult> {
    static void encode(ArgumentEncoder&, const WebCore::TextCheckingResult&);
    static bool decode(ArgumentDecoder&, WebCore::TextCheckingResult&);
};
    
template<> struct ArgumentCoder<WebCore::URL> {
    static void encode(ArgumentEncoder&, const WebCore::URL&);
    static bool decode(ArgumentDecoder&, WebCore::URL&);
};

template<> struct ArgumentCoder<WebCore::UserStyleSheet> {
    static void encode(ArgumentEncoder&, const WebCore::UserStyleSheet&);
    static bool decode(ArgumentDecoder&, WebCore::UserStyleSheet&);
};

template<> struct ArgumentCoder<WebCore::UserScript> {
    static void encode(ArgumentEncoder&, const WebCore::UserScript&);
    static bool decode(ArgumentDecoder&, WebCore::UserScript&);
};

template<> struct ArgumentCoder<WebCore::ScrollableAreaParameters> {
    static void encode(ArgumentEncoder&, const WebCore::ScrollableAreaParameters&);
    static bool decode(ArgumentDecoder&, WebCore::ScrollableAreaParameters&);
};

template<> struct ArgumentCoder<WebCore::FixedPositionViewportConstraints> {
    static void encode(ArgumentEncoder&, const WebCore::FixedPositionViewportConstraints&);
    static bool decode(ArgumentDecoder&, WebCore::FixedPositionViewportConstraints&);
};

template<> struct ArgumentCoder<WebCore::StickyPositionViewportConstraints> {
    static void encode(ArgumentEncoder&, const WebCore::StickyPositionViewportConstraints&);
    static bool decode(ArgumentDecoder&, WebCore::StickyPositionViewportConstraints&);
};

#if !USE(COORDINATED_GRAPHICS)
template<> struct ArgumentCoder<WebCore::FilterOperations> {
    static void encode(ArgumentEncoder&, const WebCore::FilterOperations&);
    static bool decode(ArgumentDecoder&, WebCore::FilterOperations&);
};
    
template<> struct ArgumentCoder<WebCore::FilterOperation> {
    static void encode(ArgumentEncoder&, const WebCore::FilterOperation&);
};
bool decodeFilterOperation(ArgumentDecoder&, RefPtr<WebCore::FilterOperation>&);
#endif

#if ENABLE(INDEXED_DATABASE)
template<> struct ArgumentCoder<WebCore::IDBDatabaseMetadata> {
    static void encode(ArgumentEncoder&, const WebCore::IDBDatabaseMetadata&);
    static bool decode(ArgumentDecoder&, WebCore::IDBDatabaseMetadata&);
};

template<> struct ArgumentCoder<WebCore::IDBGetResult> {
    static void encode(ArgumentEncoder&, const WebCore::IDBGetResult&);
    static bool decode(ArgumentDecoder&, WebCore::IDBGetResult&);
};

template<> struct ArgumentCoder<WebCore::IDBIndexMetadata> {
    static void encode(ArgumentEncoder&, const WebCore::IDBIndexMetadata&);
    static bool decode(ArgumentDecoder&, WebCore::IDBIndexMetadata&);
};

template<> struct ArgumentCoder<WebCore::IDBKeyData> {
    static void encode(ArgumentEncoder&, const WebCore::IDBKeyData&);
    static bool decode(ArgumentDecoder&, WebCore::IDBKeyData&);
};

template<> struct ArgumentCoder<WebCore::IDBKeyPath> {
    static void encode(ArgumentEncoder&, const WebCore::IDBKeyPath&);
    static bool decode(ArgumentDecoder&, WebCore::IDBKeyPath&);
};

template<> struct ArgumentCoder<WebCore::IDBKeyRangeData> {
    static void encode(ArgumentEncoder&, const WebCore::IDBKeyRangeData&);
    static bool decode(ArgumentDecoder&, WebCore::IDBKeyRangeData&);
};

template<> struct ArgumentCoder<WebCore::IDBObjectStoreMetadata> {
    static void encode(ArgumentEncoder&, const WebCore::IDBObjectStoreMetadata&);
    static bool decode(ArgumentDecoder&, WebCore::IDBObjectStoreMetadata&);
};

#endif // ENABLE(INDEXED_DATABASE)

template<> struct ArgumentCoder<WebCore::SessionID> {
    static void encode(ArgumentEncoder&, const WebCore::SessionID&);
    static bool decode(ArgumentDecoder&, WebCore::SessionID&);
};

template<> struct ArgumentCoder<WebCore::BlobPart> {
    static void encode(ArgumentEncoder&, const WebCore::BlobPart&);
    static bool decode(ArgumentDecoder&, WebCore::BlobPart&);
};

#if ENABLE(CONTENT_FILTERING)
template<> struct ArgumentCoder<WebCore::ContentFilter> {
    static void encode(ArgumentEncoder&, const WebCore::ContentFilter&);
    static bool decode(ArgumentDecoder&, WebCore::ContentFilter&);
};
#endif

} // namespace IPC

#endif // WebCoreArgumentCoders_h
