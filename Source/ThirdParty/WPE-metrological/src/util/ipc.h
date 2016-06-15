#ifndef wpe_mesa_ipc_h
#define wpe_mesa_ipc_h

#include <gio/gio.h>
#include <memory>
#include <stdint.h>

namespace IPC {

struct Message {
    static const size_t size = 32;
    static const size_t dataSize = 24;

    uint64_t messageCode { 0 };
    uint8_t messageData[dataSize] { 0, };

    static char* data(Message& message) { return reinterpret_cast<char*>(std::addressof(message)); }
    static Message& cast(char* data) { return *reinterpret_cast<Message*>(data); }
};
static_assert(sizeof(Message) == Message::size, "Message is of correct size");

class Host {
public:
    class Handler {
    public:
        virtual void handleFd(int) = 0;
        virtual void handleMessage(char*, size_t) = 0;
    };

    Host();

    void initialize(Handler&);
    void deinitialize();

    int releaseClientFD();

    void sendMessage(char*, size_t);

private:
    static gboolean socketCallback(GSocket*, GIOCondition, gpointer);

    Handler* m_handler;

    GSocket* m_socket;
    GSource* m_source;
    int m_clientFd { -1 };
};

class Client {
public:
    class Handler {
    public:
        virtual void handleMessage(char*, size_t) = 0;
    };

    Client();

    void initialize(Handler&, int);
    void deinitialize();

    void readSynchronously();

    void sendFd(int);
    void sendMessage(char*, size_t);

private:
    static gboolean socketCallback(GSocket*, GIOCondition, gpointer);

    Handler* m_handler;

    GSocket* m_socket;
    GSource* m_source;
};

} // namespace IPC

#endif // wpe_mesa_ipc_h
