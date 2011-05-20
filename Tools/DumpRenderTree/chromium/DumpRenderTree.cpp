/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "TestShell.h"
#include "webkit/support/webkit_support.h"
#include <v8/include/v8-testing.h>
#include <v8/include/v8.h>
#include <wtf/Vector.h>

using namespace std;

static const char optionComplexText[] = "--complex-text";
static const char optionDumpAllPixels[] = "--dump-all-pixels";
static const char optionNotree[] = "--notree";
static const char optionPixelTests[] = "--pixel-tests";
static const char optionThreaded[] = "--threaded";
static const char optionDebugRenderTree[] = "--debug-render-tree";
static const char optionDebugLayerTree[] = "--debug-layer-tree";

static const char optionPixelTestsWithName[] = "--pixel-tests=";
static const char optionTestShell[] = "--test-shell";
static const char optionAllowExternalPages[] = "--allow-external-pages";
static const char optionStartupDialog[] = "--testshell-startup-dialog";
static const char optionCheckLayoutTestSystemDeps[] = "--check-layout-test-sys-deps";

static const char optionHardwareAcceleratedGL[] = "--enable-hardware-gpu";
static const char optionEnableAcceleratedCompositing[] = "--enable-accelerated-compositing";
static const char optionForceCompositingMode[] = "--force-compositing-mode";
static const char optionEnableAccelerated2DCanvas[] = "--enable-accelerated-2d-canvas";
static const char optionEnableLegacyAccelerated2DCanvas[] = "--enable-legacy-accelerated-2d-canvas";
static const char optionEnableAcceleratedDrawing[] = "--enable-accelerated-drawing";
static const char optionEnableCompositeToTexture[] = "--enable-composite-to-texture";
static const char optionUseGraphicsContext3DImplementation[] = "--use-graphics-context-3d-implementation=";

static const char optionStressOpt[] = "--stress-opt";
static const char optionStressDeopt[] = "--stress-deopt";
static const char optionJavaScriptFlags[] = "--js-flags=";
static const char optionNoTimeout[] = "--no-timeout";

class WebKitSupportTestEnvironment {
public:
    WebKitSupportTestEnvironment()
    {
        webkit_support::SetUpTestEnvironment();
    }
    ~WebKitSupportTestEnvironment()
    {
        webkit_support::TearDownTestEnvironment();
    }
};

static void runTest(TestShell& shell, TestParams& params, const string& testName, bool testShellMode)
{
    int oldTimeoutMsec = shell.layoutTestTimeout();
    params.pixelHash = "";
    string pathOrURL = testName;
    if (testShellMode) {
        string timeOut;
        string::size_type separatorPosition = pathOrURL.find(' ');
        if (separatorPosition != string::npos) {
            timeOut = pathOrURL.substr(separatorPosition + 1);
            pathOrURL.erase(separatorPosition);
            separatorPosition = timeOut.find_first_of(' ');
            if (separatorPosition != string::npos) {
                params.pixelHash = timeOut.substr(separatorPosition + 1);
                timeOut.erase(separatorPosition);
            }
            shell.setLayoutTestTimeout(atoi(timeOut.c_str()));
        }
    } else {
        string::size_type separatorPosition = pathOrURL.find("'");
        if (separatorPosition != string::npos) {
            params.pixelHash = pathOrURL.substr(separatorPosition + 1);
            pathOrURL.erase(separatorPosition);
        }
    }
    params.testUrl = webkit_support::CreateURLForPathOrURL(pathOrURL);
    webkit_support::SetCurrentDirectoryForFileURL(params.testUrl);
    v8::V8::SetFlagsFromString(shell.javaScriptFlags().c_str(), shell.javaScriptFlags().length());
    if (shell.stressOpt() || shell.stressDeopt()) {
      if (shell.stressOpt())
        v8::Testing::SetStressRunType(v8::Testing::kStressTypeOpt);
      else
        v8::Testing::SetStressRunType(v8::Testing::kStressTypeDeopt);
      for (int i = 0; i < v8::Testing::GetStressRuns(); i++) {
          v8::Testing::PrepareStressRun(i);
          bool isLastLoad = (i == (v8::Testing::GetStressRuns() - 1));
          shell.setDumpWhenFinished(isLastLoad);
          shell.resetTestController();
          shell.runFileTest(params);
      }
    } else {
      shell.resetTestController();
      shell.runFileTest(params);
    }
    shell.setLayoutTestTimeout(oldTimeoutMsec);
}

int main(int argc, char* argv[])
{
    WebKitSupportTestEnvironment testEnvironment;
    platformInit(&argc, &argv);

    TestParams params;
    Vector<string> tests;
    bool serverMode = false;
    bool testShellMode = false;
    bool allowExternalPages = false;
    bool startupDialog = false;
    bool acceleratedCompositingEnabled = false;
    bool compositeToTexture = false;
    bool forceCompositingMode = false;
    bool accelerated2DCanvasEnabled = false;
    bool legacyAccelerated2DCanvasEnabled = false;
    bool acceleratedDrawingEnabled = false;
    bool stressOpt = false;
    bool stressDeopt = false;
    bool hardwareAcceleratedGL = false;
    string javaScriptFlags;
    bool noTimeout = false;
    for (int i = 1; i < argc; ++i) {
        string argument(argv[i]);
        if (argument == "-")
            serverMode = true;
        else if (argument == optionNotree)
            params.dumpTree = false;
        else if (argument == optionPixelTests)
            params.dumpPixels = true;
        else if (!argument.find(optionPixelTestsWithName)) {
            params.dumpPixels = true;
            params.pixelFileName = argument.substr(strlen(optionPixelTestsWithName));
        } else if (argument == optionDebugRenderTree)
            params.debugRenderTree = true;
        else if (argument == optionDebugLayerTree)
            params.debugLayerTree = true;
        else if (argument == optionTestShell) {
            testShellMode = true;
            serverMode = true;
        } else if (argument == optionAllowExternalPages)
            allowExternalPages = true;
        else if (argument == optionStartupDialog)
            startupDialog = true;
        else if (argument == optionCheckLayoutTestSystemDeps)
            return checkLayoutTestSystemDependencies() ? EXIT_SUCCESS : EXIT_FAILURE;
        else if (argument == optionHardwareAcceleratedGL)
            hardwareAcceleratedGL = true;
        else if (argument == optionEnableAcceleratedCompositing)
            acceleratedCompositingEnabled = true;
        else if (argument == optionEnableCompositeToTexture)
            compositeToTexture = true;
        else if (argument == optionForceCompositingMode)
            forceCompositingMode = true;
        else if (argument == optionEnableAccelerated2DCanvas)
            accelerated2DCanvasEnabled = true;
        else if (argument == optionEnableLegacyAccelerated2DCanvas)
            legacyAccelerated2DCanvasEnabled = true;
        else if (argument == optionEnableAcceleratedDrawing)
            acceleratedDrawingEnabled = true;
        else if (!argument.find(optionUseGraphicsContext3DImplementation)) {
            string implementation = argument.substr(strlen(optionUseGraphicsContext3DImplementation));
            if (!implementation.compare("IN_PROCESS")) 
              webkit_support::SetGraphicsContext3DImplementation(webkit_support::IN_PROCESS);
            else if (!implementation.compare("IN_PROCESS_COMMAND_BUFFER")) 
              webkit_support::SetGraphicsContext3DImplementation(webkit_support::IN_PROCESS_COMMAND_BUFFER);
            else 
              fprintf(stderr, "Unknown GraphicContext3D implementation %s\n", implementation.c_str());
        } else if (argument == optionStressOpt)
            stressOpt = true;
        else if (argument == optionStressDeopt)
            stressDeopt = true;
        else if (!argument.find(optionJavaScriptFlags))
            javaScriptFlags = argument.substr(strlen(optionJavaScriptFlags));
        else if (argument == optionNoTimeout)
            noTimeout = true;
        else if (argument.size() && argument[0] == '-')
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
        else
            tests.append(argument);
    }
    if (testShellMode && params.dumpPixels && params.pixelFileName.empty()) {
        fprintf(stderr, "--pixel-tests with --test-shell requires a file name.\n");
        return EXIT_FAILURE;
    }
    if (stressOpt && stressDeopt) {
        fprintf(stderr, "--stress-opt and --stress-deopt are mutually exclusive.\n");
        return EXIT_FAILURE;
    }

    webkit_support::SetUpGLBindings(hardwareAcceleratedGL ? webkit_support::GL_BINDING_DEFAULT : webkit_support::GL_BINDING_SOFTWARE_RENDERER);

    if (startupDialog)
        openStartupDialog();

    { // Explicit scope for the TestShell instance.
        TestShell shell(testShellMode);
        shell.setAllowExternalPages(allowExternalPages);
        shell.setAcceleratedCompositingEnabled(acceleratedCompositingEnabled);
        shell.setCompositeToTexture(compositeToTexture);
        shell.setForceCompositingMode(forceCompositingMode);
        shell.setAccelerated2dCanvasEnabled(accelerated2DCanvasEnabled);
        shell.setLegacyAccelerated2dCanvasEnabled(legacyAccelerated2DCanvasEnabled);
        shell.setAcceleratedDrawingEnabled(acceleratedDrawingEnabled);
        shell.setJavaScriptFlags(javaScriptFlags);
        shell.setStressOpt(stressOpt);
        shell.setStressDeopt(stressDeopt);
        if (noTimeout) {
            // 0x20000000ms is big enough for the purpose to avoid timeout in debugging.
            shell.setLayoutTestTimeout(0x20000000);
        }
        if (serverMode && !tests.size()) {
            params.printSeparators = true;
            char testString[2048]; // 2048 is the same as the sizes of other platforms.
            while (fgets(testString, sizeof(testString), stdin)) {
                char* newLinePosition = strchr(testString, '\n');
                if (newLinePosition)
                    *newLinePosition = '\0';
                if (testString[0] == '\0')
                    continue;
                runTest(shell, params, testString, testShellMode);
            }
        } else if (!tests.size())
            printf("#EOF\n");
        else {
            params.printSeparators = tests.size() > 1;
            for (unsigned i = 0; i < tests.size(); i++)
                runTest(shell, params, tests[i], testShellMode);
        }

        shell.callJSGC();
        shell.callJSGC();

        // When we finish the last test, cleanup the LayoutTestController.
        // It may have references to not-yet-cleaned up windows. By cleaning up
        // here we help purify reports.
        shell.resetTestController();
    }

    return EXIT_SUCCESS;
}
