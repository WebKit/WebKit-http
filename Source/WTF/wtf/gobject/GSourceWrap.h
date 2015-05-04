#ifndef GSourceWrap_h
#define GSourceWrap_h

#include <chrono>
#include <functional>
#include <glib.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>
#include <wtf/gobject/GRefPtr.h>

typedef struct _GSocket GSocket;

namespace WTF {

class GSourceWrap {
    WTF_MAKE_NONCOPYABLE(GSourceWrap);
private:
    class Base {
    public:
        Base() = default;
        ~Base();

        bool isScheduled() const;
        bool isActive() const;

    protected:
        void initialize(const char* name, int priority, GMainContext*);
        void schedule(std::chrono::microseconds);
        void cancel();

        static GSourceFuncs sourceFunctions;
        static gboolean staticVoidCallback(gpointer);
        static gboolean dynamicVoidCallback(gpointer);
        static gboolean dynamicBoolCallback(gpointer);
        static gboolean staticOneShotCallback(gpointer);
        static gboolean staticSocketCallback(GSocket*, GIOCondition, gpointer);

        struct Context {
            std::chrono::microseconds delay;
            bool dispatching;
            Base* wrap;
            GRefPtr<GCancellable> cancellable;
        };

        using DispatchContext = std::pair<GSource*, gpointer>;
        template<typename T>
        using CallbackContext = std::pair<std::function<T>, Context&>;
        template<typename T>
        static void destroyCallbackContext(gpointer);

        GRefPtr<GSource> m_source;
        Context m_context;
    };

public:
    class Static : public Base {
    public:
        Static() = default;
        Static(const char* name, std::function<void ()>&&, int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* = nullptr);
        void initialize(const char* name, std::function<void ()>&&, int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* = nullptr);

        void schedule(std::chrono::microseconds = std::chrono::microseconds(0));
        void cancel();
    };

    class Dynamic : public Base {
    public:
        Dynamic(const char* name, int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* = nullptr);

        void schedule(std::function<void ()>&&, std::chrono::microseconds = std::chrono::microseconds(0));
        void schedule(std::function<bool ()>&&, std::chrono::microseconds = std::chrono::microseconds(0));
        void cancel();
    };

    class OneShot : public Base {
    public:
        static void construct(const char* name, std::function<void ()>&& function, std::chrono::microseconds delay = std::chrono::microseconds(0), int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* context = nullptr)
        {
            new OneShot(name, WTF::move(function), delay, priority, context);
        }

    private:
        OneShot(const char* name, std::function<void ()>&&, std::chrono::microseconds, int priority, GMainContext*);
    };

    class Socket : public Base {
    public:
        Socket() = default;
        void initialize(const char* name, std::function<bool (GIOCondition)>&&, GSocket*, GIOCondition, int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* = nullptr);

        void cancel();
    };
};

class GSourceQueue {
    WTF_MAKE_NONCOPYABLE(GSourceQueue);
public:
    GSourceQueue();
    GSourceQueue(const char*, int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* = nullptr);
    ~GSourceQueue();

    void initialize(const char*, int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* = nullptr);

    void queue(std::function<void ()>&&);

private:
    void dispatchQueue();

    GSourceWrap::Static m_sourceWrap;
    GMutex m_mutex;
    Vector<std::function<void ()>, 16> m_queue;
};

} // namespace WTF

using WTF::GSourceWrap;
using WTF::GSourceQueue;

#endif // GSourceWrap_h
