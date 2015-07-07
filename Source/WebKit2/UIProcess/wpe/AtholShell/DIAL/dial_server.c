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

#define _BSD_SOURCE

#include "dial_data.h"
#include "dial_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "mongoose.h"
#include "url_lib.h"

// TODO: Partners should define this port
#define DIAL_PORT (56789)
#define DIAL_DATA_SIZE (8*1024)

static const char *gLocalhost = "127.0.0.1";

struct DIALApp_ {
    struct DIALApp_ *next;
    struct DIALAppCallbacks callbacks;
    struct DIALData_ *dial_data;
    void *callback_data;
    DIAL_run_t run_id;
    DIALStatus state;
    char *name;
    char payload[DIAL_MAX_PAYLOAD];
    int useAdditionalData;
    char corsAllowedOrigin[256];

};

typedef struct DIALApp_ DIALApp;

struct DIALServer_ {
    struct mg_context *ctx;
    struct DIALApp_ *apps;
    pthread_mutex_t mux;
};

static void ds_lock(DIALServer *ds) {
    pthread_mutex_lock(&ds->mux);
}

static void ds_unlock(DIALServer *ds) {
    pthread_mutex_unlock(&ds->mux);
}

// finds an app and returns a pointer to the previous element's next pointer
// if not found, return a pointer to the last element's next pointer
static DIALApp **find_app(DIALServer *ds, const char *app_name) {
    DIALApp *app;
    DIALApp **ret = &ds->apps;

    for (app = ds->apps; app != NULL; ret = &app->next, app = app->next) {
        if (!strcmp(app_name, app->name)) {
            break;
        }
    }
    return ret;
}

static void url_decode_xml_encode(char *dst, char *src, size_t src_size) {
    char *url_decoded_key = (char *) malloc(src_size + 1);
    urldecode(url_decoded_key, src, src_size);
    xmlencode(dst, url_decoded_key, 2 * src_size);
    free(url_decoded_key);
}

/*
 * A bad payload is defined to be an unprintable character or a
 * non-ascii character.
 */
static int isBadPayload(const char* pPayload, int numBytes) {
    int i = 0;
    // fprintf( stderr, "Payload: checking %d bytes\n", numBytes);
    for (; i < numBytes; i++) {
        // High order bit should not be set
        // 0x7F is DEL (non-printable)
        // Anything under 32 is non-printable
        if (((pPayload[i] & 0x80) == 0x80) || (pPayload[i] == 0x7F)
                || (pPayload[i] <= 0x1F))
            return 1;
    }
    return 0;
}

static void handle_app_start(struct mg_connection *conn,
                             const struct mg_request_info *request_info,
                             const char *app_name,
                             const char *origin_header) {
    char additional_data_param[256] = {0, };
    char body[DIAL_MAX_PAYLOAD + sizeof(additional_data_param) + 2] = {0, };
    DIALApp *app;
    DIALServer *ds = request_info->user_data;
    int body_size;

    ds_lock(ds);
    app = *find_app(ds, app_name);
    if (!app) {
        mg_send_http_error(conn, 404, "Not Found", "Not Found");
    } else {
        body_size = mg_read(conn, body, sizeof(body));
        // NUL-terminate it just in case
        if (body_size > DIAL_MAX_PAYLOAD) {
            mg_send_http_error(conn, 413, "413 Request Entity Too Large",
                               "413 Request Entity Too Large");
        } else if (isBadPayload(body, body_size)) {
            mg_send_http_error(conn, 400, "400 Bad Request", "400 Bad Request");
        } else {
            char laddr[INET6_ADDRSTRLEN];
            const struct sockaddr_in *addr =
                    (struct sockaddr_in *) &request_info->local_addr;
            inet_ntop(addr->sin_family, &addr->sin_addr, laddr, sizeof(laddr));
            in_port_t dial_port = DIAL_get_port(ds);

            if (app->useAdditionalData) {
                if (body_size != 0) {
                    strcat(body, "&");
                }
                // Construct additionalDataUrl=http://host:port/apps/app_name/dial_data
                sprintf(additional_data_param,
                        "additionalDataUrl=http%%3A%%2F%%2Flocalhost%%3A%d%%2Fapps%%2F%s%%2Fdial_data%%3F",
                        dial_port, app_name);
                strcat(body, additional_data_param);
                body_size = strlen(body);
            }
            // fprintf(stderr, "Starting the app with params %s\n", body);
            app->state = app->callbacks.start_cb(ds, app_name, body, body_size,
                                                 &app->run_id,
                                                 app->callback_data);
            if (app->state == kDIALStatusRunning) {
                mg_printf(
                        conn,
                        "HTTP/1.1 201 Created\r\n"
                        "Content-Type: text/plain\r\n"
                        "Location: http://%s:%d/apps/%s/run\r\n"
                        "Access-Control-Allow-Origin: %s\r\n"
                        "\r\n",
                        laddr, dial_port, app_name, origin_header);
                // copy the payload into the application struct
                memset(app->payload, 0, DIAL_MAX_PAYLOAD);
                memcpy(app->payload, body, body_size);
            } else {
                mg_send_http_error(conn, 503, "Service Unavailable",
                                   "Service Unavailable");
            }
        }
    }
    ds_unlock(ds);
}

static void handle_app_status(struct mg_connection *conn,
                              const struct mg_request_info *request_info,
                              const char *app_name,
                              const char *origin_header) {
    DIALApp *app;
    int canStop = 0;
    DIALServer *ds = request_info->user_data;

    ds_lock(ds);
    app = *find_app(ds, app_name);
    if (!app) {
        mg_send_http_error(conn, 404, "Not Found", "Not Found");
        ds_unlock(ds);
        return;
    }

    char dial_data[DIAL_DATA_SIZE] = {0,};
    char *end = dial_data + DIAL_DATA_SIZE;
    char *p = dial_data;

    for (DIALData* first = app->dial_data; first != NULL; first = first->next) {
        p = smartstrcat(p, "    <", end - p);
        size_t key_length = strlen(first->key);
        char *encoded_key = (char *) malloc(2 * key_length + 1);
        url_decode_xml_encode(encoded_key, first->key, key_length);

        size_t value_length = strlen(first->value);
        char *encoded_value = (char *) malloc(2 * value_length + 1);
        url_decode_xml_encode(encoded_value, first->value, value_length);

        p = smartstrcat(p, encoded_key, end - p);
        p = smartstrcat(p, ">", end - p);
        p = smartstrcat(p, encoded_value, end - p);
        p = smartstrcat(p, "</", end - p);
        p = smartstrcat(p, encoded_key, end - p);
        p = smartstrcat(p, ">", end - p);
        free(encoded_key);
        free(encoded_value);
    }
    app->state = app->callbacks.status_cb(ds, app_name, app->run_id, &canStop,
                                          app->callback_data);
    mg_printf(
            conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/xml\r\n"
            "Access-Control-Allow-Origin: %s\r\n"
            "\r\n"
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
            "<service xmlns=\"urn:dial-multiscreen-org:schemas:dial\" dialVer=%s>\r\n"
            "  <name>%s</name>\r\n"
            "  <options allowStop=\"%s\"/>\r\n"
            "  <state>%s</state>\r\n"
            "%s"
            "  <additionalData>\n"
            "%s"
            "\n  </additionalData>\n"
            "</service>\r\n",
            origin_header,
            DIAL_VERSION,
            app->name,
            canStop ? "true" : "false",
            app->state ? "running" : "stopped",
            app->state == kDIALStatusStopped ?
                    "" : "  <link rel=\"run\" href=\"run\"/>\r\n",
            dial_data);
    ds_unlock(ds);
}

static void handle_app_stop(struct mg_connection *conn,
                            const struct mg_request_info *request_info,
                            const char *app_name,
                            const char *origin_header) {
    DIALApp *app;
    DIALServer *ds = request_info->user_data;
    int canStop = 0;

    ds_lock(ds);
    app = *find_app(ds, app_name);

    // update the application state
    if (app) {
        app->state = app->callbacks.status_cb(ds, app_name, app->run_id,
                                              &canStop, app->callback_data);
    }

    if (!app || app->state != kDIALStatusRunning) {
        mg_send_http_error(conn, 404, "Not Found", "Not Found");
    } else {
        app->callbacks.stop_cb(ds, app_name, app->run_id, app->callback_data);
        app->state = kDIALStatusStopped;
        mg_printf(conn, "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/plain\r\n"
                  "Access-Control-Allow-Origin: %s\r\n"
                  "\r\n",
                  origin_header);
    }
    ds_unlock(ds);
}

static void handle_dial_data(struct mg_connection *conn,
                             const struct mg_request_info *request_info,
                             const char *app_name,
                             const char *origin_header,
                             int use_payload) {
    char body[DIAL_DATA_MAX_PAYLOAD + 2] = {0, };

    DIALApp *app;
    DIALServer *ds = request_info->user_data;

    ds_lock(ds);
    app = *find_app(ds, app_name);
    if (!app) {
        mg_send_http_error(conn, 404, "Not Found", "Not Found");
        ds_unlock(ds);
        return;
    }
    int nread;
    if (!use_payload) {
        if (request_info->query_string) {
            strncpy(body, request_info->query_string, DIAL_DATA_MAX_PAYLOAD);
            nread = strlen(body);
        } else {
          nread = 0;
        }
    } else {
        nread = mg_read(conn, body, DIAL_DATA_MAX_PAYLOAD);
        body[nread] = '\0';
    }
    if (nread > DIAL_DATA_MAX_PAYLOAD) {
        mg_send_http_error(conn, 413, "413 Request Entity Too Large",
                           "413 Request Entity Too Large");
        ds_unlock(ds);
        return;
    }

    if (isBadPayload(body, nread)) {
        mg_send_http_error(conn, 400, "400 Bad Request", "400 Bad Request");
        ds_unlock(ds);
        return;
    }

    app->dial_data = parse_params(body);
    store_dial_data(app->name, app->dial_data);

    mg_printf(conn, "HTTP/1.1 200 OK\r\n"
              "Access-Control-Allow-Origin: %s\r\n"
              "\r\n",
              origin_header);

    ds_unlock(ds);
}

static int ends_with(const char *str, const char *suffix) {
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}


// str contains a white space separated list of strings (only supports SPACE  characters for now)
static int ends_with_in_list (const char *str, const char *list) {
    if (!str || !list)
        return 0;
    
    const char * scanPointer=list;
    const char * spacePointer;
    unsigned int substringSize = 257;
    char *substring = (char *)malloc(substringSize);
    if (!substring){
        return 0;
    }
    while ( (spacePointer =strchr(scanPointer, ' ')) != NULL) {
    	int copyLength = spacePointer - scanPointer;      
      
      // protect against buffer overflow
      if (copyLength>=substringSize){
          substringSize=copyLength+1;
          free(substring);
          substring=(char *)malloc(substringSize);
          if (!substring){
              return 0;
          }
      }

    	memcpy(substring, scanPointer, copyLength);
    	substring[copyLength] = '\0';
    	//printf("found %s \n", substring);
    	if (ends_with(str, substring)) {
          free(substring);
          return 1;
    	}
    	scanPointer = scanPointer + copyLength + 1; // assumption: only 1 character
    }
    free(substring);
    return ends_with(str, scanPointer);
}

static int should_check_for_origin( char * origin ) {
    const char * const CHECK_PROTOS[] = { "http:", "https:", "file:" };
    for (int i = 0; i < 3; ++i) {
        if (!strncmp(origin, CHECK_PROTOS[i], strlen(CHECK_PROTOS[i]) - 1)) {
            return 1;
        }
    }
    return 0;
}

static int is_allowed_origin(DIALServer* ds, char * origin, const char * app_name) {
    if (!origin || strlen(origin)==0 || !should_check_for_origin(origin)) {
        return 1;
    }

    ds_lock(ds);
    DIALApp *app;
    int result = 0;
    for (app = ds->apps; app != NULL; app = app->next) {
    	if (!strcmp(app->name, app_name)) {
            if (!app->corsAllowedOrigin[0] ||
                ends_with_in_list(origin, app->corsAllowedOrigin)) {
                result = 1;
                break;
            }
        }
    }
    ds_unlock(ds);

    return result;
}

#define APPS_URI "/apps/"
#define RUN_URI "/run"

static void *options_response(DIALServer *ds, struct mg_connection *conn, char *host_header, char *origin_header, const char* app_name, const char* methods)
{    
    if (host_header && is_allowed_origin(ds, origin_header, app_name)) {
        mg_printf(
                  conn,
                  "HTTP/1.1 204 No Content\r\n"
                  "Access-Control-Allow-Methods: %s\r\n"
                  "Access-Control-Max-Age: 86400\r\n"
                  "Access-Control-Allow-Origin: %s\r\n"
                  "Content-Length: 0"
                  "\r\n",
                  methods,
                  origin_header);
        return "done";
    }
    mg_send_http_error(conn, 403, "Forbidden", "Forbidden");
    return "done";   
}

static void *request_handler(enum mg_event event, struct mg_connection *conn,
                             const struct mg_request_info *request_info) {
    DIALServer *ds = request_info->user_data;

    // fprintf(stderr, "Received request %s\n", request_info->uri);
    char *host_header = {0,};
    char *origin_header = {0,};
    for (int i = 0; i < request_info->num_headers; ++i) {
        if (!strcmp(request_info->http_headers[i].name, "Host")) {
            host_header = request_info->http_headers[i].value;
        } else if (!strcmp(request_info->http_headers[i].name,
                          "Origin")) {
            origin_header = request_info->http_headers[i].value;
        }
    }
    // fprintf(stderr, "Origin %s, Host: %s\n", origin_header, host_header);
    if (event == MG_NEW_REQUEST) {
        // URL ends with run
        if (!strncmp(request_info->uri + strlen(request_info->uri) - 4, RUN_URI,
                     strlen(RUN_URI))) {
            char app_name[256] = {0, };  // assuming the application name is not over 256 chars.
            strncpy(app_name, request_info->uri + strlen(APPS_URI),
                    ((strlen(request_info->uri) - 4) - (sizeof(APPS_URI) - 1)));

            if (!strcmp(request_info->request_method, "OPTIONS")) {
                return options_response(ds, conn, host_header, origin_header, app_name, "DELETE, OPTIONS");
            }

            // DELETE non-empty app name
            if (app_name[0] != '\0'
                    && !strcmp(request_info->request_method, "DELETE")) {
                if (host_header && is_allowed_origin(ds, origin_header, app_name)) {
                    handle_app_stop(conn, request_info, app_name, origin_header);
                } else {
                    mg_send_http_error(conn, 403, "Forbidden", "Forbidden");
                    return "done";
                }
            } else {
                mg_send_http_error(conn, 501, "Not Implemented",
                                   "Not Implemented");
            }
        }
        // URI starts with "/apps/" and is followed by an app name
        else if (!strncmp(request_info->uri, APPS_URI, sizeof(APPS_URI) - 1)
                && !strchr(request_info->uri + strlen(APPS_URI), '/')) {
            const char *app_name;
            app_name = request_info->uri + sizeof(APPS_URI) - 1;

            if (!strcmp(request_info->request_method, "OPTIONS")) {
                return options_response(ds, conn, host_header, origin_header, app_name, "GET, POST, OPTIONS");
            }

            // start app
            if (!strcmp(request_info->request_method, "POST")) {
                if (host_header && is_allowed_origin(ds, origin_header, app_name)) {
                    handle_app_start(conn, request_info, app_name, origin_header);
                } else {
                    mg_send_http_error(conn, 403, "Forbidden", "Forbidden");
                    return "done";
                }
            // get app status
            } else if (!strcmp(request_info->request_method, "GET")) {
                handle_app_status(conn, request_info, app_name, origin_header);
            } else {
                mg_send_http_error(conn, 501, "Not Implemented",
                                   "Not Implemented");
            }
        // URI is of the form */app_name/dial_data
        } else if (strstr(request_info->uri, DIAL_DATA_URI)) {
            char laddr[INET6_ADDRSTRLEN];
            const struct sockaddr_in *addr =
                    (struct sockaddr_in *) &request_info->remote_addr;
            inet_ntop(addr->sin_family, &addr->sin_addr, laddr, sizeof(laddr));
            if ( !strncmp(laddr, gLocalhost, strlen(gLocalhost)) ) {
                char *app_name = parse_app_name(request_info->uri);

                if (!strcmp(request_info->request_method, "OPTIONS")) {
                    void *ret = options_response(ds, conn, host_header, origin_header, app_name, "POST, OPTIONS");
                    free(app_name);
                    return ret;
                }

                int use_payload =
                    strcmp(request_info->request_method, "POST") ? 0 : 1;
                handle_dial_data(conn, request_info, app_name, origin_header,
                                 use_payload);

                free(app_name);
            } else {
                // If the request is not from local host, return an error
                mg_send_http_error(conn, 403, "Forbidden", "Forbidden");
            }
        } else {
            mg_send_http_error(conn, 404, "Not Found", "Not Found");
        }
        return "done";
    } else if (event == MG_EVENT_LOG) {
        fprintf( stderr, "MG: %s\n", request_info->log_message);
        return "done";
    }
    return NULL;
}

extern DIALServer *DIAL_create() {
    DIALServer *ds = calloc(1, sizeof(DIALServer));
    pthread_mutex_init(&ds->mux, NULL);
    return ds;
}

void DIAL_start(DIALServer *ds) {
    ds->ctx = mg_start(&request_handler, ds, DIAL_PORT);
}

void DIAL_stop(DIALServer *ds) {
    mg_stop(ds->ctx);
    pthread_mutex_destroy(&ds->mux);
}

in_port_t DIAL_get_port(DIALServer *ds) {
    struct sockaddr sa;
    socklen_t len = sizeof(sa);
    if (!mg_get_listen_addr(ds->ctx, &sa, &len)) {
        return 0;
    }
    return ntohs(((struct sockaddr_in *) &sa)->sin_port);
}

int DIAL_register_app(DIALServer *ds, const char *app_name,
                      struct DIALAppCallbacks *callbacks, void *user_data,
                      int useAdditionalData,
                      const char* corsAllowedOrigin) {
    DIALApp **ptr, *app;
    int ret;

    ds_lock(ds);
    ptr = find_app(ds, app_name);
    if (*ptr != NULL) {  // app already registered
        ds_unlock(ds);
        ret = 0;
    } else {
        app = malloc(sizeof(DIALApp));
        app->callbacks = *callbacks;
        app->name = strdup(app_name);
        app->next = *ptr;
        app->state = kDIALStatusStopped;
        app->callback_data = user_data;
        app->dial_data = retrieve_dial_data(app->name);
        app->useAdditionalData = useAdditionalData;
        app->corsAllowedOrigin[0] = '\0';
        if (corsAllowedOrigin &&
            strlen(corsAllowedOrigin) < sizeof(app->corsAllowedOrigin)) {
          strcpy(app->corsAllowedOrigin, corsAllowedOrigin);
        }
        *ptr = app;
        ret = 1;
    }
    ds_unlock(ds);
    return ret;
}

int DIAL_unregister_app(DIALServer *ds, const char *app_name) {
    DIALApp **ptr, *app;
    int ret;

    ds_lock(ds);
    ptr = find_app(ds, app_name);
    if (*ptr == NULL) {  // no such app
        ret = 0;
    } else {
        app = *ptr;
        *ptr = app->next;
        free(app->name);
        free(app);
        ret = 1;
    }

    ds_unlock(ds);
    return ret;
}

const char * DIAL_get_payload(DIALServer *ds, const char *app_name) {
    const char * pPayload = NULL;
    DIALApp **ptr, *app;

    // NOTE: Don't grab the mutex as we are calling this function from
    // inside the application callback which already has the lock.
    //ds_lock(ds);
    ptr = find_app(ds, app_name);
    if (*ptr != NULL) {
        app = *ptr;
        pPayload = app->payload;
    }
    //ds_unlock(ds);
    return pPayload;
}

