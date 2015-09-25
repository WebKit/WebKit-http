#ifndef LibinputServer_h
#define LibinputServer_h

#include <glib.h>
#include <libinput.h>
#include <libudev.h>
#include <memory>

namespace WPE {

namespace Input {
class Client;
class KeyboardEventHandler;
}

class LibinputServer {
public:
    static LibinputServer& singleton();

    void setClient(Input::Client* client);

private:
    LibinputServer();
    ~LibinputServer();

    void processEvents();

    struct udev* m_udev;
    struct libinput* m_libinput;

    Input::Client* m_client;
    std::unique_ptr<Input::KeyboardEventHandler> m_keyboardEventHandler;

    class EventSource {
    public:
        static GSourceFuncs s_sourceFuncs;

        GSource source;
        GPollFD pfd;
        LibinputServer* server;
    };
};

} // namespace WPE

#endif // LibinputServer_h
