#ifndef GSourceWrap_h
#define GSourceWrap_h

#include <chrono>
#include <functional>
#include <glib.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>
#include <wtf/gobject/GRefPtr.h>

namespace WTF {

class GSourceWrap {
    WTF_MAKE_NONCOPYABLE(GSourceWrap);
private:
    class Base {
    public:
        Base();
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

        struct Source {
            GSource baseSource;
            std::chrono::microseconds delay;
            bool dispatching;
            Base* wrap;
        };

        struct CallbackContext {
            Source& source;
            gpointer data;
        };

        Source* source() const { return reinterpret_cast<Source*>(m_source.get()); }
        GRefPtr<GSource> m_source;
    };

public:
    class Static : public Base {
    public:
        Static() { }
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
