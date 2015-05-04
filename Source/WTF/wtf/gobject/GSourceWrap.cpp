#include "config.h"
#include "GSourceWrap.h"

#include <gio/gio.h>
#include <wtf/gobject/GMutexLocker.h>
#include <cstdio>

namespace WTF {

GSourceFuncs GSourceWrap::sourceFunctions = {
    nullptr, // prepare
    nullptr, // check
    // dispatch
    [](GSource* source, GSourceFunc callback, gpointer data) -> gboolean
    {
        ASSERT(source);
        if (g_source_get_ready_time(source) == -1)
            return G_SOURCE_CONTINUE;
        DispatchContext context{ source, data };
        return callback(&context);
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

gboolean GSourceWrap::staticDelayBasedVoidCallback(gpointer data)
{
    auto& dispatch = *reinterpret_cast<DispatchContext*>(data);
    auto& callback = *reinterpret_cast<DelayBased::CallbackContext<void ()>*>(dispatch.second);
    if (g_cancellable_is_cancelled(callback.second.cancellable.get()))
        return G_SOURCE_CONTINUE;

    callback.second.dispatching = true;
    g_source_set_ready_time(dispatch.first, -1);

    callback.first();

    callback.second.dispatching = false;
    return G_SOURCE_CONTINUE;
}

gboolean GSourceWrap::dynamicDelayBasedVoidCallback(gpointer data)
{
    auto& dispatch = *reinterpret_cast<DispatchContext*>(data);
    auto& callback = *reinterpret_cast<DelayBased::CallbackContext<void ()>*>(dispatch.second);
    if (g_cancellable_is_cancelled(callback.second.cancellable.get()))
        return G_SOURCE_CONTINUE;

    callback.second.dispatching = true;
    g_source_set_ready_time(dispatch.first, -1);
    g_source_set_callback(dispatch.first, nullptr, nullptr, nullptr);

    callback.first();

    callback.second.dispatching = false;
    return G_SOURCE_CONTINUE;
}

gboolean GSourceWrap::dynamicDelayBasedBoolCallback(gpointer data)
{
    auto& dispatch = *reinterpret_cast<DispatchContext*>(data);
    auto& callback = *reinterpret_cast<DelayBased::CallbackContext<bool ()>*>(dispatch.second);
    if (g_cancellable_is_cancelled(callback.second.cancellable.get()))
        return G_SOURCE_CONTINUE;

    callback.second.dispatching = true;
    g_source_set_ready_time(dispatch.first, -1);

    if (callback.first())
        g_source_set_ready_time(dispatch.first, targetTimeForDelay(callback.second.delay));
    else
        g_source_set_callback(dispatch.first, nullptr, nullptr, nullptr);

    callback.second.dispatching = false;
    return G_SOURCE_CONTINUE;
}

gboolean GSourceWrap::staticOneShotCallback(gpointer data)
{
    auto& dispatch = *reinterpret_cast<DispatchContext*>(data);
    auto& callback = *reinterpret_cast<OneShot::CallbackContext*>(dispatch.second);

    g_source_set_ready_time(dispatch.first, -1);
    callback.first();

    return G_SOURCE_REMOVE;
}

gboolean GSourceWrap::staticSocketCallback(GSocket*, GIOCondition condition, gpointer data)
{
    auto& callback = *reinterpret_cast<Socket::CallbackContext*>(data);
    if (g_cancellable_is_cancelled(callback.second.get()))
        return G_SOURCE_REMOVE;

    return callback.first(condition);
}

gint64 GSourceWrap::targetTimeForDelay(std::chrono::microseconds delay)
{
    gint64 currentTime = g_get_monotonic_time();
    gint64 targetTime = currentTime + std::min<gint64>(G_MAXINT64 - currentTime, delay.count());
    ASSERT(targetTime >= currentTime);

    return targetTime;
}

GSourceWrap::Base::~Base()
{
    g_source_destroy(m_source.get());
}

bool GSourceWrap::DelayBased::isScheduled() const
{
    ASSERT(m_source);
    return g_source_get_ready_time(m_source.get()) != -1;
}

bool GSourceWrap::DelayBased::isActive() const
{
    return isScheduled() || m_context.dispatching;
}

void GSourceWrap::DelayBased::initialize(const char* name, int priority, GMainContext* context)
{
    ASSERT(!m_source);
    m_source = adoptGRef(g_source_new(&sourceFunctions, sizeof(GSource)));

    m_context.delay = std::chrono::microseconds(0);
    m_context.cancellable = adoptGRef(g_cancellable_new());
    m_context.dispatching = false;

    g_source_set_name(m_source.get(), name);
    g_source_set_priority(m_source.get(), priority);

    if (!context)
        context = g_main_context_get_thread_default();
    g_source_attach(m_source.get(), context);
}

void GSourceWrap::DelayBased::schedule(std::chrono::microseconds delay)
{
    ASSERT(m_source);
    m_context.delay = delay;

    if (g_cancellable_is_cancelled(m_context.cancellable.get()))
        m_context.cancellable = adoptGRef(g_cancellable_new());

    g_source_set_ready_time(m_source.get(), targetTimeForDelay(delay));
}

void GSourceWrap::DelayBased::cancel()
{
    ASSERT(m_source);
    g_cancellable_cancel(m_context.cancellable.get());
    g_source_set_ready_time(m_source.get(), -1);
}

GSourceWrap::Static::Static(const char* name, std::function<void ()>&& function, int priority, GMainContext* context)
{
    initialize(name, WTF::move(function), priority, context);
}

void GSourceWrap::Static::initialize(const char* name, std::function<void ()>&& function, int priority, GMainContext* context)
{
    DelayBased::initialize(name, priority, context);

    g_source_set_callback(m_source.get(), static_cast<GSourceFunc>(staticDelayBasedVoidCallback),
        new CallbackContext<void ()>{ WTF::move(function), m_context }, static_cast<GDestroyNotify>(destroyCallbackContext<CallbackContext<void ()>>));
}

void GSourceWrap::Static::schedule(std::chrono::microseconds delay)
{
    DelayBased::schedule(delay);
}

void GSourceWrap::Static::cancel()
{
    DelayBased::cancel();
}

GSourceWrap::Dynamic::Dynamic(const char* name, int priority, GMainContext* context)
{
    DelayBased::initialize(name, priority, context);
}

void GSourceWrap::Dynamic::schedule(std::function<void ()>&& function, std::chrono::microseconds delay)
{
    g_source_set_callback(m_source.get(), static_cast<GSourceFunc>(dynamicDelayBasedVoidCallback),
        new CallbackContext<void ()>{ WTF::move(function), m_context }, static_cast<GDestroyNotify>(destroyCallbackContext<CallbackContext<void ()>>));

    DelayBased::schedule(delay);
}

void GSourceWrap::Dynamic::schedule(std::function<bool ()>&& function, std::chrono::microseconds delay)
{
    g_source_set_callback(m_source.get(), static_cast<GSourceFunc>(dynamicDelayBasedBoolCallback),
        new CallbackContext<bool ()>{ WTF::move(function), m_context }, static_cast<GDestroyNotify>(destroyCallbackContext<CallbackContext<bool ()>>));

    DelayBased::schedule(delay);
}

void GSourceWrap::Dynamic::cancel()
{
    DelayBased::cancel();

    g_source_set_callback(m_source.get(), nullptr, nullptr, nullptr);
}

void GSourceWrap::OneShot::construct(const char* name, std::function<void ()>&& function, std::chrono::microseconds delay, int priority, GMainContext* context)
{
    GRefPtr<GSource> source = adoptGRef(g_source_new(&sourceFunctions, sizeof(GSource)));

    g_source_set_name(source.get(), name);
    g_source_set_priority(source.get(), priority);

    g_source_set_callback(source.get(), static_cast<GSourceFunc>(staticOneShotCallback),
        new CallbackContext{ WTF::move(function), nullptr }, static_cast<GDestroyNotify>(destroyCallbackContext<CallbackContext>));
    g_source_set_ready_time(source.get(), targetTimeForDelay(delay));

    if (!context)
        context = g_main_context_get_thread_default();
    g_source_attach(source.get(), context);
}

void GSourceWrap::Socket::initialize(const char* name, std::function<bool (GIOCondition)>&& function, GSocket* socket, GIOCondition condition, int priority, GMainContext* context)
{
    ASSERT(!m_source);
    GCancellable* cancellable = g_cancellable_new();
    m_source = adoptGRef(g_socket_create_source(socket, condition, cancellable));
    m_cancellable = adoptGRef(cancellable);

    g_source_set_name(m_source.get(), name);
    g_source_set_priority(m_source.get(), priority);

    g_source_set_callback(m_source.get(), reinterpret_cast<GSourceFunc>(staticSocketCallback),
        new CallbackContext{ WTF::move(function), m_cancellable }, static_cast<GDestroyNotify>(destroyCallbackContext<CallbackContext>));

    if (!context)
        context = g_main_context_get_thread_default();
    g_source_attach(m_source.get(), context);
}

void GSourceWrap::Socket::cancel()
{
    g_cancellable_cancel(m_cancellable.get());
}

GSourceQueue::GSourceQueue()
{
    g_mutex_init(&m_mutex);
}

GSourceQueue::GSourceQueue(const char* name, int priority, GMainContext* context)
    : m_sourceWrap(name, std::bind(&GSourceQueue::dispatchQueue, this), priority, context)
{
    g_mutex_init(&m_mutex);
}

GSourceQueue::~GSourceQueue()
{
    g_mutex_clear(&m_mutex);
}

void GSourceQueue::initialize(const char* name, int priority, GMainContext* context)
{
    m_sourceWrap.initialize(name, std::bind(&GSourceQueue::dispatchQueue, this), priority, context);
}

void GSourceQueue::queue(std::function<void ()>&& function)
{
    WTF::GMutexLocker<GMutex> lock(m_mutex);
    m_queue.append(WTF::move(function));

    m_sourceWrap.schedule();
}

void GSourceQueue::dispatchQueue()
{
    while (1) {
        decltype(m_queue) queue;
        {
            WTF::GMutexLocker<GMutex> lock(m_mutex);
            queue = WTF::move(m_queue);
        }

        if (!queue.size())
            break;

        for (auto& function : queue)
            function();
    }
}

} // namespace WTF
