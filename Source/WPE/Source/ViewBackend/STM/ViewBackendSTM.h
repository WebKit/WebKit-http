#if WPE_BACKEND(STM)

#include <WPE/ViewBackend/ViewBackend.h>

namespace WPE {

namespace ViewBackend {

class ViewBackendSTM final : public ViewBackend {
public:
    ViewBackendSTM();
    virtual ~ViewBackendSTM();

    void setClient(Client*) override;
    uint32_t constructRenderingTarget(uint32_t, uint32_t) override;
    std::pair<const uint8_t*, size_t> authenticate() override { return { nullptr, 0 }; };
    void commitBuffer(int, const uint8_t*, size_t) override;
    void destroyBuffer(uint32_t) override;

    void handleKeyboardEvent(const Input::KeyboardEvent& event) override;
    void handlePointerEvent(const Input::PointerEvent& event) override;
    void handleAxisEvent(const Input::AxisEvent& event) override;

    void setInputClient(Input::Client*) override;

private:
    Client* m_client;
    Input::Client* m_input_client;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(STM)
