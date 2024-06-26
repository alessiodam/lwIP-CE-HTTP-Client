#include "http-client.h"

static http_client_callback_t http_callback = NULL;

static err_t http_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (http_callback) {
        http_callback(arg, pcb, p, err);
    }
    if (p == NULL) {
        altcp_close(pcb);
    } else {
        altcp_recved(pcb, p->len);
        pbuf_free(p);
    }
    return ERR_OK;
}

static err_t http_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    char *request = (char *)arg;
    altcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    altcp_output(pcb);
    altcp_recv(pcb, http_recv);
    return ERR_OK;
}

err_t http_request(
    http_method_t method,
    struct netif *netif,
    const char *server_ip,
    uint16_t server_port,
    const char *path,
    const char *headers,
    const char *body,
    unsigned int buf_len,
    http_client_callback_t http_client_callback
) {
    struct altcp_pcb *pcb = altcp_tcp_new_ip_type(IPADDR_TYPE_V4);
    ip4_addr_t ip;
    if (!ip4addr_aton(server_ip, &ip)) {
        return ERR_VAL;
    }

    http_callback = http_client_callback;

    char request[buf_len];
    const char *method_str;
    switch (method) {
        case HTTP_GET: method_str = "GET"; break;
        case HTTP_POST: method_str = "POST"; break;
        case HTTP_PUT: method_str = "PUT"; break;
        case HTTP_DELETE: method_str = "DELETE"; break;
        case HTTP_HEAD: method_str = "HEAD"; break;
        default: return ERR_VAL;
    }

    if (method == HTTP_POST || method == HTTP_PUT) {
        snprintf(request, sizeof(request),
                 "%s %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "%s"
                 "Content-Length: %d\r\n"
                 "Connection: close\r\n\r\n"
                 "%s",
                 method_str, path, server_ip, headers, body ? (int)strlen(body) : 0, body ? body : "");
    } else {
        snprintf(request, sizeof(request),
                 "%s %s HTTP/1.1\r\n"
                 "Host: %s\r\n"
                 "%s"
                 "Connection: close\r\n\r\n",
                 method_str, path, server_ip, headers);
    }

    altcp_connect(pcb, (ip_addr_t *)&ip, server_port, http_connected);
    altcp_arg(pcb, request);

    return ERR_OK;
}
