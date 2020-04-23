/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined (USE_BREAKPAD)
#include "config.h"
#include "BreakpadExceptionHandler.h"
#include <client/linux/handler/exception_handler.h>
#include <signal.h>

namespace WebKit
{
// called by 'google_breakpad::ExceptionHandler' on every crash
static bool breakpadCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
  (void) descriptor;
  (void) context;
  return succeeded;
}

void installExceptionHandler()
{
#ifdef SIGPIPE
  signal (SIGPIPE, SIG_IGN);
#endif
  static google_breakpad::ExceptionHandler* excHandler = NULL;
  delete excHandler;
  const char* BREAKPAD_MINIDUMP_DIR = "/opt/minidumps";
  excHandler = new google_breakpad::ExceptionHandler(google_breakpad::MinidumpDescriptor(BREAKPAD_MINIDUMP_DIR), NULL, breakpadCallback, NULL, true, -1);
}
}
#endif

