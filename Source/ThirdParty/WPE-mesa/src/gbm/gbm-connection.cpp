#include "gbm-connection.h"

#include <cstdio>
#include <gio/gunixfdmessage.h>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>

namespace GBM {

Host::Host() = default;

void Host::initialize(Client& client)
{
    m_client = &client;

    int sockets[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    if (ret == -1)
        return;

    m_socket = g_socket_new_from_fd(sockets[0], 0);
    if (!m_socket) {
        close(sockets[0]);
        close(sockets[1]);
        return;
    }

    m_source = g_socket_create_source(m_socket, G_IO_IN, 0);
    g_source_set_callback(m_source, reinterpret_cast<GSourceFunc>(socketCallback), this, 0);
    g_source_attach(m_source, g_main_context_get_thread_default());

    m_clientFd = sockets[1];
}

void Host::deinitialize()
{
    m_client = nullptr;

    if (m_clientFd != -1)
        close(m_clientFd);

    if (m_source)
        g_source_destroy(m_source);
    if (m_socket)
        g_object_unref(m_socket);
}

int Host::releaseClientFD()
{
    int fd = m_clientFd;
    m_clientFd = -1;
    return fd;
}

void Host::frameComplete()
{
    struct ipc_gbm_message messageData = { };
    messageData.message_code = 23;

    g_socket_send(m_socket, reinterpret_cast<char*>(&messageData), sizeof(messageData), 0, 0);
}

void Host::releaseBuffer(uint32_t handle)
{
    struct ipc_gbm_message messageData = { };
    messageData.message_code = 16;

    auto* releaseBuffer = reinterpret_cast<struct release_buffer*>(std::addressof(messageData.data));
    releaseBuffer->handle = handle;

    g_socket_send(m_socket, reinterpret_cast<char*>(&messageData), sizeof(messageData), 0, 0);
}

gboolean Host::socketCallback(GSocket* socket, GIOCondition condition, gpointer data)
{
    auto& host = *static_cast<Host*>(data);

    if (condition & G_IO_IN) {
        GSocketControlMessage** messages;
        int nMessages;

        char* buff[MESSAGE_SIZE];
        GInputVector vector = { buff, sizeof(buff) };

        gssize len = g_socket_receive_message(socket, 0, &vector, 1, &messages, &nMessages, 0, 0, 0);
        if (len == -1)
            return FALSE;

        if (nMessages == 1 && G_IS_UNIX_FD_MESSAGE(messages[0])) {
            GUnixFDMessage* fdMessage = G_UNIX_FD_MESSAGE(messages[0]);

            int* fds;
            int nFds;
            fds = g_unix_fd_message_steal_fds(fdMessage, &nFds);

            if (host.m_client)
                host.m_client->importBufferFd(fds[0]);

            g_free(fds);
        }

        if (nMessages > 0) {
            for (int i = 0; i < nMessages; ++i)
                g_object_unref(messages[i]);
            g_free(messages);

            return TRUE;
        }

        if (len == MESSAGE_SIZE) {
            auto* message = reinterpret_cast<struct ipc_gbm_message*>(&buff);
            uint64_t messageCode = message->message_code;

            switch (messageCode) {
            case 42:
            {
                auto* commit = reinterpret_cast<struct buffer_commit*>(std::addressof(message->data));
                if (host.m_client)
                    host.m_client->commitBuffer(commit);
                break;
            }
            default:
                fprintf(stderr, "ViewBackend: read invalid message\n");
                return FALSE;
            }
        }
    }

    return TRUE;
}

} // namespace GBM
