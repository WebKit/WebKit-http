#ifndef wpe_metrological_ipc_bcmnexus_h
#define wpe_metrological_ipc_bcmnexus_h

#include <memory>

namespace IPC {

namespace BCMNexus {

static const size_t messageSize = 32;
static const size_t messageDataSize = 24;

struct Message {
    uint64_t messageCode { 0 };
    uint8_t data[messageDataSize] { 0, };
};
static_assert(sizeof(Message) == messageSize, "Message is of correct size");

static char* messageData(Message& message)
{
    return reinterpret_cast<char*>(std::addressof(message));
}

static Message& asMessage(char* data)
{
    return *reinterpret_cast<Message*>(data);
}

struct BufferCommit {
    uint32_t width;
    uint32_t height;
    uint8_t padding[16];

    static const uint64_t code = 1;
    static BufferCommit& construct(Message& message, uint32_t width, uint32_t height)
    {
        message.messageCode = code;

        auto& messageData = *reinterpret_cast<BufferCommit*>(std::addressof(message.data));
        messageData.width = width;
        messageData.height = height;

        return messageData;
    }
    static BufferCommit& cast(Message& message)
    {
        return *reinterpret_cast<BufferCommit*>(message.data);
    }
};
static_assert(sizeof(BufferCommit) == messageDataSize, "BufferCommit is of correct size");

struct FrameComplete {
    int8_t padding[24];

    static const uint64_t code = 2;
    static FrameComplete& construct(Message& message)
    {
        message.messageCode = code;
        return *reinterpret_cast<FrameComplete*>(std::addressof(message.data));
    }
    static FrameComplete& cast(Message& message)
    {
        return *reinterpret_cast<FrameComplete*>(message.data);
    }
};
static_assert(sizeof(FrameComplete) == messageDataSize, "FrameComplete is of correct size");

} // namespace BCMNexus

} // namespace IPC

#endif // wpe_metrological_ipc_bcmnexus_h
