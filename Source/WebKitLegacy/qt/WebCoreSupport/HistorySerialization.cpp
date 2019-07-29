/*
 * Copyright (C) 2005, 2006, 2008, 2011, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2016 Konstantin Tokarev <annulen@yandex.ru>
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

#include "HistorySerialization.h"

#include <WebCore/FormData.h>
#include <WebCore/KeyedDecoderQt.h>
#include <WebCore/KeyedEncoderQt.h>

namespace WebCore {

enum class FormDataType {
    Data,
    EncodedFile,
    EncodedBlob,
};

static void encodeElement(KeyedEncoder& encoder, const FormDataElement& element)
{
    switchOn(element.data,
        [&] (const Vector<char>& bytes) {
            encoder.encodeEnum("type", FormDataType::Data);
            encoder.encodeBytes("data", reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
        }, [&] (const FormDataElement::EncodedFileData& fileData) {
            encoder.encodeEnum("type", FormDataType::EncodedFile);
            encoder.encodeString("filename", fileData.filename);
            encoder.encodeString("generatedFilename", fileData.generatedFilename);
            encoder.encodeBool("shouldGenerateFile", fileData.shouldGenerateFile);
            encoder.encodeInt64("fileStart", fileData.fileStart);
            encoder.encodeInt64("fileLength", fileData.fileLength);
            if (fileData.expectedFileModificationTime)
                encoder.encodeDouble("expectedFileModificationTime", fileData.expectedFileModificationTime.value().secondsSinceEpoch().seconds());
        }, [&] (const FormDataElement::EncodedBlobData& blobData) {
            encoder.encodeEnum("type", FormDataType::EncodedBlob);
            encoder.encodeString("url", blobData.url.string());
        });
}

static bool decodeElement(KeyedDecoder& decoder, FormDataElement& element)
{
    FormDataType type;
    if (!decoder.decodeEnum("type", type, [](FormDataType type) {
        switch (type) {
        case FormDataType::Data:
        case FormDataType::EncodedFile:
        case FormDataType::EncodedBlob:
            return true;
        }

        return false;
    }))
        return false;

    switch (type) {
    case FormDataType::Data: {
        Vector<char> bytes;
        if (!decoder.decodeBytes("data", bytes))
            return false;
        element.data = WTFMove(bytes);
        break;
    }
    case FormDataType::EncodedFile: {
        FormDataElement::EncodedFileData fileData;
        if (!decoder.decodeString("filename", fileData.filename))
            return false;
        if (!decoder.decodeString("generatedFilename", fileData.generatedFilename))
            return false;
        if (!decoder.decodeBool("shouldGenerateFile", fileData.shouldGenerateFile))
            return false;

        int64_t fileStart;
        if (!decoder.decodeInt64("fileStart", fileStart))
            return false;
        if (fileStart < 0)
            return false;

        int64_t fileLength;
        if (!decoder.decodeInt64("fileLength", fileLength))
            return false;
        if (fileLength != BlobDataItem::toEndOfFile && fileLength < fileStart)
            return false;

        fileData.fileStart = fileStart;
        fileData.fileLength = fileLength;

        double expectedFileModificationTime; // Optional field
        if (decoder.decodeDouble("expectedFileModificationTime", expectedFileModificationTime))
            fileData.expectedFileModificationTime = WallTime::fromRawSeconds(expectedFileModificationTime);

        element.data = WTFMove(fileData);
        break;
    }

    case FormDataType::EncodedBlob: {
        String blobURLString;
        if (!decoder.decodeString("url", blobURLString))
            return false;

        element.data = FormDataElement::EncodedBlobData { URL(URL(), blobURLString) };
        break;
    }
    }

    return true;
}

void encodeFormData(const FormData& formData, KeyedEncoder& encoder)
{
    encoder.encodeBool("alwaysStream", formData.alwaysStream());
    encoder.encodeBytes("boundary", reinterpret_cast<const uint8_t*>(formData.boundary().data()), formData.boundary().size());

    encoder.encodeObjects("elements", formData.elements().begin(), formData.elements().end(), [](KeyedEncoder& encoder, const FormDataElement& element) {
        encodeElement(encoder, element);
    });

    encoder.encodeInt64("identifier", formData.identifier());
}

RefPtr<FormData> decodeFormData(KeyedDecoder& decoder)
{
    RefPtr<FormData> data = FormData::create();

    bool alwaysStream;
    if (!decoder.decodeBool("alwaysStream", alwaysStream))
        return nullptr;
    data->setAlwaysStream(alwaysStream);

    if (!decoder.decodeBytes("boundary", const_cast<Vector<char>&>(data->boundary())))
        return nullptr;

    if (!decoder.decodeObjects("elements", const_cast<Vector<FormDataElement>&>(data->elements()), [](KeyedDecoder& decoder, FormDataElement& element) {
        return decodeElement(decoder, element);
    }))
        return nullptr;

    int64_t identifier;
    if (!decoder.decodeInt64("identifier", identifier))
        return nullptr;

    data->setIdentifier(identifier);
    return data;
}

static void encodeBackForwardTreeNode(KeyedEncoder& encoder, const HistoryItem& item)
{
    encoder.encodeObjects("children", item.children().begin(), item.children().end(),
        encodeBackForwardTreeNode);

    encoder.encodeString("originalURLString", item.originalURLString());
    encoder.encodeString("urlString", item.urlString());

    encoder.encodeInt64("documentSequenceNumber", item.documentSequenceNumber());

    encoder.encodeObjects("documentState", item.documentState().begin(), item.documentState().end(), [](KeyedEncoder& encoder, const String& string) {
        encoder.encodeString("string", string);
    });

    encoder.encodeString("formContentType", item.formContentType());

    encoder.encodeConditionalObject("formData", const_cast<HistoryItem&>(item).formData(), [](KeyedEncoder& encoder, const FormData& formData) {
        encodeFormData(formData, encoder);
    });

    encoder.encodeInt64("itemSequenceNumber", item.itemSequenceNumber());

    encoder.encodeString("referrer", item.referrer());

    encoder.encodeObject("scrollPosition", item.scrollPosition(), [](KeyedEncoder& encoder, const IntPoint& scrollPosition) {
        encoder.encodeInt32("x", scrollPosition.x());
        encoder.encodeInt32("y", scrollPosition.y());
    });

    encoder.encodeFloat("pageScaleFactor", item.pageScaleFactor());

    encoder.encodeConditionalObject("stateObject", item.stateObject(), [](KeyedEncoder& encoder, const SerializedScriptValue& stateObject) {
        encoder.encodeBytes("data", stateObject.data().data(), stateObject.data().size());
    });

    encoder.encodeString("target", item.target());
}

void encodeBackForwardTree(KeyedEncoderQt& encoder, const HistoryItem& item)
{
    encoder.encodeString("title", item.title());
    encodeBackForwardTreeNode(encoder, item);
    if (item.userData().isValid())
        encoder.encodeVariant("userData", item.userData());
}

template<typename F>
bool decodeString(KeyedDecoder& decoder, const String& key, F&& consumer)
{
    String tmp;
    if (!decoder.decodeString(key, tmp))
        return false;
    consumer(WTFMove(tmp));
    return true;
}

static bool decodeBackForwardTreeNode(KeyedDecoder& decoder, HistoryItem& item)
{
    if (!decodeString(decoder, "urlString", [&item](String&& str) {
        item.setURLString(str);
    }))
        return false;

    decodeString(decoder, "originalURLString", [&item](String&& str) {
        item.setOriginalURLString(str);
    });

    Vector<int> ignore;
    decoder.decodeObjects("children", ignore, [&item](KeyedDecoder& decoder, int&) {
        Ref<HistoryItem> element = HistoryItem::create();
        if (decodeBackForwardTreeNode(decoder, element)) {
            item.addChildItem(WTFMove(element));
            return true;
        }
        return false;
    });

    int64_t documentSequenceNumber;
    if (decoder.decodeInt64("documentSequenceNumber", documentSequenceNumber))
        item.setDocumentSequenceNumber(documentSequenceNumber);

    Vector<String> documentState;
    decoder.decodeObjects("documentState", documentState, [](KeyedDecoder& decoder, String& string) -> bool {
        return decoder.decodeString("string", string);
    });
    item.setDocumentState(documentState);

    decodeString(decoder, "formContentType", [&item](String&& str) {
        item.setFormContentType(str);
    });

    RefPtr<FormData> formData;
    if (decoder.decodeObject("formData", formData, [](KeyedDecoder& decoder, RefPtr<FormData>& formData) {
        formData = decodeFormData(decoder);
        return formData != nullptr;
    }))
        item.setFormData(WTFMove(formData));

    int64_t itemSequenceNumber;
    if (decoder.decodeInt64("itemSequenceNumber", itemSequenceNumber))
        item.setItemSequenceNumber(itemSequenceNumber);

    decodeString(decoder, "referrer", [&item](String&& str) {
        item.setReferrer(str);
    });

    int ignore2;
    decoder.decodeObject("scrollPosition", ignore2, [&item](KeyedDecoder& decoder, int&) -> bool {
        int x, y;
        if (!decoder.decodeInt32("x", x))
            return false;
        if (!decoder.decodeInt32("y", y))
            return false;
        item.setScrollPosition(IntPoint(x, y));
        return true;
    });

    float pageScaleFactor;
    if (decoder.decodeFloat("pageScaleFactor", pageScaleFactor))
        item.setPageScaleFactor(pageScaleFactor);

    RefPtr<SerializedScriptValue> stateObject;
    decoder.decodeConditionalObject("stateObject", stateObject, [](KeyedDecoder& decoder, RefPtr<SerializedScriptValue>& stateObject) -> bool {
        Vector<uint8_t> bytes;
        if (decoder.decodeBytes("data", bytes)) {
            stateObject = SerializedScriptValue::adopt(WTFMove(bytes));
            return true;
        }
        return false;
    });

    decodeString(decoder, "target", [&item](String&& str) {
        item.setTarget(str);
    });

    return true;
}

bool decodeBackForwardTree(KeyedDecoderQt& decoder, HistoryItem& item)
{
    if (!decodeString(decoder, "title", [&item](String&& str) {
        item.setTitle(str);
    }))
        return false;

    QVariant userData;
    if (decoder.decodeVariant("userData", userData))
        item.setUserData(userData);

    return decodeBackForwardTreeNode(decoder, item);
}

}
