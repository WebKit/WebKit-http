#ifndef WPE_ViewBackend_ViewBackendWesteros_h
#define WPE_ViewBackend_ViewBackendWesteros_h

#if WPE_BACKEND(WESTEROS)

#include <WPE/ViewBackend/ViewBackend.h>

#include <westeros-compositor.h>

namespace WPE {

namespace ViewBackend {

class WesterosViewbackendInput;
class WesterosViewbackendOutput;

class ViewBackendWesteros final : public ViewBackend {
public:
    ViewBackendWesteros();
    virtual ~ViewBackendWesteros();

    void setClient(Client*) override;
    std::pair<const uint8_t*, size_t> authenticate() override { return { nullptr, 0 }; };
    uint32_t constructRenderingTarget(uint32_t, uint32_t) override;
    void commitBuffer(int, const uint8_t*, size_t) override;
    void destroyBuffer(uint32_t) override;
    void setInputClient(Input::Client*) override;

private:
    WesterosViewbackendInput* m_input_handler;
    WesterosViewbackendOutput* m_output_handler;
    WstCompositor* m_compositor;
    Client* m_client;
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(WESTEROS)
#endif // WPE_ViewBackend_ViewBackendWesteros_h
