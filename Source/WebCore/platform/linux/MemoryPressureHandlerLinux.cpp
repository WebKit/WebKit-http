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

#include "Logging.h"

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

namespace WebCore {

// Disable memory event reception for a minimum of s_minimumHoldOffTime
// seconds after receiving an event. Don't let events fire any sooner than
// s_holdOffMultiplier times the last cleanup processing time. Effectively
// this is 1 / s_holdOffMultiplier percent of the time.
// These value seems reasonable and testing verifies that it throttles frequent
// low memory events, greatly reducing CPU usage.
static const unsigned s_minimumHoldOffTime = 5;
static const unsigned s_holdOffMultiplier = 20;
static const unsigned s_pollTimeSec = 1;
static const size_t s_memCriticalLimit = 30 * KB * KB; // 30 MB
static const size_t s_memNonCriticalLimit = 70 * KB * KB; // 70 MB
static size_t s_pollMaximumProcessMemoryCriticalLimit = 0;
static size_t s_pollMaximumProcessMemoryNonCriticalLimit = 0;

static const char* s_cgroupMemoryPressureLevel = "/sys/fs/cgroup/memory/memory.pressure_level";
static const char* s_cgroupEventControl = "/sys/fs/cgroup/memory/cgroup.event_control";
static const char* s_processStatus = "/proc/self/status";
static const char* s_memInfo = "/proc/meminfo";
static const char* s_cmdline = "/proc/self/cmdline";


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

void MemoryPressureHandler::waitForMemoryPressureEvent(void*)
{
    ASSERT(!isMainThread());
    int eventFD = MemoryPressureHandler::singleton().m_eventFD;
    if (!eventFD) {
        LOG(MemoryPressure, "Invalidate eventfd.");
        return;
    }

    uint64_t buffer;
    if (read(eventFD, &buffer, sizeof(buffer)) <= 0) {
        LOG(MemoryPressure, "Failed to read eventfd.");
        return;
    }

    // FIXME: Current memcg does not provide any way for users to know how serious the memory pressure is.
    // So we assume all notifications from memcg are critical for now. If memcg had better inferfaces
    // to get a detailed memory pressure level in the future, we should update here accordingly.
    bool critical = true;
    if (ReliefLogger::loggingEnabled())
        LOG(MemoryPressure, "Got memory pressure notification (%s)", critical ? "critical" : "non-critical");

    MemoryPressureHandler::singleton().setUnderMemoryPressure(critical);
    callOnMainThread([critical] {
        MemoryPressureHandler::singleton().respondToMemoryPressure(critical ? Critical::Yes : Critical::No);
    });
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
                nonCriticalLimit = criticalLimit * 0.75;
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
        } else {
            size_t memFree = readToken(s_memInfo, "MemFree:", KB);

            if (!memFree)
                return;

            if (memFree < s_memNonCriticalLimit) {
                critical = memFree < s_memCriticalLimit;
                break;
            }
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
        close(m_eventFD);
        m_eventFD = 0;
    }
    if (m_pressureLevelFD) {
        close(m_pressureLevelFD);
        m_pressureLevelFD = 0;
    }
}

void MemoryPressureHandler::install()
{
    if (m_installed)
        return;

    bool cgroupsPressureHandlerOk = false;

    do {
        m_eventFD = eventfd(0, EFD_CLOEXEC);
        if (m_eventFD == -1) {
            LOG(MemoryPressure, "eventfd() failed: %m");
            break;
        }

        m_pressureLevelFD = open(s_cgroupMemoryPressureLevel, O_CLOEXEC | O_RDONLY);
        if (m_pressureLevelFD == -1) {
            logErrorAndCloseFDs("Failed to open memory.pressure_level");
            break;
        }

        int fd = open(s_cgroupEventControl, O_CLOEXEC | O_WRONLY);
        if (fd == -1) {
            logErrorAndCloseFDs("Failed to open cgroup.event_control");
            break;
        }

        char line[128] = {0, };
        if (snprintf(line, sizeof(line), "%d %d low", m_eventFD, m_pressureLevelFD) < 0
            || write(fd, line, strlen(line) + 1) < 0) {
            logErrorAndCloseFDs("Failed to write cgroup.event_control");
            close(fd);
            break;
        }
        close(fd);

        m_threadID = createThread(waitForMemoryPressureEvent, this, "WebCore: MemoryPressureHandler");
        if (!m_threadID) {
            logErrorAndCloseFDs("Failed to create a thread for MemoryPressureHandler");
            break;
        }

        LOG(MemoryPressure, "Cgroups memory pressure handler installed.");
        cgroupsPressureHandlerOk = true;
    } while (false);

    bool vmstatPressureHandlerOk = false;

    // If cgroups isn't available, try to use a simpler poll method based on meminfo.
    if (!cgroupsPressureHandlerOk) {
        defaultPollMaximumProcessMemory(s_pollMaximumProcessMemoryCriticalLimit, s_pollMaximumProcessMemoryNonCriticalLimit);
        do {
            m_threadID = createThread(pollMemoryPressure, this, "WebCore: MemoryPressureHandler");
            if (!m_threadID) {
                logErrorAndCloseFDs("Failed to create a thread for MemoryPressureHandler");
                break;
            }

            LOG(MemoryPressure, "Vmstat memory pressure handler installed.");
            vmstatPressureHandlerOk = true;
        } while (false);
    }

    if (!cgroupsPressureHandlerOk && !vmstatPressureHandlerOk)
        return;

    if (ReliefLogger::loggingEnabled() && isUnderMemoryPressure())
        LOG(MemoryPressure, "System is no longer under memory pressure.");

    setUnderMemoryPressure(false);
    m_installed = true;
}

void MemoryPressureHandler::uninstall()
{
    if (!m_installed)
        return;

    if (m_threadID) {
        detachThread(m_threadID);
        m_threadID = 0;
    }

    logErrorAndCloseFDs(nullptr);
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

void MemoryPressureHandler::respondToMemoryPressure(Critical critical, Synchronous synchronous)
{
    uninstall();

    double startTime = monotonicallyIncreasingTime();
    m_lowMemoryHandler(critical, synchronous);
    unsigned holdOffTime = (monotonicallyIncreasingTime() - startTime) * s_holdOffMultiplier;
    holdOff(std::max(holdOffTime, s_minimumHoldOffTime));
}

void MemoryPressureHandler::platformReleaseMemory(Critical)
{
#ifdef __GLIBC__
    ReliefLogger log("Run malloc_trim");
    malloc_trim(0);
#endif
}

size_t MemoryPressureHandler::ReliefLogger::platformMemoryUsage()
{
    FILE* file = fopen(s_processStatus, "r");
    if (!file)
        return static_cast<size_t>(-1);

    size_t vmSize = static_cast<size_t>(-1); // KB
    String token = nextToken(file);
    while (!token.isEmpty()) {
        if (token == "VmSize:") {
            vmSize = nextToken(file).toInt() * KB;
            break;
        }
        token = nextToken(file);
    }
    fclose(file);

    return vmSize;
}

} // namespace WebCore

#endif // OS(LINUX)
