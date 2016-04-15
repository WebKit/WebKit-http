#ifndef WPEViewClient_h
#define WPEViewClient_h

#include "APIClient.h"
#include <WebKit/WKView.h>

namespace API {
template<> struct ClientTraits<WKViewClientBase> {
    typedef std::tuple<WKViewClientV0> Versions;
};
}

namespace WKWPE {

class View;

class ViewClient : public API::Client<WKViewClientBase> {
public:
    void frameDisplayed(View&);
};

} // namespace WebKit


#endif // WPEViewClient_h
