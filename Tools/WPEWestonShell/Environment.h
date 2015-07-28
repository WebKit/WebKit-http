#ifndef Environment_h
#define Environment_h

#include <DerivedSources/WebCore/WaylandWPEProtocolServer.h>
#include <WebKit/WKGeometry.h>
#include <weston/compositor.h>

namespace WPE {

class Environment {
public:
    Environment(struct weston_compositor*);

    struct weston_compositor* compositor() const { return m_compositor; }
    WKSize outputSize() const { return m_outputSize; }

    void updateCursorPosition(int x, int y);

private:
    static const struct wl_wpe_interface m_wpeInterface;

    void createOutput(struct weston_output*);
    static void outputCreated(struct wl_listener*, void* data);
    void createCursor();

    void registerSurface(struct weston_surface*);

    struct weston_compositor* m_compositor;

    struct weston_layer m_layer;
    struct wl_list m_outputList;
    struct wl_listener m_outputCreatedListener;
    WKSize m_outputSize;

    static constexpr unsigned c_cursorSize = 5;
    static constexpr unsigned c_cursorOffset = (c_cursorSize - 1) / 2;
    struct weston_view* m_cursorView;
};

} // namespace WPE

#endif // Environment_h
