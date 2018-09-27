/*
 * Copyright (C) 2011, 2012 Apple Inc. All Rights Reserved.
 * Copyright (C) 2014 Raspberry Pi Foundation. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MemoryPressureHandler.h"

#if OS(LINUX)

#include "CurrentProcessMemoryStatus.h"
#include "Logging.h"

#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <malloc.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>
#include <wtf/text/WTFString.h>

#if USE(GLIB)
#include <glib-unix.h>
#endif

namespace WebCore {

// Disable memory event reception for a minimum of s_minimumHoldOffTime
// seconds after receiving an event. Don't let events fire any sooner than
// s_holdOffMultiplier times the last cleanup processing time. Effectively
// this is 1 / s_holdOffMultiplier percent of the time.
// If after releasing the memory we don't free at least s_minimumBytesFreedToUseMinimumHoldOffTime,
// we wait longer to try again (s_maximumHoldOffTime).
// These value seems reasonable and testing verifies that it throttles frequent
// low memory events, greatly reducing CPU usage.
static const unsigned s_minimumHoldOffTime = 5;
static const unsigned s_maximumHoldOffTime = 30;
static const size_t s_minimumBytesFreedToUseMinimumHoldOffTime = 1 * MB;
static const unsigned s_holdOffMultiplier = 20;
static const unsigned s_pollTimeSec = 1;
static const size_t s_memCriticalLimit = 3 * KB * KB; // 3 MB
static const size_t s_memNonCriticalLimit = 5 * KB * KB; // 5 MB
static size_t s_pollMaximumProcessMemoryCriticalLimit = 0;
static size_t s_pollMaximumProcessMemoryNonCriticalLimit = 0;

static const char* s_cgroupMemoryPressureLevel = "/sys/fs/cgroup/memory/memory.pressure_level";
static const char* s_cgroupEventControl = "/sys/fs/cgroup/memory/cgroup.event_control";

static const char* s_processStatus = "/proc/self/status";
static const char* s_memInfo = "/proc/meminfo";
static const char* s_cmdline = "/proc/self/cmdline";


#if USE(GLIB)
typedef struct {
    GSource source;
    gpointer fdTag;
    GIOCondition condition;
} EventFDSource;

static const unsigned eventFDSourceCondition = G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL;

static GSourceFuncs eventFDSourceFunctions = {
    nullptr, // prepare
    nullptr, // check
    // dispatch
    [](GSource* source, GSourceFunc callback, gpointer userData) -> gboolean
    {
        EventFDSource* eventFDSource = reinterpret_cast<EventFDSource*>(source);
        unsigned events = g_source_query_unix_fd(source, eventFDSource->fdTag) & eventFDSourceCondition;
        if (events & G_IO_HUP || events & G_IO_ERR || events & G_IO_NVAL)
            return G_SOURCE_REMOVE;

        gboolean returnValue = G_SOURCE_CONTINUE;
        if (events & G_IO_IN)
            returnValue = callback(userData);
        g_source_set_ready_time(source, -1);
        return returnValue;
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};
#endif

MemoryPressureHandler::EventFDPoller::EventFDPoller(int fd, std::function<void ()>&& notifyHandler)
    : m_fd(fd)
    , m_notifyHandler(WTFMove(notifyHandler))
{
#if USE(GLIB)
    if (m_fd.value() == -1) {
        m_threadID = createThread(pollMemoryPressure, this, "WebCore: MemoryPressureHandler");
        return;
    }

    m_source = adoptGRef(g_source_new(&eventFDSourceFunctions, sizeof(EventFDSource)));
    g_source_set_name(m_source.get(), "WebCore: MemoryPressureHandler");
    if (!g_unix_set_fd_nonblocking(m_fd.value(), TRUE, nullptr)) {
        LOG(MemoryPressure, "Failed to set eventfd nonblocking");
        return;
    }

    EventFDSource* eventFDSource = reinterpret_cast<EventFDSource*>(m_source.get());
    eventFDSource->fdTag = g_source_add_unix_fd(m_source.get(), m_fd.value(), static_cast<GIOCondition>(eventFDSourceCondition));
    g_source_set_callback(m_source.get(), [](gpointer userData) -> gboolean {
        static_cast<EventFDPoller*>(userData)->readAndNotify();
        return G_SOURCE_REMOVE;
    }, this, nullptr);
    g_source_attach(m_source.get(), nullptr);
#else
    m_threadID = createThread("WebCore: MemoryPressureHandler", [this] { readAndNotify(); }
#endif
}

MemoryPressureHandler::EventFDPoller::~EventFDPoller()
{
    m_fd = std::nullopt;
#if USE(GLIB)
    g_source_destroy(m_source.get());
#else
    detachThread(m_threadID);
#endif
}

static inline bool isFatalReadError(int error)
{
#if USE(GLIB)
    // We don't really need to read the buffer contents, if the poller
    // notified us, but read would block or is no longer available, is
    // enough to trigger the memory pressure handler.
    return error != EAGAIN && error != EWOULDBLOCK;
#else
    return true;
#endif
}

void MemoryPressureHandler::EventFDPoller::readAndNotify() const
{
    if (!m_fd) {
        LOG(MemoryPressure, "Invalidate eventfd.");
        return;
    }

    uint64_t buffer;
    if (read(m_fd.value(), &buffer, sizeof(buffer)) == -1) {
        if (isFatalReadError(errno)) {
            LOG(MemoryPressure, "Failed to read eventfd.");
            return;
        }
    }

    m_notifyHandler();
}

static inline String nextToken(FILE* file)
{
    if (!file)
        return String();

    static const unsigned bufferSize = 128;
    char buffer[bufferSize] = {0, };
    unsigned index = 0;
    while (index < bufferSize) {
        int ch = fgetc(file);
        if (ch == EOF || (isASCIISpace(ch) && index)) // Break on non-initial ASCII space.
            break;
        if (!isASCIISpace(ch)) {
            buffer[index] = ch;
            index++;
        }
    }

    return String(buffer);
}

size_t readToken(const char* filename, const char* key, size_t fileUnits)
{
    size_t result = static_cast<size_t>(-1);
    FILE* file = fopen(filename, "r");
    if (!file)
        return result;

    String token = nextToken(file);
    while (!token.isEmpty()) {
        if (token == key) {
            result = nextToken(file).toUInt64() * fileUnits;
            break;
        }
        token = nextToken(file);
    }
    fclose(file);
    return result;
}

static String getProcessName()
{
    FILE* file = fopen(s_cmdline, "r");
    if (!file)
        return String();

    String result = nextToken(file);
    fclose(file);

    return result;
}

static bool defaultPollMaximumProcessMemory(size_t &criticalLimit, size_t &nonCriticalLimit)
{
    // Syntax: Case insensitive, process name, wildcard (*), unit multipliers (M=Mb, K=Kb, <empty>=bytes).
    // Example: WPE_POLL_MAX_MEMORY='WPEWebProcess:500M,*Process:150M'

    String processName(getProcessName().convertToLowercaseWithoutLocale());
    String s(getenv("WPE_POLL_MAX_MEMORY"));
    if (!s.isEmpty()) {
        Vector<String> entries;
        s.split(',', false, entries);
        for (const String& entry : entries) {
            Vector<String> keyvalue;
            entry.split(':', false, keyvalue);
            if (keyvalue.size() != 2)
                continue;
            String key = "*"+keyvalue[0].stripWhiteSpace().convertToLowercaseWithoutLocale();
            String value = keyvalue[1].stripWhiteSpace().convertToLowercaseWithoutLocale();
            size_t units = 1;
            if (value.endsWith('k'))
                units = 1024;
            else if (value.endsWith('m'))
                units = 1024 * 1024;
            if (units != 1)
                value = value.substring(0, value.length()-1);
            bool ok = false;
            size_t size = size_t(value.toUInt64(&ok));
            if (!ok)
                continue;

            if (!fnmatch(key.utf8().data(), processName.utf8().data(), 0)) {
                criticalLimit = size * units;
                nonCriticalLimit = criticalLimit * 0.95; //0.75;
                return true;
            }
        }
    }

    return false;
}

void MemoryPressureHandler::pollMemoryPressure(void*)
{
    ASSERT(!isMainThread());

    bool critical;
    String processName(getProcessName());
    do {
        if (s_pollMaximumProcessMemoryCriticalLimit) {
            size_t vmRSS = readToken(s_processStatus, "VmRSS:", KB);

            if (!vmRSS)
                return;

            if (vmRSS > s_pollMaximumProcessMemoryNonCriticalLimit) {
                critical = vmRSS > s_pollMaximumProcessMemoryCriticalLimit;
                break;
            }
        }

        size_t memFree = readToken(s_memInfo, "MemFree:", KB);

        if (!memFree)
            return;

        if (memFree < s_memNonCriticalLimit) {
            critical = memFree < s_memCriticalLimit;
            break;
        }

        sleep(s_pollTimeSec);
    } while (true);

    if (ReliefLogger::loggingEnabled())
        LOG(MemoryPressure, "Polled memory pressure (%s)", critical ? "critical" : "non-critical");

    MemoryPressureHandler::singleton().setUnderMemoryPressure(critical);
    callOnMainThread([critical] {
        MemoryPressureHandler::singleton().respondToMemoryPressure(critical ? Critical::Yes : Critical::No);
    });
}

inline void MemoryPressureHandler::logErrorAndCloseFDs(const char* log)
{
    if (log)
        LOG(MemoryPressure, "%s, error : %m", log);

    if (m_eventFD) {
        close(m_eventFD.value());
        m_eventFD = std::nullopt;
    }
    if (m_pressureLevelFD) {
        close(m_pressureLevelFD.value());
        m_pressureLevelFD = std::nullopt;
    }
}

bool MemoryPressureHandler::tryEnsureEventFD()
{
    // If the env var to use the poll method based on meminfo is set, this method overrides anything else.
    if (m_eventFD != -1 && defaultPollMaximumProcessMemory(s_pollMaximumProcessMemoryCriticalLimit, s_pollMaximumProcessMemoryNonCriticalLimit)) {
        m_eventFD = -1;
        return true;
    }

    if (m_eventFD)
        return true;

    auto setupCgroups = [this]() -> bool {
        int fd = eventfd(0, EFD_CLOEXEC);
        if (fd == -1) {
            LOG(MemoryPressure, "eventfd() failed: %m");
            return false;
        }
        m_eventFD = fd;

        fd = open(s_cgroupMemoryPressureLevel, O_CLOEXEC | O_RDONLY);
        if (fd == -1) {
            logErrorAndCloseFDs("Failed to open memory.pressure_level");
            return false;
        }
        m_pressureLevelFD = fd;

        fd = open(s_cgroupEventControl, O_CLOEXEC | O_WRONLY);
        if (fd == -1) {
            logErrorAndCloseFDs("Failed to open cgroup.event_control");
            return false;
        }

        char line[128] = {0, };
        if (snprintf(line, sizeof(line), "%d %d low", m_eventFD.value(), m_pressureLevelFD.value()) < 0
            || write(fd, line, strlen(line) + 1) < 0) {
            logErrorAndCloseFDs("Failed to write cgroup.event_control");
            close(fd);
            return false;
        }

        return true;
    };

    if (setupCgroups())
        return true;

    return false;
}

void MemoryPressureHandler::install()
{
    if (m_installed || m_holdOffTimer.isActive())
        return;

    if (!tryEnsureEventFD())
        return;

    m_eventFDPoller = std::make_unique<EventFDPoller>(m_eventFD.value(), [this] {
        // FIXME: Current memcg does not provide any way for users to know how serious the memory pressure is.
        // So we assume all notifications from memcg are critical for now. If memcg had better inferfaces
        // to get a detailed memory pressure level in the future, we should update here accordingly.
        bool critical = true;
        if (ReliefLogger::loggingEnabled())
            LOG(MemoryPressure, "Got memory pressure notification (%s)", critical ? "critical" : "non-critical");

        setUnderMemoryPressure(critical);
        if (isMainThread())
            respondToMemoryPressure(critical ? Critical::Yes : Critical::No);
        else
            RunLoop::main().dispatch([this, critical] { respondToMemoryPressure(critical ? Critical::Yes : Critical::No); });
    });

    if (ReliefLogger::loggingEnabled() && isUnderMemoryPressure())
        LOG(MemoryPressure, "System is no longer under memory pressure.");

    setUnderMemoryPressure(false);
    m_installed = true;
}

void MemoryPressureHandler::uninstall()
{
    if (!m_installed)
        return;

    m_holdOffTimer.stop();
    m_eventFDPoller = nullptr;

    if (m_pressureLevelFD) {
        close(m_pressureLevelFD.value());
        m_pressureLevelFD = std::nullopt;

        // Only close the eventFD used for cgroups.
        if (m_eventFD) {
            close(m_eventFD.value());
            m_eventFD = std::nullopt;
        }
    }

    m_installed = false;
}

void MemoryPressureHandler::holdOffTimerFired()
{
    install();
}

void MemoryPressureHandler::holdOff(unsigned seconds)
{
    m_holdOffTimer.startOneShot(seconds);
}

static size_t processMemoryUsage()
{
    ProcessMemoryStatus memoryStatus;
    currentProcessMemoryStatus(memoryStatus);
    return (memoryStatus.resident - memoryStatus.shared);
}

void MemoryPressureHandler::respondToMemoryPressure(Critical critical, Synchronous synchronous)
{
    uninstall();

    double startTime = monotonicallyIncreasingTime();
    int64_t processMemory = processMemoryUsage();
    releaseMemory(critical, synchronous);
    int64_t bytesFreed = processMemory - processMemoryUsage();
    unsigned holdOffTime = s_maximumHoldOffTime;
    if (bytesFreed > 0 && static_cast<size_t>(bytesFreed) >= s_minimumBytesFreedToUseMinimumHoldOffTime)
        holdOffTime = (monotonicallyIncreasingTime() - startTime) * s_holdOffMultiplier;
    holdOff(std::max(holdOffTime, s_minimumHoldOffTime));
}

void MemoryPressureHandler::platformReleaseMemory(Critical)
{
#ifdef __GLIBC__
    malloc_trim(0);
#endif
}

std::optional<MemoryPressureHandler::ReliefLogger::MemoryUsage> MemoryPressureHandler::ReliefLogger::platformMemoryUsage()
{
    return MemoryUsage {processMemoryUsage(), 0};
}

void MemoryPressureHandler::setMemoryPressureMonitorHandle(int fd)
{
    ASSERT(!m_eventFD);
    m_eventFD = fd;
}

} // namespace WebCore

#endif // OS(LINUX)
