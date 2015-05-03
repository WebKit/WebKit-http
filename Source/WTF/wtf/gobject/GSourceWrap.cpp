#include "config.h"
#include "GSourceWrap.h"

#include <gio/gio.h>
#include <wtf/gobject/GMutexLocker.h>
#include <cstdio>

namespace WTF {

static void destroyVoidCallback(gpointer data)
{
    auto* function = reinterpret_cast<std::function<void ()>*>(data);
    delete function;
}

static void destroyBoolCallback(gpointer data)
{
    auto* function = reinterpret_cast<std::function<bool ()>*>(data);
    delete function;
}

static void destroySocketCallback(gpointer data)
{
    auto* pair = reinterpret_cast<std::pair<std::function<bool (GIOCondition)>, void*>*>(data);
    delete pair;
}

static gint64 targetTimeForDelay(std::chrono::microseconds delay)
{
    gint64 currentTime = g_get_monotonic_time();
    gint64 targetTime = currentTime + std::min<gint64>(G_MAXINT64 - currentTime, delay.count());
    ASSERT(targetTime >= currentTime);

    return targetTime;
}

GSourceWrap::Base::Base()
{
}

GSourceWrap::Base::~Base()
{
    g_source_destroy(m_source.get());
}

bool GSourceWrap::Base::isScheduled() const
{
    ASSERT(m_source);
    return g_source_get_ready_time(m_source.get()) != -1;
}

bool GSourceWrap::Base::isActive() const
{
    return isScheduled() || m_context.dispatching;
}

void GSourceWrap::Base::initialize(const char* name, int priority, GMainContext* context)
{
    ASSERT(!m_source);
    m_source = adoptGRef(g_source_new(&sourceFunctions, sizeof(Source)));
    reinterpret_cast<Source*>(m_source.get())->context = &m_context;

    m_context.delay = std::chrono::microseconds(0);
    m_context.dispatching = false;
    m_context.wrap = this;
    m_context.cancellable = nullptr;

    g_source_set_name(m_source.get(), name);
    if (priority != G_PRIORITY_DEFAULT_IDLE)
        g_source_set_priority(m_source.get(), priority);

    if (!context)
        context = g_main_context_get_thread_default();
    g_source_attach(m_source.get(), context);
}

void GSourceWrap::Base::schedule(std::chrono::microseconds delay)
{
    ASSERT(m_source);
    m_context.delay = delay;

    g_source_set_ready_time(m_source.get(), targetTimeForDelay(delay));
}

void GSourceWrap::Base::cancel()
{
    ASSERT(m_source);
    g_source_set_ready_time(m_source.get(), -1);
}

GSourceFuncs GSourceWrap::Base::sourceFunctions = {
    nullptr, // prepare
    nullptr, // check
    // dispatch
    [](GSource* source, GSourceFunc callback, gpointer data) -> gboolean
    {
        ASSERT(source);
        if (g_source_get_ready_time(source) == -1)
            return G_SOURCE_CONTINUE;
        CallbackContext context{ *reinterpret_cast<Source*>(source), data };
        return callback(&context);
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

gboolean GSourceWrap::Base::staticVoidCallback(gpointer data)
{
    auto& context = *reinterpret_cast<CallbackContext*>(data);
    context.source.context->dispatching = true;
    g_source_set_ready_time(&context.source.baseSource, -1);

    auto& function = *reinterpret_cast<std::function<void ()>*>(context.data);
    function();

    context.source.context->dispatching = false;
    return G_SOURCE_CONTINUE;
}

gboolean GSourceWrap::Base::dynamicVoidCallback(gpointer data)
{
    auto& context = *reinterpret_cast<CallbackContext*>(data);
    context.source.context->dispatching = true;
    g_source_set_ready_time(&context.source.baseSource, -1);
    g_source_set_callback(&context.source.baseSource, nullptr, nullptr, nullptr);

    auto& function = *reinterpret_cast<std::function<void ()>*>(context.data);
    function();

    context.source.context->dispatching = false;
    return G_SOURCE_CONTINUE;
}

gboolean GSourceWrap::Base::dynamicBoolCallback(gpointer data)
{
    auto& context = *reinterpret_cast<CallbackContext*>(data);
    context.source.context->dispatching = true;
    g_source_set_ready_time(&context.source.baseSource, -1);

    auto& function = *reinterpret_cast<std::function<bool ()>*>(context.data);
    if (function())
        g_source_set_ready_time(&context.source.baseSource, targetTimeForDelay(context.source.context->delay));
    else
        g_source_set_callback(&context.source.baseSource, nullptr, nullptr, nullptr);

    context.source.context->dispatching = false;
    return G_SOURCE_CONTINUE;
}

gboolean GSourceWrap::Base::staticOneShotCallback(gpointer data)
{
    auto& context = *reinterpret_cast<CallbackContext*>(data);
    context.source.context->dispatching = true;
    g_source_set_ready_time(&context.source.baseSource, -1);

    auto& function = *reinterpret_cast<std::function<void ()>*>(context.data);
    function();

    context.source.context->dispatching = false;
    delete context.source.context->wrap;
    return G_SOURCE_REMOVE;
}

gboolean GSourceWrap::Base::staticSocketCallback(GSocket*, GIOCondition condition, gpointer data)
{
    auto& pair = *reinterpret_cast<std::pair<std::function<bool (GIOCondition)>, Source::Context&>*>(data);
    if (g_cancellable_is_cancelled(pair.second.cancellable.get()))
        return G_SOURCE_REMOVE;

    pair.second.dispatching = true;

    bool retval = pair.first(condition);

    pair.second.dispatching = false;
    return retval;
}

GSourceWrap::Static::Static(const char* name, std::function<void ()>&& function, int priority, GMainContext* context)
{
    initialize(name, WTF::move(function), priority, context);
}

void GSourceWrap::Static::initialize(const char* name, std::function<void ()>&& function, int priority, GMainContext* context)
{
    Base::initialize(name, priority, context);

    g_source_set_callback(m_source.get(), static_cast<GSourceFunc>(staticVoidCallback),
        new std::function<void ()>(WTF::move(function)), static_cast<GDestroyNotify>(destroyVoidCallback));
}

void GSourceWrap::Static::schedule(std::chrono::microseconds delay)
{
    Base::schedule(delay);
}

void GSourceWrap::Static::cancel()
{
    Base::cancel();
}

GSourceWrap::Dynamic::Dynamic(const char* name, int priority, GMainContext* context)
{
    Base::initialize(name, priority, context);
}

void GSourceWrap::Dynamic::schedule(std::function<void ()>&& function, std::chrono::microseconds delay)
{
    g_source_set_callback(m_source.get(), static_cast<GSourceFunc>(dynamicVoidCallback),
        new std::function<void ()>(WTF::move(function)), static_cast<GDestroyNotify>(destroyVoidCallback));

    Base::schedule(delay);
}

void GSourceWrap::Dynamic::schedule(std::function<bool ()>&& function, std::chrono::microseconds delay)
{
    g_source_set_callback(m_source.get(), static_cast<GSourceFunc>(dynamicBoolCallback),
        new std::function<bool ()>(WTF::move(function)), static_cast<GDestroyNotify>(destroyBoolCallback));

    Base::schedule(delay);
}

void GSourceWrap::Dynamic::cancel()
{
    Base::cancel();

    g_source_set_callback(m_source.get(), nullptr, nullptr, nullptr);
}

GSourceWrap::OneShot::OneShot(const char* name, std::function<void ()>&& function, std::chrono::microseconds delay, int priority, GMainContext* context)
{
    Base::initialize(name, priority, context);

    g_source_set_callback(m_source.get(), static_cast<GSourceFunc>(staticOneShotCallback),
        new std::function<void ()>(WTF::move(function)), static_cast<GDestroyNotify>(destroyVoidCallback));

    Base::schedule(delay);
}

void GSourceWrap::Socket::initialize(const char* name, std::function<bool (GIOCondition)>&& function, GSocket* socket, GIOCondition condition, int priority, GMainContext* context)
{
    ASSERT(!m_source);
    GCancellable* cancellable = g_cancellable_new();
    m_source = adoptGRef(g_socket_create_source(socket, condition, cancellable));

    m_context.delay = std::chrono::microseconds(0);
    m_context.dispatching = false;
    m_context.wrap = this;
    m_context.cancellable = adoptGRef(cancellable);

    g_source_set_name(m_source.get(), name);
    if (priority != G_PRIORITY_DEFAULT_IDLE)
        g_source_set_priority(m_source.get(), priority);

    g_source_set_callback(m_source.get(), reinterpret_cast<GSourceFunc>(staticSocketCallback),
        new std::pair<std::function<bool (GIOCondition)>, Base::Source::Context&>{ WTF::move(function), m_context }, static_cast<GDestroyNotify>(destroySocketCallback));

    if (!context)
        context = g_main_context_get_thread_default();
    g_source_attach(m_source.get(), context);
}

void GSourceWrap::Socket::cancel()
{
    g_cancellable_cancel(m_context.cancellable.get());
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
