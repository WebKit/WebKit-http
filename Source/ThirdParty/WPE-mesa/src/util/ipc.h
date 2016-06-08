#ifndef wpe_mesa_ipc_h
#define wpe_mesa_ipc_h

#include <gio/gio.h>

namespace IPC {

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

    void send(char*, size_t);

private:
    static gboolean socketCallback(GSocket*, GIOCondition, gpointer);

    Handler* m_handler;

    GSocket* m_socket;
    GSource* m_source;
    size_t m_messageSize { 32 };
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

    void sendFd(int);
    void sendMessage(char*, size_t);

private:
    static gboolean socketCallback(GSocket*, GIOCondition, gpointer);

    Handler* m_handler;

    GSocket* m_socket;
    GSource* m_source;
    size_t m_messageSize { 32 };
};

} // namespace IPC

#endif // wpe_mesa_ipc_h
