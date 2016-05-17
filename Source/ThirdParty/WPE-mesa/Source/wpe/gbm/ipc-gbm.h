#ifndef wpe_ipc_gbm_h
#define wpe_ipc_gbm_h

#define MESSAGE_SIZE 32

#define DATA_OFFSET sizeof(uint64_t)
#define DATA_SIZE 24

#include <stdint.h>

struct ipc_gbm_message {
    uint64_t message_code;
    char data[DATA_SIZE];
};
static_assert(sizeof(struct ipc_gbm_message) == MESSAGE_SIZE, "ipc_gbm_message is of correct size");

struct buffer_commit {
    uint32_t handle;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
    uint32_t padding;
};
static_assert(sizeof(struct buffer_commit) == DATA_SIZE, "buffer_commit is of correct size");

struct frame_complete {
    char data[24];
};
static_assert(sizeof(struct frame_complete) == DATA_SIZE, "frame_complete is of correct size");

struct release_buffer {
    uint32_t handle;
    char data[20];
};
static_assert(sizeof(struct release_buffer) == DATA_SIZE, "release_buffer is of correct size");


#endif // wpe_ipc_gbm_h
