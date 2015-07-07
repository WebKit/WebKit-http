#ifndef DIAL_Server_h
#define DIAL_Server_h

#include "dial_server.h"
#include <glib.h>

namespace DIAL {

class Server {
public:
    class Client {
    public:
        virtual void startApp(unsigned, const char*) = 0;
        virtual void stopApp(unsigned) = 0;
    };

    Server(Client&);
    ~Server();

private:
    Client& m_client;

    GMutex m_threadMutex;
    GCond m_threadCondition;

    static gpointer DIALThread(gpointer);
    GThread* m_thread;

    DIALServer* m_dialServer;
    static struct Callbacks {
        struct DIALAppCallbacks netflix;
        struct DIALAppCallbacks yt;
    } m_callbacks;

    static unsigned m_appID;
    unsigned m_currentAppID { 0 };
};

} // namespace DIAL

#endif // DIAL_Server_h
