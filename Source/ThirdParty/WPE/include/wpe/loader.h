#ifndef wpe_loader_h
#define wpe_loader_h

#ifdef __cplusplus
extern "C" {
#endif

struct wpe_loader_interface {
    void* (*load_object)(const char*);
};

#ifdef __cplusplus
}
#endif

#endif // wpe_loader_h
