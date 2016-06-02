#ifndef wpe_gbm_connection_h
#define wpe_gbm_connection_h

#include "ipc-gbm.h"
#include <gio/gio.h>

namespace GBM {

class Host {
public:
    class Client {
    public:
        virtual void importBufferFd(int) = 0;
        virtual void commitBuffer(struct buffer_commit*) = 0;
    };

    Host();

    void initialize(Client&);
    void deinitialize();

    int releaseClientFD();

    void frameComplete();
    void releaseBuffer(uint32_t);

private:
    static gboolean socketCallback(GSocket*, GIOCondition, gpointer);

    Client* m_client;

    GSocket* m_socket;
    GSource* m_source;
    int m_clientFd { -1 };
};

} // namespace GBM

#endif // wpe_gbm_connection_h
