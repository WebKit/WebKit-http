#if WPE_PLATFORM_BCM_RPI

#include <WPE/ViewBackend/ViewBackend.h>

#include <bcm_host.h>

namespace WPE {

namespace ViewBackend {

class ViewBackendBCMRPi final : public ViewBackend {
public:
    ViewBackendBCMRPi();
    virtual ~ViewBackendBCMRPi();

    void setClient(Client*) override;
    uint32_t createBCMElement(int32_t width, int32_t height);
    void commitBCMBuffer(uint32_t handle, uint32_t width, uint32_t height) override;

    void setInputClient(Input::Client*) override;

private:
    Client* m_client;

    DISPMANX_DISPLAY_HANDLE_T m_displayHandle;
    DISPMANX_ELEMENT_HANDLE_T m_elementHandle;
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_PLATFORM_BCM_RPI
