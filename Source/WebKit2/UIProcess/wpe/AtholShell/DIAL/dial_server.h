/*
 * Copyright (c) 2014 Netflix, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NETFLIX, INC. AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NETFLIX OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DIAL_SERVER_H_
#define DIAL_SERVER_H_

#include <netinet/in.h>

/*
 * Dial application states
 */
typedef enum {
    kDIALStatusStopped,
    kDIALStatusRunning
} DIALStatus;

/*
 * DIAL version that is reported via in the status response.
 */
#define DIAL_VERSION ("\"1.7\"")

/*
 * The maximum DIAL payload accepted per the DIAL 1.6.1 specification.
 */
#define DIAL_MAX_PAYLOAD (4096)

/*
 * Opaque DIAL server handle
 */
struct DIALServer_;
typedef struct DIALServer_ DIALServer;

/*
 * Opaque run id that can be system specific
 */
typedef void * DIAL_run_t;

/*
 * DIAL start callback
 */
typedef DIALStatus (*DIAL_app_start_cb)(DIALServer *ds, const char *app_name,
                                        const char *args, size_t arglen,
                                        DIAL_run_t *run_id, void *callback_data);
/*
 * DIAL stop callback
 */
typedef void (*DIAL_app_stop_cb)(DIALServer *ds, const char *app_name,
                                 DIAL_run_t run_id, void *callback_data);
/*
 * DIAL status callback
 */
typedef DIALStatus (*DIAL_app_status_cb)(DIALServer *ds, const char *app_name,
                                         DIAL_run_t run_id, int* pCanStop,
                                         void *callback_data);

/*
 * DIAL callbacks
 */
struct DIALAppCallbacks {
    DIAL_app_start_cb start_cb;
    DIAL_app_stop_cb stop_cb;
    DIAL_app_status_cb status_cb;
};

/*
 * Creates the DIAL server.  Returns a handle to the DIAL server.
 */
#ifdef __cplusplus
extern "C"
#endif
DIALServer *DIAL_create();

/*
 * Starts the DIAL server.
 *
 * @param[in] ds DIAL server handle
 */
#ifdef __cplusplus
extern "C"
#endif
void DIAL_start(DIALServer *ds);

/*
 * Stop the DIAL server.
 *
 * @param[in] ds DIAL server handle
 */
#ifdef __cplusplus
extern "C"
#endif
void DIAL_stop(DIALServer *ds);

/*
 * Register a DIAL application
 *
 * @param[in] ds DIAL server handle
 * @param[in] app_name Name of the application.  This should match the DIAL
 * application end point.
 * @param[in] callbacks Structure with application callbacks
 * @param[in] callback_data Client user data
 * @param[in] if non-0, the app supports DIALadditionalDataURL.
 * @param[in] if non-NULL, specifies the CORS allowed origin for this app.
 *
 * @return 1 if successful, 0 otherwise
 */
#ifdef __cplusplus
extern "C"
#endif
int DIAL_register_app(DIALServer *ds, const char *app_name,
                      struct DIALAppCallbacks *callbacks,
                      void *callback_data, int useAdditionalData,
                      const char* corsAllowedOrigin);

/*
 * Unregsiter an application
 *
 * @param[in] ds DIAL server handle
 * @param[in] app_name Name of the DIAL application
 *
 * @return 1 if successful, 0 otherwise
 */
#ifdef __cplusplus
extern "C"
#endif
int DIAL_unregister_app(DIALServer *ds, const char *app_name);

/*
 * Get the DIAL REST endpoint
 *
 * @return Port number of the DIAL rest endpoint.  Returns 0 on error.
 */
#ifdef __cplusplus
extern "C"
#endif
in_port_t DIAL_get_port(DIALServer *ds);

/*
 * Get the last payload delivered to an application.  This can be used
 * by application clients to see if the payload changed between lauches.
 *
 * @param[in] ds DIAL server handle
 * @param[in] app_name Name of the application
 *
 * @return Pointer to a NULL terminated string.
 */
const char * DIAL_get_payload(DIALServer *ds, const char *app_name);

#endif  // DIAL_SERVER_H_
