#include "config.h"
#include "BeaconLoader.h"

#include "Document.h"
#include "File.h"
#include "runtime/ArrayBufferView.h"
#include "DOMFormData.h"
#include "ResourceRequest.h"
#include "SecurityOrigin.h"

namespace WebCore {

namespace {

class Beacon {
public:
    virtual bool serialize(ResourceRequest&, int, int&) const = 0;
    virtual unsigned long long size() const = 0;

protected:
    static unsigned long long beaconSize(Blob*);
    static unsigned long long beaconSize(ArrayBufferView*);
    static unsigned long long beaconSize(DOMFormData*);
    static unsigned long long beaconSize(const String&);

    static bool serialize(Blob*, ResourceRequest&, int, int&);
    static bool serialize(ArrayBufferView*, ResourceRequest&, int, int&);
    static bool serialize(DOMFormData*, ResourceRequest&, int, int&);
    static bool serialize(const String&, ResourceRequest&, int, int&);
};

template<typename Payload>
class BeaconData final : public Beacon {
public:
    BeaconData(const Payload& data)
        : m_data(data)
    {
    }

    bool serialize(ResourceRequest& request, int allowance, int& payloadLength) const override
    {
        return Beacon::serialize(m_data, request, allowance, payloadLength);
    }

    unsigned long long size() const override
    {
        return beaconSize(m_data);
    }

private:
    const Payload& m_data;
};

} // namespace

class BeaconLoader::Sender {
public:
    static bool send(Frame& frame, int allowance, const URL& beaconURL, const Beacon& beacon, int& payloadLength)
    {
        if (!frame.document())
            return false;

        unsigned long long entitySize = beacon.size();
        if (allowance > 0 && static_cast<unsigned long long>(allowance) < entitySize)
            return false;

        ResourceRequest request(beaconURL);
        request.setHTTPMethod("POST");
        request.setHTTPHeaderField(HTTPHeaderName::CacheControl, "max-age=0");
        SecurityOrigin& sourceOrigin = frame.document()->securityOrigin();
        FrameLoader::addHTTPOriginIfNeeded(request, sourceOrigin.toString());
        frame.loader().addExtraFieldsToSubresourceRequest(request);

        payloadLength = entitySize;
        if (!beacon.serialize(request, allowance, payloadLength))
            return false;

        // Leak the loader, since it will kill itself as soon as it receives a response.
        RefPtr<BeaconLoader> loader = adoptRef(new BeaconLoader(frame, request));
        loader->ref();
        return true;
    }
};

bool BeaconLoader::sendBeacon(Frame& frame, int allowance, const URL& beaconURL, Blob* data, int& payloadLength)
{
    BeaconData<decltype(data)> beacon(data);
    return Sender::send(frame, allowance, beaconURL, beacon, payloadLength);
}

bool BeaconLoader::sendBeacon(Frame& frame, int allowance, const URL& beaconURL, ArrayBufferView* data, int& payloadLength)
{
    BeaconData<decltype(data)> beacon(data);
    return Sender::send(frame, allowance, beaconURL, beacon, payloadLength);
}

bool BeaconLoader::sendBeacon(Frame& frame, int allowance, const URL& beaconURL, DOMFormData* data, int& payloadLength)
{
    BeaconData<decltype(data)> beacon(data);
    return Sender::send(frame, allowance, beaconURL, beacon, payloadLength);
}

bool BeaconLoader::sendBeacon(Frame& frame, int allowance, const URL& beaconURL, const String& data, int& payloadLength)
{
    BeaconData<decltype(data)> beacon(data);
    return Sender::send(frame, allowance, beaconURL, beacon, payloadLength);
}

BeaconLoader::BeaconLoader(Frame& frame, ResourceRequest& request)
{
    startPingLoad(frame, request, ShouldFollowRedirects::Yes);
}

namespace {

unsigned long long Beacon::beaconSize(Blob* data)
{
    return data->size();
}

bool Beacon::serialize(Blob* data, ResourceRequest& request, int, int&)
{
    ASSERT(data);
    RefPtr<FormData> entityBody = FormData::create();

    if (is<File>(*data))
        entityBody->appendFile(downcast<File>(*data).path());
    else
        entityBody->appendBlob(data->url());

    request.setHTTPBody(WTFMove(entityBody));
    const String& blobType = data->type();
    if (!blobType.isEmpty() && Blob::isValidContentType(blobType))
        request.setHTTPContentType(AtomicString(blobType));
    return true;
}

unsigned long long Beacon::beaconSize(ArrayBufferView* data)
{
    return data->byteLength();
}

bool Beacon::serialize(ArrayBufferView* data, ResourceRequest& request, int, int&)
{
    ASSERT(data);
    RefPtr<FormData> entityBody = FormData::create(data->baseAddress(), data->byteLength());

    request.setHTTPBody(WTFMove(entityBody));
    // FIXME: a reasonable choice, but not in the spec; should it give a default?
    AtomicString contentType = AtomicString("application/octet-stream");
    request.setHTTPContentType(contentType);

    return true;
}

unsigned long long Beacon::beaconSize(DOMFormData*)
{
    // FormData's size cannot be determined until serialized.
    return 0;
}

bool Beacon::serialize(DOMFormData* data, ResourceRequest& request, int allowance, int& payloadLength)
{
    ASSERT(data);
    RefPtr<FormData> entityBody = FormData::createMultiPart(*(static_cast<FormDataList*>(data)), data->encoding(), 0);
    unsigned long long entitySize = entityBody->sizeInBytes();

    if (allowance > 0 && static_cast<unsigned long long>(allowance) < entitySize)
        return false;

    AtomicString contentType = AtomicString("multipart/form-data; boundary=", AtomicString::ConstructFromLiteral) + entityBody->boundary().data();
    request.setHTTPBody(WTFMove(entityBody));
    request.setHTTPContentType(contentType);

    payloadLength = entitySize;
    return true;
}

unsigned long long Beacon::beaconSize(const String& data)
{
    return data.sizeInBytes();
}

bool Beacon::serialize(const String& data, ResourceRequest& request, int, int&)
{
    RefPtr<FormData> entityBody = FormData::create(data.utf8());

    request.setHTTPBody(WTFMove(entityBody));
    request.setHTTPContentType("text/plain;charset=UTF-8");
    return true;
}

} // namespace

} // namespace WebCore
