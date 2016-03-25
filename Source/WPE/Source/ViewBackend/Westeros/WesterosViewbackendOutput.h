#ifndef WPE_ViewBackend_WesterosViewbackendOutput_h
#define WPE_ViewBackend_WesterosViewbackendOutput_h

#if WPE_BACKEND(WESTEROS)

#include "ViewBackendWesteros.h"

namespace WPE {

namespace ViewBackend {

enum WesterosViewbackendOutputMode {
    WesterosViewbackendModeCurrent = 0x1,
    WesterosViewbackendModePreferred = 0x2
};

class WesterosViewbackendOutput {
public:
    WesterosViewbackendOutput();
    virtual ~WesterosViewbackendOutput();
    void registerClient(Client* client);
    void unregisterClient() { m_client = nullptr; }
    void initializeNestedOutputHandler(WstCompositor *compositor, ViewBackendWesteros *backend);

    static void handleGeometryCallback( void *userData, int32_t x, int32_t y, int32_t mmWidth, int32_t mmHeight,
                                                 int32_t subPixel, const char *make, const char *model, int32_t transform );
    static void handleModeCallback( void *userData, uint32_t flags, int32_t width, int32_t height, int32_t refreshRate );
    static void handleDoneCallback( void *UserData );
    static void handleScaleCallback( void *UserData, int32_t scale );

private:
    WstCompositor* m_compositor;
    ViewBackendWesteros* m_viewbackend;
    Client* m_client;
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(WESTEROS)
#endif // WPE_ViewBackend_WesterosViewbackendOutput_h
