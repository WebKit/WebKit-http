#ifndef WPE_ViewBackend_WesterosViewbackendOutput_h
#define WPE_ViewBackend_WesterosViewbackendOutput_h

#include <cstdint>
#include <glib.h>
#include <westeros-compositor.h>

struct wpe_view_backend;

namespace Westeros {

enum WesterosViewbackendOutputMode {
    WesterosViewbackendModeCurrent = 0x1,
    WesterosViewbackendModePreferred = 0x2
};

class WesterosViewbackendOutput {
public:
    WesterosViewbackendOutput(struct wpe_view_backend*);
    virtual ~WesterosViewbackendOutput();
    void initializeNestedOutputHandler(WstCompositor *compositor);
    void initializeClient();
    void deinitialize() { m_viewbackend = nullptr; }

    static void handleGeometryCallback( void *userData, int32_t x, int32_t y, int32_t mmWidth, int32_t mmHeight,
                                                 int32_t subPixel, const char *make, const char *model, int32_t transform );
    static void handleModeCallback( void *userData, uint32_t flags, int32_t width, int32_t height, int32_t refreshRate );
    static void handleDoneCallback( void *UserData );
    static void handleScaleCallback( void *UserData, int32_t scale );

private:
    void clearDataArray();

    WstCompositor* m_compositor;
    struct wpe_view_backend* m_viewbackend;
    uint32_t m_width;
    uint32_t m_height;
    GPtrArray* m_modeDataArray;
};

} // namespace Westeros

#endif // WPE_ViewBackend_WesterosViewbackendOutput_h
