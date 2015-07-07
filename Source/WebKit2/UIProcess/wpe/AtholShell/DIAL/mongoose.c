// Copyright (c) 2004-2010 Sergey Lyubka
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _BSD_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#ifndef BUFSIZ
#define BUFSIZ  4096
#endif

#define MAX_REQUEST_SIZE 4096
#define NUM_THREADS 4
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdint.h>
#include <inttypes.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>


#define ERRNO errno
#define INVALID_SOCKET (-1)

typedef int SOCKET;

#include "mongoose.h"

#define MONGOOSE_VERSION "3.0"
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#if defined(DEBUG)
#define DEBUG_TRACE(x) do { \
  flockfile(stdout); \
  printf("*** %lu.%p.%s.%d: ", \
         (unsigned long) time(NULL), (void *) pthread_self(), \
         __func__, __LINE__); \
  printf x; \
  putchar('\n'); \
  fflush(stdout); \
  funlockfile(stdout); \
} while (0)
#else
#define DEBUG_TRACE(x)
#endif // DEBUG

typedef void * (*mg_thread_func_t)(void *);


// Describes a socket which was accept()-ed by the master thread and queued for
// future handling by the worker thread.
struct socket {
  SOCKET sock;          // Listening socket
  struct sockaddr_in local_addr;  // Local socket address
  struct sockaddr_in remote_addr;  // Remote socket address
};

struct mg_context {
  volatile int stop_flag;       // Should we stop event loop
  mg_callback_t user_callback;  // User-defined callback function
  void *user_data;              // User-defined data

  SOCKET             local_socket;
  struct sockaddr_in local_address;

  volatile int num_threads;  // Number of threads
  pthread_mutex_t mutex;     // Protects (max|num)_threads
  pthread_cond_t  cond;      // Condvar for tracking workers terminations

  struct socket queue[20];   // Accepted sockets
  volatile int sq_head;      // Head of the socket queue
  volatile int sq_tail;      // Tail of the socket queue
  pthread_cond_t sq_full;    // Singaled when socket is produced
  pthread_cond_t sq_empty;   // Signaled when socket is consumed
};

struct mg_connection {
  struct mg_request_info request_info;
  struct mg_context *ctx;
  struct socket client;       // Connected client
  time_t birth_time;          // Time connection was accepted
  int64_t num_bytes_sent;     // Total bytes sent to client
  int64_t content_len;        // Content-Length header value
  int64_t consumed_content;   // How many bytes of content is already read
  char *buf;                  // Buffer for received data
  int buf_size;               // Buffer size
  int request_len;            // Size of the request + headers in a buffer
  int data_len;               // Total size of data in a buffer
};

static void *call_user(struct mg_connection *conn, enum mg_event event) {
  conn->request_info.user_data = conn->ctx->user_data;
  return conn->ctx->user_callback == NULL ? NULL :
    conn->ctx->user_callback(event, conn, &conn->request_info);
}

// Print error message to the opened error log stream.
static void cry(struct mg_connection *conn, const char *fmt, ...) {
  char buf[BUFSIZ];
  va_list ap;

  va_start(ap, fmt);
  (void) vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  // Do not lock when getting the callback value, here and below.
  // I suppose this is fine, since function cannot disappear in the
  // same way string option can.
  conn->request_info.log_message = buf;
  if (call_user(conn, MG_EVENT_LOG) == NULL) {
      DEBUG_TRACE(("[%s]", buf));
  }
  conn->request_info.log_message = NULL;
}

// Return fake connection structure. Used for logging, if connection
// is not applicable at the moment of logging.
static struct mg_connection *fc(struct mg_context *ctx) {
  static struct mg_connection fake_connection;
  fake_connection.ctx = ctx;
  return &fake_connection;
}

const char *mg_version(void) {
  return MONGOOSE_VERSION;
}

static int lowercase(const char *s) {
  return tolower(* (const unsigned char *) s);
}

static int mg_strcasecmp(const char *s1, const char *s2) {
  int diff;

  do {
    diff = lowercase(s1++) - lowercase(s2++);
  } while (diff == 0 && s1[-1] != '\0');

  return diff;
}

// Like snprintf(), but never returns negative value, or the value
// that is larger than a supplied buffer.
// Thanks to Adam Zeldis to pointing snprintf()-caused vulnerability
// in his audit report.
static int mg_vsnprintf(struct mg_connection *conn, char *buf, size_t buflen,
                        const char *fmt, va_list ap) {
  int n;

  if (buflen == 0)
    return 0;

  n = vsnprintf(buf, buflen, fmt, ap);

  if (n < 0) {
    cry(conn, "vsnprintf error");
    n = 0;
  } else if (n >= (int) buflen) {
    cry(conn, "truncating vsnprintf buffer: [%.*s]",
        n > 200 ? 200 : n, buf);
    n = (int) buflen - 1;
  }
  buf[n] = '\0';

  return n;
}

static int mg_snprintf(struct mg_connection *conn, char *buf, size_t buflen,
                       const char *fmt, ...) {
  va_list ap;
  int n;

  va_start(ap, fmt);
  n = mg_vsnprintf(conn, buf, buflen, fmt, ap);
  va_end(ap);

  return n;
}

// Skip the characters until one of the delimiters characters found.
// 0-terminate resulting word. Skip the delimiter and following whitespaces if any.
// Advance pointer to buffer to the next word. Return found 0-terminated word.
// Delimiters can be quoted with quotechar.
static char *skip_quoted(char **buf, const char *delimiters, const char *whitespace, char quotechar) {
  char *p, *begin_word, *end_word, *end_whitespace;

  begin_word = *buf;
  end_word = begin_word + strcspn(begin_word, delimiters);

  /* Check for quotechar */
  if (end_word > begin_word) {
    p = end_word - 1;
    while (*p == quotechar) {
      /* If there is anything beyond end_word, copy it */
      if (*end_word == '\0') {
        *p = '\0';
        break;
      } else {
        size_t end_off = strcspn(end_word + 1, delimiters);
        memmove (p, end_word, end_off + 1);
        p += end_off; /* p must correspond to end_word - 1 */
        end_word += end_off + 1;
      }
    }
    for (p++; p < end_word; p++) {
      *p = '\0';
    }
  }

  if (*end_word == '\0') {
    *buf = end_word;
  } else {
    end_whitespace = end_word + 1 + strspn(end_word + 1, whitespace);

    for (p = end_word; p < end_whitespace; p++) {
      *p = '\0';
    }

    *buf = end_whitespace;
  }

  return begin_word;
}

// Simplified version of skip_quoted without quote char
// and whitespace == delimiters
static char *skip(char **buf, const char *delimiters) {
  return skip_quoted(buf, delimiters, delimiters, 0);
}


// Return HTTP header value, or NULL if not found.
static const char *get_header(const struct mg_request_info *ri,
                              const char *name) {
  int i;

  for (i = 0; i < ri->num_headers; i++)
    if (!mg_strcasecmp(name, ri->http_headers[i].name))
      return ri->http_headers[i].value;

  return NULL;
}

const char *mg_get_header(const struct mg_connection *conn, const char *name) {
  return get_header(&conn->request_info, name);
}

static const char *suggest_connection_header(const struct mg_connection *conn) {
  return "close";
}

void mg_send_http_error(struct mg_connection *conn, int status,
                        const char *reason, const char *fmt, ...) {
  char buf[BUFSIZ];
  va_list ap;
  int len;

  conn->request_info.status_code = status;

  buf[0] = '\0';
  len = 0;

  /* Errors 1xx, 204 and 304 MUST NOT send a body */
  if (status > 199 && status != 204 && status != 304) {
    len = mg_snprintf(conn, buf, sizeof(buf), "Error %d: %s", status, reason);
    cry(conn, "%s", buf);
    buf[len++] = '\n';

    va_start(ap, fmt);
    len += mg_vsnprintf(conn, buf + len, sizeof(buf) - len, fmt, ap);
    va_end(ap);
  }
  DEBUG_TRACE(("[%s]", buf));

  mg_printf(conn, "HTTP/1.1 %d %s\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: %d\r\n"
      "Connection: %s\r\n\r\n", status, reason, len,
      suggest_connection_header(conn));
  conn->num_bytes_sent += mg_printf(conn, "%s", buf);
}

static int start_thread(struct mg_context *ctx, mg_thread_func_t func,
                        void *param) {
  pthread_t thread_id;
  pthread_attr_t attr;
  int retval;

  (void) pthread_attr_init(&attr);
  (void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  // TODO(lsm): figure out why mongoose dies on Linux if next line is enabled
  // (void) pthread_attr_setstacksize(&attr, sizeof(struct mg_connection) * 5);

  if ((retval = pthread_create(&thread_id, &attr, func, param)) != 0) {
    cry(fc(ctx), "%s: %s", __func__, strerror(retval));
  }

  return retval;
}

static int set_non_blocking_mode(SOCKET sock) {
  int flags;

  flags = fcntl(sock, F_GETFL, 0);
  (void) fcntl(sock, F_SETFL, flags | O_NONBLOCK);

  return 0;
}

// Write data to the IO channel - opened file descriptor, socket or SSL
// descriptor. Return number of bytes written.
static int64_t push(FILE *fp, SOCKET sock, const char *buf, int64_t len) {
  int64_t sent;
  int n, k;

  sent = 0;
  while (sent < len) {

    /* How many bytes we send in this iteration */
    k = len - sent > INT_MAX ? INT_MAX : (int) (len - sent);

    if (fp != NULL) {
      n = fwrite(buf + sent, 1, (size_t)k, fp);
      if (ferror(fp))
        n = -1;
    } else {
      n = send(sock, buf + sent, (size_t)k, 0);
    }

    if (n < 0)
      break;

    sent += n;
  }

  return sent;
}

// Read from IO channel - opened file descriptor, socket, or SSL descriptor.
// Return number of bytes read.
static int pull(SOCKET sock, char *buf, int len) {
  int nread;

  nread = recv(sock, buf, (size_t) len, 0);

  return nread;
}

int mg_read(struct mg_connection *conn, void *buf, size_t len) {
  int n, buffered_len, nread;
  const char *buffered;

  assert((conn->content_len == -1 && conn->consumed_content == 0) ||
         conn->consumed_content <= conn->content_len);
  DEBUG_TRACE(("%p %zu %" PRId64 " %" PRId64, buf, len,
               conn->content_len, conn->consumed_content));
  nread = 0;
  if (conn->consumed_content < conn->content_len) {

    // Adjust number of bytes to read.
    int64_t to_read = conn->content_len - conn->consumed_content;
    if (to_read < (int64_t) len) {
      len = (int) to_read;
    }

    // How many bytes of data we have buffered in the request buffer?
    buffered = conn->buf + conn->request_len + conn->consumed_content;
    buffered_len = conn->data_len - conn->request_len;
    assert(buffered_len >= 0);

    // Return buffered data back if we haven't done that yet.
    if (conn->consumed_content < (int64_t) buffered_len) {
      buffered_len -= (int) conn->consumed_content;
      if (len < (size_t) buffered_len) {
        buffered_len = len;
      }
      memcpy(buf, buffered, (size_t)buffered_len);
      len -= buffered_len;
      buf = (char *) buf + buffered_len;
      conn->consumed_content += buffered_len;
      nread = buffered_len;
    }

    // We have returned all buffered data. Read new data from the remote socket.
    while (len > 0) {
      n = pull(conn->client.sock, (char *) buf, (int) len);
      if (n <= 0) {
        break;
      }
      buf = (char *) buf + n;
      conn->consumed_content += n;
      nread += n;
      len -= n;
    }
  }
  return nread;
}

int mg_write(struct mg_connection *conn, const void *buf, size_t len) {
  return (int) push(NULL, conn->client.sock, (const char *) buf, (int64_t) len);
}

int mg_printf(struct mg_connection *conn, const char *fmt, ...) {
  char buf[BUFSIZ];
  int len;
  va_list ap;

  va_start(ap, fmt);
  len = mg_vsnprintf(conn, buf, sizeof(buf), fmt, ap);
  va_end(ap);

  return mg_write(conn, buf, (size_t)len);
}

// URL-decode input buffer into destination buffer.
// 0-terminate the destination buffer. Return the length of decoded data.
// form-url-encoded data differs from URI encoding in a way that it
// uses '+' as character for space, see RFC 1866 section 8.2.1
// http://ftp.ics.uci.edu/pub/ietf/html/rfc1866.txt
static size_t url_decode(const char *src, size_t src_len, char *dst,
                         size_t dst_len, int is_form_url_encoded) {
  size_t i, j;
  int a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

  for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
    if (src[i] == '%' &&
        isxdigit(* (const unsigned char *) (src + i + 1)) &&
        isxdigit(* (const unsigned char *) (src + i + 2))) {
      a = tolower(* (const unsigned char *) (src + i + 1));
      b = tolower(* (const unsigned char *) (src + i + 2));
      dst[j] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
      i += 2;
    } else if (is_form_url_encoded && src[i] == '+') {
      dst[j] = ' ';
    } else {
      dst[j] = src[i];
    }
  }

  dst[j] = '\0'; /* Null-terminate the destination */

  return j;
}

// Check whether full request is buffered. Return:
//   -1  if request is malformed
//    0  if request is not yet fully buffered
//   >0  actual request length, including last \r\n\r\n
static int get_request_len(const char *buf, int buflen) {
  const char *s, *e;
  int len = 0;

  DEBUG_TRACE(("buf: %p, len: %d", buf, buflen));
  for (s = buf, e = s + buflen - 1; len <= 0 && s < e; s++)
    // Control characters are not allowed but >=128 is.
    if (!isprint(* (const unsigned char *) s) && *s != '\r' &&
        *s != '\n' && * (const unsigned char *) s < 128) {
      len = -1;
    } else if (s[0] == '\n' && s[1] == '\n') {
      len = (int) (s - buf) + 2;
    } else if (s[0] == '\n' && &s[1] < e &&
        s[1] == '\r' && s[2] == '\n') {
      len = (int) (s - buf) + 3;
    }

  return len;
}

// Protect against directory disclosure attack by removing '..',
// excessive '/' and '\' characters
static void remove_double_dots_and_double_slashes(char *s) {
  char *p = s;

  while (*s != '\0') {
    *p++ = *s++;
    if (s[-1] == '/' || s[-1] == '\\') {
      // Skip all following slashes and backslashes
      while (*s == '/' || *s == '\\') {
        s++;
      }

      // Skip all double-dots
      while (*s == '.' && s[1] == '.') {
        s += 2;
      }
    }
  }
  *p = '\0';
}

// Parse HTTP headers from the given buffer, advance buffer to the point
// where parsing stopped.
static void parse_http_headers(char **buf, struct mg_request_info *ri) {
  int i;

  for (i = 0; i < (int) ARRAY_SIZE(ri->http_headers); i++) {
    ri->http_headers[i].name = skip_quoted(buf, ":", " ", 0);
    ri->http_headers[i].value = skip(buf, "\r\n");
    if (ri->http_headers[i].name[0] == '\0')
      break;
    ri->num_headers = i + 1;
  }
}

static int is_valid_http_method(const char *method) {
  // fprintf(stderr, "Received HTTP method %s\n", method);
  return !strcmp(method, "GET") || !strcmp(method, "POST") ||
    !strcmp(method, "DELETE") || !strcmp(method, "OPTIONS");
}

// Parse HTTP request, fill in mg_request_info structure.
static int parse_http_request(char *buf, struct mg_request_info *ri) {
  int status = 0;

  // RFC says that all initial whitespaces should be ingored
  while (*buf != '\0' && isspace(* (unsigned char *) buf)) {
    buf++;
  }

  ri->request_method = skip(&buf, " ");
  ri->uri = skip(&buf, " ");
  ri->http_version = skip(&buf, "\r\n");

  if (is_valid_http_method(ri->request_method) &&
      strncmp(ri->http_version, "HTTP/", 5) == 0) {
    ri->http_version += 5;   /* Skip "HTTP/" */
    parse_http_headers(&buf, ri);
    status = 1;
  }

  return status;
}

// Keep reading the input from socket sock
// into buffer buf, until \r\n\r\n appears in the buffer (which marks the end
// of HTTP request). Buffer buf may already have some data. The length of the
// data is stored in nread.  Upon every read operation, increase nread by the
// number of bytes read.
static int read_request(SOCKET sock, char *buf, int bufsiz,
                        int *nread) {
  int n, request_len;

  request_len = 0;
  while (*nread < bufsiz && request_len == 0) {
    n = pull(sock, buf + *nread, bufsiz - *nread);
    if (n <= 0) {
      break;
    } else {
      *nread += n;
      request_len = get_request_len(buf, *nread);
    }
  }

  return request_len;
}

// This is the heart of the Mongoose's logic.
// This function is called when the request is read, parsed and validated,
// and Mongoose must decide what action to take: serve a file, or
// a directory, or call embedded function, etcetera.
static void handle_request(struct mg_connection *conn) {
  struct mg_request_info *ri = &conn->request_info;
  int uri_len;

  if ((conn->request_info.query_string = strchr(ri->uri, '?')) != NULL) {
    * conn->request_info.query_string++ = '\0';
  }
  uri_len = strlen(ri->uri);
  (void) url_decode(ri->uri, (size_t)uri_len, ri->uri, (size_t)(uri_len + 1), 0);
  remove_double_dots_and_double_slashes(ri->uri);

  DEBUG_TRACE(("%s", ri->uri));
  if (call_user(conn, MG_NEW_REQUEST) == NULL) {
    mg_send_http_error(conn, 404, "Not Found", "%s", "File not found");
  }
}

static void close_all_listening_sockets(struct mg_context *ctx) {
  (void) close(ctx->local_socket);
}

// only reports address of the first listening socket
int mg_get_listen_addr(struct mg_context *ctx,
    struct sockaddr *addr, socklen_t *addrlen) {
  size_t len = sizeof(ctx->local_address);
  if (*addrlen < len) return 0;
  *addrlen = len;
  memcpy(addr, &ctx->local_address, len);
  return 1;
}

static int set_ports_option(struct mg_context *ctx, int port) {
  int reuseaddr = 1, success = 1;
  socklen_t sock_len = sizeof(ctx->local_address);
  // MacOS needs that. If we do not zero it, subsequent bind() will fail.
  memset(&ctx->local_address, 0, sock_len);
  ctx->local_address.sin_family = AF_INET;
  ctx->local_address.sin_port = htons((uint16_t) port);
  ctx->local_address.sin_addr.s_addr = htonl(INADDR_ANY);

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500 * 1000;

  if ((ctx->local_socket = socket(PF_INET, SOCK_STREAM, 6)) == INVALID_SOCKET ||
      setsockopt(ctx->local_socket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) != 0 ||
      setsockopt(ctx->local_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0 ||
      bind(ctx->local_socket, (const struct sockaddr *) &ctx->local_address, sock_len) != 0 ||
      // TODO(steineldar): Replace 20 (max socket backlog len in connections).
      listen(ctx->local_socket, 20) != 0) {
    close(ctx->local_socket);
    cry(fc(ctx), "%s: cannot bind to port %d: %s", __func__,
        port, strerror(ERRNO));
    success = 0;
  } else if (getsockname(ctx->local_socket, (struct sockaddr *) &ctx->local_address, &sock_len)) {
    close(ctx->local_socket);
    cry(fc(ctx), "%s: %s", __func__, strerror(ERRNO));
    success = 0;
  }

  if (!success) {
    ctx->local_socket = INVALID_SOCKET;
    close_all_listening_sockets(ctx);
  }

  return success;
}


static void reset_per_request_attributes(struct mg_connection *conn) {
  struct mg_request_info *ri = &conn->request_info;

  ri->request_method = ri->uri = ri->http_version = NULL;
  ri->num_headers = 0;
  ri->status_code = -1;

  conn->num_bytes_sent = conn->consumed_content = 0;
  conn->content_len = -1;
  conn->request_len = conn->data_len = 0;
}

static void close_socket_gracefully(SOCKET sock) {
  char buf[BUFSIZ];
  int n;

  // Send FIN to the client
  (void) shutdown(sock, SHUT_WR);
  set_non_blocking_mode(sock);

  // Read and discard pending data. If we do not do that and close the
  // socket, the data in the send buffer may be discarded. This
  // behaviour is seen on Windows, when client keeps sending data
  // when server decide to close the connection; then when client
  // does recv() it gets no data back.
  do {
    n = pull(sock, buf, sizeof(buf));
  } while (n > 0);

  // Now we know that our FIN is ACK-ed, safe to close
  (void) close(sock);
}

static void close_connection(struct mg_connection *conn) {
  if (conn->client.sock != INVALID_SOCKET) {
    close_socket_gracefully(conn->client.sock);
  }
}

static void discard_current_request_from_buffer(struct mg_connection *conn) {
  int buffered_len, body_len;

  buffered_len = conn->data_len - conn->request_len;
  assert(buffered_len >= 0);

  if (conn->content_len == -1) {
    body_len = 0;
  } else if (conn->content_len < (int64_t) buffered_len) {
    body_len = (int) conn->content_len;
  } else {
    body_len = buffered_len;
  }

  conn->data_len -= conn->request_len + body_len;
  memmove(conn->buf, conn->buf + conn->request_len + body_len,
          (size_t) conn->data_len);
}

static void process_new_connection(struct mg_connection *conn) {
  struct mg_request_info *ri = &conn->request_info;
  const char *cl;

  reset_per_request_attributes(conn);

  // If next request is not pipelined, read it in
  if ((conn->request_len = get_request_len(conn->buf, conn->data_len)) == 0) {
    conn->request_len = read_request(conn->client.sock,
        conn->buf, conn->buf_size, &conn->data_len);
  }
  assert(conn->data_len >= conn->request_len);
  if (conn->request_len == 0 && conn->data_len == conn->buf_size) {
    mg_send_http_error(conn, 413, "Request Too Large", "");
    return;
  } if (conn->request_len <= 0) {
    return;  // Remote end closed the connection
  }

  // Nul-terminate the request cause parse_http_request() uses sscanf
  conn->buf[conn->request_len - 1] = '\0';
  if (!parse_http_request(conn->buf, ri)) {
    // Do not put garbage in the access log, just send it back to the client
    mg_send_http_error(conn, 400, "Bad Request",
        "Cannot parse HTTP request: [%.*s]", conn->data_len, conn->buf);
  } else if (strcmp(ri->http_version, "1.0") && strcmp(ri->http_version, "1.1")) {
    // Request seems valid, but HTTP version is strange
    mg_send_http_error(conn, 505, "HTTP version not supported", "");
  } else {
    // Request is valid, handle it
    cl = get_header(ri, "Content-Length");
    conn->content_len = cl == NULL ? -1 : strtoll(cl, NULL, 10);
    conn->birth_time = time(NULL);
    handle_request(conn);
    discard_current_request_from_buffer(conn);
  }
}

// Worker threads take accepted socket from the queue
static int consume_socket(struct mg_context *ctx, struct socket *sp) {
  (void) pthread_mutex_lock(&ctx->mutex);
  DEBUG_TRACE(("going idle"));

  // If the queue is empty, wait. We're idle at this point.
  while (ctx->sq_head == ctx->sq_tail && ctx->stop_flag == 0) {
    pthread_cond_wait(&ctx->sq_full, &ctx->mutex);
  }
  // Master thread could wake us up without putting a socket.
  // If this happens, it is time to exit.
  if (ctx->stop_flag) {
    (void) pthread_mutex_unlock(&ctx->mutex);
    return 0;
  }
  assert(ctx->sq_head > ctx->sq_tail);

  // Copy socket from the queue and increment tail
  *sp = ctx->queue[ctx->sq_tail % ARRAY_SIZE(ctx->queue)];
  ctx->sq_tail++;
  DEBUG_TRACE(("grabbed socket %d, going busy", sp->sock));

  // Wrap pointers if needed
  while (ctx->sq_tail > (int) ARRAY_SIZE(ctx->queue)) {
    ctx->sq_tail -= ARRAY_SIZE(ctx->queue);
    ctx->sq_head -= ARRAY_SIZE(ctx->queue);
  }

  (void) pthread_cond_signal(&ctx->sq_empty);
  (void) pthread_mutex_unlock(&ctx->mutex);

  return 1;
}


static void worker_thread(struct mg_context *ctx) {
  struct mg_connection *conn;
  // This is the specified request size limit for DIAL requests.  Note that
  // this will effectively make the request limit one byte *smaller* than the
  // required in the DIAL specification.
  int buf_size = MAX_REQUEST_SIZE;

  conn = (struct mg_connection *) calloc(1, sizeof(*conn) + buf_size);
  conn->buf_size = buf_size;
  conn->buf = (char *) (conn + 1);
  assert(conn != NULL);

  while (ctx->stop_flag == 0 && consume_socket(ctx, &conn->client)) {
    conn->birth_time = time(NULL);
    conn->ctx = ctx;

    // Fill in IP, port info early so even if SSL setup below fails,
    // error handler would have the corresponding info.
    // Thanks to Johannes Winkelmann for the patch.
    memcpy(&conn->request_info.remote_addr,
           &conn->client.remote_addr, sizeof(conn->client.remote_addr));

    // Fill in local IP info
    socklen_t addr_len = sizeof(conn->request_info.local_addr);
    getsockname(conn->client.sock,
        (struct sockaddr *) &conn->request_info.local_addr, &addr_len);

    process_new_connection(conn);

    close_connection(conn);
  }
  free(conn);

  // Signal master that we're done with connection and exiting
  (void) pthread_mutex_lock(&ctx->mutex);
  ctx->num_threads--;
  (void) pthread_cond_signal(&ctx->cond);
  assert(ctx->num_threads >= 0);
  (void) pthread_mutex_unlock(&ctx->mutex);

  DEBUG_TRACE(("exiting"));
}

// Master thread adds accepted socket to a queue
static void produce_socket(struct mg_context *ctx, const struct socket *sp) {
  (void) pthread_mutex_lock(&ctx->mutex);

  // If the queue is full, wait
  while (ctx->sq_head - ctx->sq_tail >= (int) ARRAY_SIZE(ctx->queue)) {
    (void) pthread_cond_wait(&ctx->sq_empty, &ctx->mutex);
  }
  assert(ctx->sq_head - ctx->sq_tail < (int) ARRAY_SIZE(ctx->queue));

  // Copy socket to the queue and increment head
  ctx->queue[ctx->sq_head % ARRAY_SIZE(ctx->queue)] = *sp;
  ctx->sq_head++;
  DEBUG_TRACE(("queued socket %d", sp->sock));

  (void) pthread_cond_signal(&ctx->sq_full);
  (void) pthread_mutex_unlock(&ctx->mutex);
}


static void master_thread(struct mg_context *ctx) {
  struct socket accepted;

  socklen_t sock_len = sizeof(accepted.local_addr);
  memcpy(&accepted.local_addr, &ctx->local_address, sock_len);

  while (ctx->stop_flag == 0) {
    memset(&accepted.remote_addr, 0, sock_len);

    accepted.sock = accept(ctx->local_socket,
        (struct sockaddr *) &accepted.remote_addr, &sock_len);

    if (accepted.sock != INVALID_SOCKET) {
      // Put accepted socket structure into the queue.
      DEBUG_TRACE(("accepted socket %d", accepted.sock));
      produce_socket(ctx, &accepted);
    }
  }
  DEBUG_TRACE(("stopping workers"));

  // Stop signal received: somebody called mg_stop. Quit.
  close_all_listening_sockets(ctx);

  // Wakeup workers that are waiting for connections to handle.
  pthread_cond_broadcast(&ctx->sq_full);

  // Wait until all threads finish
  (void) pthread_mutex_lock(&ctx->mutex);
  while (ctx->num_threads > 0) {
    (void) pthread_cond_wait(&ctx->cond, &ctx->mutex);
  }
  (void) pthread_mutex_unlock(&ctx->mutex);

  // All threads exited, no sync is needed. Destroy mutex and condvars
  (void) pthread_mutex_destroy(&ctx->mutex);
  (void) pthread_cond_destroy(&ctx->cond);
  (void) pthread_cond_destroy(&ctx->sq_empty);
  (void) pthread_cond_destroy(&ctx->sq_full);

  // Signal mg_stop() that we're done
  ctx->stop_flag = 2;

  DEBUG_TRACE(("exiting"));
}

static void free_context(struct mg_context *ctx) {
  // Deallocate context itself
  free(ctx);
}

void mg_stop(struct mg_context *ctx) {
  ctx->stop_flag = 1;

  // Wait until mg_fini() stops
  while (ctx->stop_flag != 2) {
    // TODO(steineldar): Avoid busy waiting.
    (void) sleep(0);
  }
  free_context(ctx);
}

struct mg_context *mg_start(mg_callback_t user_callback, void *user_data, int port) {
  struct mg_context *ctx;

  // Allocate context and initialize reasonable general case defaults.
  // TODO(lsm): do proper error handling here.
  ctx = (struct mg_context *) calloc(1, sizeof(*ctx));
  ctx->user_callback = user_callback;
  ctx->user_data = user_data;

  if (!set_ports_option(ctx, port)) {
    free_context(ctx);
    return NULL;
  }
  // Ignore SIGPIPE signal, so if browser cancels the request, it
  // won't kill the whole process.
  (void) signal(SIGPIPE, SIG_IGN);
  (void) pthread_mutex_init(&ctx->mutex, NULL);
  (void) pthread_cond_init(&ctx->cond, NULL);
  (void) pthread_cond_init(&ctx->sq_empty, NULL);
  (void) pthread_cond_init(&ctx->sq_full, NULL);

  // Start master (listening) thread
  start_thread(ctx, (mg_thread_func_t) master_thread, ctx);

  // Start worker threads
  for (int i = 0; i < NUM_THREADS; i++) {
    if (start_thread(ctx, (mg_thread_func_t) worker_thread, ctx) != 0) {
      cry(fc(ctx), "Cannot start worker thread: %d", ERRNO);
    } else {
      ctx->num_threads++;
    }
  }

  return ctx;
}
