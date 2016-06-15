#ifndef wpe_metrological_ipc_rpi_h
#define wpe_metrological_ipc_rpi_h

#include <memory>
#include <stdint.h>

namespace IPC {

namespace BCMRPi {

struct TargetConstruction {
    uint32_t handle;
    uint32_t width;
    uint32_t height;
    uint8_t padding[12];

    static const uint64_t code = 1;
    static void construct(Message& message, uint32_t handle, uint32_t width, uint32_t height)
    {
        message.messageCode = code;

        auto& messageData = *reinterpret_cast<TargetConstruction*>(std::addressof(message.messageData));
        messageData.handle = handle;
        messageData.width = width;
        messageData.height = height;
    }
    static TargetConstruction& cast(Message& message)
    {
        return *reinterpret_cast<TargetConstruction*>(std::addressof(message.messageData));
    }
};
static_assert(sizeof(TargetConstruction) == Message::dataSize, "TargetConstruction is of correct size");

struct BufferCommit {
    uint32_t handle;
    uint32_t width;
    uint32_t height;
    uint8_t padding[12];

    static const uint64_t code = 2;
    static void construct(Message& message, uint32_t handle, uint32_t width, uint32_t height)
    {
        message.messageCode = code;

        auto& messageData = *reinterpret_cast<BufferCommit*>(std::addressof(message.messageData));
        messageData.handle = handle;
        messageData.width = width;
        messageData.height = height;
    }
    static BufferCommit& cast(Message& message)
    {
        return *reinterpret_cast<BufferCommit*>(std::addressof(message.messageData));
    }
};
static_assert(sizeof(BufferCommit) == Message::dataSize, "BufferCommit is of correct size");

struct FrameComplete {
    int8_t padding[24];

    static const uint64_t code = 3;
    static void construct(Message& message)
    {
        message.messageCode = code;
    }
    static FrameComplete& cast(Message& message)
    {
        return *reinterpret_cast<FrameComplete*>(std::addressof(message.messageData));
    }
};
static_assert(sizeof(FrameComplete) == Message::dataSize, "FrameComplete is of correct size");

} // namespace BCMRPi

} // namespace IPC

#endif // wpe_metrological_ipc_rpi_h
