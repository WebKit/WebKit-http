#ifndef GSourceWrap_h
#define GSourceWrap_h

#include <chrono>
#include <functional>
#include <glib.h>
#include <utility>
#include <wtf/Vector.h>
#include <wtf/gobject/GRefPtr.h>

typedef struct _GSocket GSocket;

namespace WTF {

class GSourceWrap {
private:
    static GSourceFuncs sourceFunctions;
    static gboolean staticDelayBasedVoidCallback(gpointer);
    static gboolean dynamicDelayBasedVoidCallback(gpointer);
    static gboolean dynamicDelayBasedBoolCallback(gpointer);
    static gboolean staticOneShotCallback(gpointer);
    static gboolean staticSocketCallback(GSocket*, GIOCondition, gpointer);

    using DispatchContext = std::pair<GSource*, gpointer>;
    template<typename T1, typename T2>
    using CallbackContextType = std::pair<std::function<T1>, T2>;

    template<typename T>
    static void destroyCallbackContext(gpointer data)
    {
        auto* context = reinterpret_cast<T*>(data);
        delete context;
    }

    static gint64 targetTimeForDelay(std::chrono::microseconds);

    class Base {
        Base(const Base&) = delete;
        Base& operator=(const Base&) = delete;
        Base(Base&&) = delete;
        Base& operator=(Base&&) = delete;
    public:
        Base() = default;
        ~Base();

    protected:
        GRefPtr<GSource> m_source;
    };

    class DelayBased : Base {
    public:
        DelayBased() = default;

        bool isScheduled() const;
        bool isActive() const;

    protected:
        void initialize(const char* name, int priority, GMainContext*);
        void schedule(std::chrono::microseconds);
        void cancel();

        struct Context {
            std::chrono::microseconds delay;
            GRefPtr<GCancellable> cancellable;
            bool dispatching;
        } m_context;

        friend class GSourceWrap;
        template<typename T>
        using CallbackContext = CallbackContextType<T, Context&>;
    };

public:
    class Static : public DelayBased {
    public:
        Static() = default;
        Static(const char* name, std::function<void ()>&&, int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* = nullptr);
        void initialize(const char* name, std::function<void ()>&&, int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* = nullptr);

        void schedule(std::chrono::microseconds = std::chrono::microseconds(0));
        void cancel();
    };

    class Dynamic : public DelayBased {
    public:
        Dynamic(const char* name, int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* = nullptr);

        void schedule(std::function<void ()>&&, std::chrono::microseconds = std::chrono::microseconds(0));
        void schedule(std::function<bool ()>&&, std::chrono::microseconds = std::chrono::microseconds(0));
        void cancel();
    };

    class OneShot {
    public:
        static void construct(const char* name, std::function<void ()>&& function, std::chrono::microseconds delay = std::chrono::microseconds(0), int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* context = nullptr);

    private:
        friend class GSourceWrap;
        using CallbackContext = CallbackContextType<void (), void*>;
    };

    class Socket : public Base {
    public:
        Socket() = default;
        void initialize(const char* name, std::function<bool (GIOCondition)>&&, GSocket*, GIOCondition, int priority = G_PRIORITY_DEFAULT_IDLE, GMainContext* = nullptr);
        void cancel();

    private:
        friend class GSourceWrap;
        using CallbackContext = CallbackContextType<bool (GIOCondition), GRefPtr<GCancellable>>;

        GRefPtr<GCancellable> m_cancellable;
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
