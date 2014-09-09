/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#import <CoreFoundation/CoreFoundation.h>
#import "XPCPtr.h"

namespace WebKit {

static void XPCServiceEventHandler(xpc_connection_t peer)
{
    xpc_connection_set_target_queue(peer, dispatch_get_main_queue());
    xpc_connection_set_event_handler(peer, ^(xpc_object_t event) {
        xpc_type_t type = xpc_get_type(event);
        if (type == XPC_TYPE_ERROR) {
            if (event == XPC_ERROR_CONNECTION_INVALID || event == XPC_ERROR_TERMINATION_IMMINENT) {
                // FIXME: Handle this case more gracefully.
                exit(EXIT_FAILURE);
            }
        } else {
            assert(type == XPC_TYPE_DICTIONARY);

            if (!strcmp(xpc_dictionary_get_string(event, "message-name"), "bootstrap")) {
                CFBundleRef webKitBundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.WebKit"));
                CFStringRef entryPointFunctionName = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("WebKitEntryPoint"));

                typedef void (*InitializerFunction)(xpc_connection_t, xpc_object_t);
                InitializerFunction initializerFunctionPtr = reinterpret_cast<InitializerFunction>(CFBundleGetFunctionPointerForName(webKitBundle, entryPointFunctionName));
                if (!initializerFunctionPtr) {
                    NSLog(@"Unable to find entry point in WebKit.framework with name: %@", (NSString *)entryPointFunctionName);
                    exit(EXIT_FAILURE);
                }

                auto reply = IPC::adoptXPC(xpc_dictionary_create_reply(event));
                xpc_dictionary_set_string(reply.get(), "message-name", "process-finished-launching");
                xpc_connection_send_message(xpc_dictionary_get_remote_connection(event), reply.get());

                initializerFunctionPtr(peer, event);
            }

            // Leak a boost onto the NetworkProcess.
            if (!strcmp(xpc_dictionary_get_string(event, "message-name"), "pre-bootstrap"))
                xpc_retain(event);
        }
    });

    xpc_connection_resume(peer);
}

} // namespace WebKit

using namespace WebKit;

int main(int argc, char** argv)
{
    xpc_main(XPCServiceEventHandler);
    return 0;
}
