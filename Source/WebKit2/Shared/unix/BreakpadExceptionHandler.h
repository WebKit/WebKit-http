#ifndef BreakpadExceptionHandler_h
#define BreakpadExceptionHandler_h

#if defined (USE_BREAKPAD)
#include "config.h"
#include <client/linux/handler/exception_handler.h>

namespace
{
// called by 'google_breakpad::ExceptionHandler' on every crash
bool breakpadCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
  (void) descriptor;
  (void) context;
  return succeeded;
}

void installExceptionHandler()
{
  static google_breakpad::ExceptionHandler* excHandler = NULL;
  delete excHandler;
  const char* BREAKPAD_MINIDUMP_DIR = "/opt/minidumps";
  excHandler = new google_breakpad::ExceptionHandler(google_breakpad::MinidumpDescriptor(BREAKPAD_MINIDUMP_DIR), NULL, breakpadCallback, NULL, true, -1);
}
}
#endif

#endif // BreakpadExceptionHandler_h
