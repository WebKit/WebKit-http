#pragma once

#include <wtf/Forward.h>

namespace ASN1 {

enum class Tag : uint8_t {
    Integer = 0x02,
    BitString = 0x03,
    OctetString = 0x04,
    ObjectIdentifier = 0x06,
    Sequence = 0x30,
};

struct TLV {
    size_t bufferSize { 0 };
    size_t bufferOffset { 0 };

    uint8_t tag { 0 };
    size_t length { 0 };
    size_t offset { 0 };

    size_t valueOffset() const { return bufferOffset + offset; }
    bool hasTag(Tag targetTag) const { return tag == static_cast<uint8_t>(targetTag); }
};

static TLV parseTLV(const Vector<uint8_t>& buffer, const size_t offset)
{
    if (offset + 2 > buffer.size())
        return { };

    uint8_t tag = buffer[offset];
    size_t length = buffer[offset + 1];
    size_t lengthBytes = 1;

    if (length > 128) {
        lengthBytes = length - 128;
        if (offset + 2 + lengthBytes > buffer.size())
            return { };

        length = 0;
        for (size_t i = 0; i < lengthBytes; ++i)
            length += buffer[offset + 2 + i] << ((lengthBytes - i - 1) * 8);

        lengthBytes += 1;
    }

    if (offset + 1 + lengthBytes + length > buffer.size())
        return { };

    return { 1 + lengthBytes + length, offset, tag, length, 1 + lengthBytes };
}

struct Node;

struct Node {
    Node() = default;
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    size_t nodeOffset { 0 };
    size_t nodeSize { 0 };

    uint8_t tag { 0 };
    size_t length { 0 };
    size_t offset { 0 };

    bool operator!() const { return !tag; }
};

struct Parser {
    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    const Vector<uint8_t>& buffer;

    void parseNode(size_t offset, Tag targetTag, Node& node) {
        if (offset + 2 > buffer.size())
            return;

        uint8_t tag = buffer[offset];
        if (tag != static_cast<uint8_t>(targetTag))
            return;

        size_t length = buffer[offset + 1];
        size_t lengthBytes = 1;

        if (length > 128) {
            lengthBytes = length - 128;
            if (offset + 2 + lengthBytes > buffer.size())
                return;

            length = 0;
            for (size_t i = 0; i < lengthBytes; ++i)
                length += buffer[offset + 2 + i] << ((lengthBytes - i - 1) * 8);

            lengthBytes += 1;
        }

        if (offset + 1 + lengthBytes + length > buffer.size())
            return;

        node.nodeOffset = offset;
        node.nodeSize = 1 + lengthBytes + length;

        node.tag = tag;
        node.length = length;
        node.offset = 1 + lengthBytes;
    }
};

} // namespace ASN1
