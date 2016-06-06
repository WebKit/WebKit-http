#ifndef wpe_mesa_ipc_gbm_h
#define wpe_mesa_ipc_gbm_h

#define MESSAGE_SIZE 32

#define DATA_OFFSET sizeof(uint64_t)
#define DATA_SIZE 24

#include <memory>
#include <stdint.h>

namespace IPC {

namespace GBM {

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
    uint32_t handle;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
    uint8_t padding[4];

    static const uint64_t code = 42;
    static BufferCommit& construct(Message& message, uint32_t handle, uint32_t width, uint32_t height, uint32_t stride, uint32_t format)
    {
        message.messageCode = code;

        auto& messageData = *reinterpret_cast<BufferCommit*>(std::addressof(message.data));
        messageData.handle = handle;
        messageData.width = width;
        messageData.height = height;
        messageData.stride = stride;
        messageData.format = format;

        return messageData;
    }
    static BufferCommit& cast(Message& message)
    {
        return *reinterpret_cast<BufferCommit*>(message.data);
    }
};
static_assert(sizeof(BufferCommit) == messageDataSize, "BufferCommit is of correct size");

struct FrameComplete {
    uint8_t data[24];

    static const uint64_t code = 23;
    static FrameComplete& construct(Message& message)
    {
        message.messageCode = code;
        return *reinterpret_cast<FrameComplete*>(std::addressof(message.data));
    }
};
static_assert(sizeof(FrameComplete) == messageDataSize, "FrameComplete is of correct size");

struct ReleaseBuffer {
    uint32_t handle;
    uint8_t data[20];

    static const uint64_t code = 16;
    static ReleaseBuffer& construct(Message& message, uint32_t handle)
    {
        message.messageCode = code;

        auto& messageData = *reinterpret_cast<ReleaseBuffer*>(std::addressof(message.data));
        messageData.handle = handle;
        return messageData;
    }
    static ReleaseBuffer& cast(Message& message)
    {
        return *reinterpret_cast<ReleaseBuffer*>(message.data);
    }
};
static_assert(sizeof(ReleaseBuffer) == messageDataSize, "ReleaseBuffer is of correct size");

} // namespace GBM

} // namespace IPC

#endif // wpe_mesa_ipc_gbm_h
