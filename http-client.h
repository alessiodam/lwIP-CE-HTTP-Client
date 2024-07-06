/*
 *--------------------------------------
 * Program Name: lwIP-CE HTTP Client
 * Author: TKB Studios
 * License: MIT License
 * Description: An HTTP Client for lwIP-CE
 *--------------------------------------
*/

#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <usbdrvce.h>
#include <stdbool.h>
#include <time.h>

#include "lwip/timeouts.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*http_client_callback_t)(struct pbuf *p, err_t err);

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD
} http_method_t;

/// @brief Sends an HTTP request
/// @param method HTTP method, GET, POST, PUT, DELETE, HEAD
/// @param host The host to connect to, can be an IP address in ASCII representation OR a domain, a DNS lookup will be performed if needed.
/// @param server_port The port of the server, example: 80
/// @param path URL to request, example: "/index.html"
/// @param headers Request headers, example: "Content-Type: text/html\r\nAccept: */*\r\n"
/// @param body Request body, example: "Hello, World!"
/// @param buf_len Size of the request buffer, not the response one.
/// @param callback Callback function for received HTTP data.
/// @return An lwIP error code.
err_t http_request(
    http_method_t method,
    const char *host,
    uint16_t server_port,
    const char *path,
    const char *headers,
    const char *body,
    unsigned int buf_len,
    http_client_callback_t callback
);

#ifdef __cplusplus
}
#endif

#endif
