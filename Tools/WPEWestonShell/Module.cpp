/*
 * Copyright (C) 2014 Igalia S.L.
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

#include "Environment.h"
#include "Shell.h"
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>

struct TestShellLaunch {
    using EntryPoint = void(*)(int, char**);
    EntryPoint launchFunction;
    int argc;
    char** argv;
};

static gpointer launchTestShell(gpointer data)
{
    GMainContext* threadContext = g_main_context_new();
    GMainLoop* threadLoop = g_main_loop_new(threadContext, FALSE);

    g_main_context_push_thread_default(threadContext);

    auto& launchData = *reinterpret_cast<TestShellLaunch*>(data);

    launchData.launchFunction(launchData.argc, launchData.argv);
    return nullptr;
}

static void loadTestShell(const char* shellPath, int argc, char** argv)
{
    void* shell = dlopen(shellPath, RTLD_NOW);
    if (!shell) {
        std::fprintf(stderr, "Failed to open test shell: %s\n", dlerror());
        return;
    }

    auto launchFunction = reinterpret_cast<TestShellLaunch::EntryPoint>(dlsym(shell, "WPETestShellLaunch"));
    if (!launchFunction) {
        std::fprintf(stderr, "Failed to load the WPETestShellLaunch entrypoint: %s\n", dlerror());
        return;
    }

    g_thread_new("WPE Test Shell", launchTestShell,
        new TestShellLaunch{ launchFunction, argc, argv });
}

WL_EXPORT int module_init(struct weston_compositor* compositor, int* argc, char** argv)
{
    auto* environment = new WPE::Environment(compositor);

    const char* testShell = std::getenv("WPE_TEST_SHELL");
    if (testShell)
        loadTestShell(testShell, *argc, argv);
    else
        g_thread_new("WPE Thread", WPE::Shell::launchWPE, new WPE::Shell(*environment));

    // This is a bit rough, but it works. We ensure that
    // all the remaining args have been handled.
    *argc = 1;

    return 0;
}
