/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CommandLine.h"

#include "PluginProcessMain.h"
#include "ProcessLauncher.h"
#include "WebProcessMain.h"
#include <wtf/text/CString.h>

#if PLATFORM(MAC)
#include <objc/objc-auto.h>
#elif PLATFORM(WIN)
#include <shlwapi.h>
#endif

using namespace WebKit;

static int WebKitMain(const CommandLine& commandLine)
{
    ProcessLauncher::ProcessType processType;    
    if (!ProcessLauncher::getProcessTypeFromString(commandLine["type"].utf8().data(), processType))
        return EXIT_FAILURE;

    switch (processType) {
        case ProcessLauncher::WebProcess:
            return WebProcessMain(commandLine);
        case ProcessLauncher::PluginProcess:
#if ENABLE(PLUGIN_PROCESS)
            return PluginProcessMain(commandLine);
#else
            break;
#endif
    }

    return EXIT_FAILURE;
}

#if PLATFORM(MAC)

extern "C" WK_EXPORT int WebKitMain(int argc, char** argv);

int WebKitMain(int argc, char** argv)
{
    ASSERT(!objc_collectingEnabled());
    
    CommandLine commandLine;
    if (!commandLine.parse(argc, argv))
        return EXIT_FAILURE;
    
    return WebKitMain(commandLine);
}

#elif PLATFORM(WIN)

static void enableDataExecutionPrevention()
{
    // Enable Data Execution prevention at runtime rather than via /NXCOMPAT
    // http://blogs.msdn.com/michael_howard/archive/2008/01/29/new-nx-apis-added-to-windows-vista-sp1-windows-xp-sp3-and-windows-server-2008.aspx

    const DWORD enableDEP = 0x00000001;

    HMODULE hMod = ::GetModuleHandleW(L"Kernel32.dll");
    if (!hMod)
        return;

    typedef BOOL (WINAPI *PSETDEP)(DWORD);

    PSETDEP procSet = reinterpret_cast<PSETDEP>(::GetProcAddress(hMod, "SetProcessDEPPolicy"));
    if (!procSet)
        return;

    // Enable Data Execution Prevention, but allow ATL thunks (for compatibility with the version of ATL that ships with the Platform SDK).
    procSet(enableDEP);
}

static void enableTerminationOnHeapCorruption()
{
    // Enable termination on heap corruption on OSes that support it (Vista and XPSP3).
    // http://msdn.microsoft.com/en-us/library/aa366705(VS.85).aspx

    const HEAP_INFORMATION_CLASS heapEnableTerminationOnCorruption = static_cast<HEAP_INFORMATION_CLASS>(1);

    HMODULE hMod = ::GetModuleHandleW(L"kernel32.dll");
    if (!hMod)
        return;

    typedef BOOL (WINAPI*HSI)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
    HSI heapSetInformation = reinterpret_cast<HSI>(::GetProcAddress(hMod, "HeapSetInformation"));
    if (!heapSetInformation)
        return;

    heapSetInformation(0, heapEnableTerminationOnCorruption, 0, 0);
}

static void disableUserModeCallbackExceptionFilter()
{
    const DWORD PROCESS_CALLBACK_FILTER_ENABLED = 0x1;
    typedef BOOL (NTAPI *getProcessUserModeExceptionPolicyPtr)(LPDWORD lpFlags);
    typedef BOOL (NTAPI *setProcessUserModeExceptionPolicyPtr)(DWORD dwFlags);

    HMODULE lib = LoadLibrary(TEXT("kernel32.dll"));
    ASSERT(lib);

    getProcessUserModeExceptionPolicyPtr getPolicyPtr = (getProcessUserModeExceptionPolicyPtr)GetProcAddress(lib, "GetProcessUserModeExceptionPolicy");
    setProcessUserModeExceptionPolicyPtr setPolicyPtr = (setProcessUserModeExceptionPolicyPtr)GetProcAddress(lib, "SetProcessUserModeExceptionPolicy");

    DWORD dwFlags;
    if (!getPolicyPtr || !setPolicyPtr || !getPolicyPtr(&dwFlags)) {
        FreeLibrary(lib);
        return;
    }

    // If this flag isn't cleared, exceptions that are thrown when running in a 64-bit version of
    // Windows are ignored, possibly leaving Safari in an inconsistent state that could cause an 
    // unrelated exception to be thrown.
    // http://support.microsoft.com/kb/976038
    // http://blog.paulbetts.org/index.php/2010/07/20/the-case-of-the-disappearing-onload-exception-user-mode-callback-exceptions-in-x64/
    setPolicyPtr(dwFlags & ~PROCESS_CALLBACK_FILTER_ENABLED);

    FreeLibrary(lib);
}

#ifndef NDEBUG
static void pauseProcessIfNeeded(HMODULE module)
{
    // Show an alert when Ctrl-Alt-Shift is held down during launch to give the user time to attach a
    // debugger. This is useful for debugging problems that happen early in the web process's lifetime.
    const unsigned short highBitMaskShort = 0x8000;
    if (!getenv("WEBKIT2_PAUSE_WEB_PROCESS_ON_LAUNCH") && !((::GetKeyState(VK_CONTROL) & highBitMaskShort) && (::GetKeyState(VK_MENU) & highBitMaskShort) && (::GetKeyState(VK_SHIFT) & highBitMaskShort)))
        return;

    wchar_t path[MAX_PATH];
    DWORD length = ::GetModuleFileNameW(module, path, WTF_ARRAY_LENGTH(path));
    if (!length || length == WTF_ARRAY_LENGTH(path))
        return;

    wchar_t* startOfFilename = ::PathFindFileNameW(path);
    String filenameString(startOfFilename, length - (startOfFilename - path));

    String message = L"You can now attach a debugger to " + filenameString + L". You can use the same debugger for " + filenameString + L" and the UI process, if desired. Click OK when you are ready for " + filenameString + L" to continue.";
    String title = filenameString + L" has launched";
    ::MessageBoxW(0, message.charactersWithNullTermination(), title.charactersWithNullTermination(), MB_OK | MB_ICONINFORMATION);
}
#endif

extern "C" __declspec(dllexport) 
int WebKitMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpstrCmdLine, int nCmdShow)
{
#ifndef NDEBUG
    pauseProcessIfNeeded(hInstance);
#endif

    enableDataExecutionPrevention();

    enableTerminationOnHeapCorruption();

    disableUserModeCallbackExceptionFilter();

    CommandLine commandLine;
    if (!commandLine.parse(lpstrCmdLine))
        return EXIT_FAILURE;

    return WebKitMain(commandLine);
}

#endif
