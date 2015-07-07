#include "config.h"
#include "Server.h"

#include "quick_ssdp.h"
#include <cstdio>

namespace DIAL {

Server::Server(Client& client)
    : m_client(client)
{
    g_mutex_init(&m_threadMutex);
    g_cond_init(&m_threadCondition);

    {
        g_mutex_lock(&m_threadMutex);
        m_thread = g_thread_new("AtholShell DIAL", Server::DIALThread, this);

        g_cond_wait(&m_threadCondition, &m_threadMutex);
        g_mutex_unlock(&m_threadMutex);
    }
}

Server::~Server()
{
    g_thread_unref(m_thread);
    g_mutex_clear(&m_threadMutex);
    g_cond_clear(&m_threadCondition);
}

gpointer Server::DIALThread(gpointer data)
{
    auto& server = *reinterpret_cast<Server*>(data);

    {
        g_mutex_lock(&server.m_threadMutex);
        g_cond_signal(&server.m_threadCondition);
        g_mutex_unlock(&server.m_threadMutex);
    }

    server.m_dialServer = DIAL_create();
    DIAL_register_app(server.m_dialServer, "Netflix", &m_callbacks.netflix, &server, 1, ".netflix.com");
    DIAL_register_app(server.m_dialServer, "YouTube", &m_callbacks.yt, &server, 1, ".youtube.com");

    DIAL_start(server.m_dialServer);
    run_ssdp(DIAL_get_port(server.m_dialServer), "WPE DIAL", "WPE DIAL 0.1", "46cd5f7b-1c6e-43dc-8cd7-ab0d6f498fb4");

    DIAL_stop(server.m_dialServer);
    free(server.m_dialServer);

    return nullptr;
}

struct Server::Callbacks Server::m_callbacks = {
    // Netflix callbacks
    {
        // start
        [](DIALServer*, const char* appname, const char*, size_t, DIAL_run_t* runID, void* data) -> DIALStatus
        {
            auto& server = *reinterpret_cast<Server*>(data);
            server.m_currentAppID = server.m_appID++;

            const char* url = "https://www.netflix.com";
            std::fprintf(stderr, "[DIAL] Start app '%s' - '%s'\n", appname, url);
            server.m_client.startApp(server.m_currentAppID, url);

            *runID = reinterpret_cast<void*>(server.m_currentAppID);
            return kDIALStatusRunning;
        },
        // stop
        [](DIALServer*, const char* appname, DIAL_run_t runID, void* data)
        {
            auto& server = *reinterpret_cast<Server*>(data);
            if (reinterpret_cast<void*>(server.m_currentAppID) != runID)
                return;

            std::fprintf(stderr, "[DIAL] Stop app '%s'\n", appname);
            server.m_client.stopApp(server.m_currentAppID);
            server.m_currentAppID = 0;
        },
        // status
        [](DIALServer*, const char* appname, DIAL_run_t runID, int* canStop, void* data) -> DIALStatus
        {
            auto& server = *reinterpret_cast<Server*>(data);
            *canStop = 1;
            std::fprintf(stderr, "[DIAL] Status for app '%s'\n", appname);
            return reinterpret_cast<void*>(server.m_currentAppID) == runID ? kDIALStatusRunning : kDIALStatusStopped;
        },
    },

    // YouTube callbacks
    {
        // start
        [](DIALServer*, const char* appname, const char* args, size_t, DIAL_run_t* runID, void* data) -> DIALStatus
        {
            auto& server = *reinterpret_cast<Server*>(data);
            server.m_currentAppID = server.m_appID++;

            char* url = g_strdup_printf("https://www.youtube.com/tv?%s", args);
            std::fprintf(stderr, "[DIAL] Start app '%s' - '%s'\n", appname, url);
            server.m_client.startApp(server.m_currentAppID, url);
            g_free(url);

            *runID = reinterpret_cast<void*>(server.m_currentAppID);
            return kDIALStatusRunning;
        },
        // stop
        [](DIALServer*, const char* appname, DIAL_run_t runID, void* data)
        {
            auto& server = *reinterpret_cast<Server*>(data);
            if (reinterpret_cast<void*>(server.m_currentAppID) != runID)
                return;

            std::fprintf(stderr, "[DIAL] Stop app '%s'\n", appname);
            server.m_client.stopApp(server.m_currentAppID);
            server.m_currentAppID = 0;
        },
        // status
        [](DIALServer*, const char* appname, DIAL_run_t runID, int* canStop, void* data) -> DIALStatus
        {
            auto& server = *reinterpret_cast<Server*>(data);
            *canStop = 1;
            std::fprintf(stderr, "[DIAL] Status for app '%s'\n", appname);
            return reinterpret_cast<void*>(server.m_currentAppID) == runID ? kDIALStatusRunning : kDIALStatusStopped;
        },
    },
};

unsigned Server::m_appID = 1;

} // namespace DIAL
