/*
 *--------------------------------------
 * Program Name: lwIP-CE HTTP Client
 * Author: TKB Studios
 * License: MIT License 
 * Description: An HTTP Client for lwIP-CE
 *--------------------------------------
*/

#include "http-client.h"

http_client_callback_t user_http_callback = NULL;

bool wait_for_dns_resolution = false;
long long dns_resolution_time_start = 0;
ip_addr_t dns_ipaddr;

void http_handle_all_events()
{
    usb_HandleEvents();       // usb events
    sys_check_timeouts();     // lwIP timers/event callbacks
}


void http_dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    printf("DNS callback: %s\n", name);
    dns_ipaddr = *ipaddr;
    wait_for_dns_resolution = false;
}

err_t http_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    printf("http_recv start\n");

    printf("http_recv size: %d\n", p->len);

    if (user_http_callback) {
        printf("Calling user callback\n");
        user_http_callback(p, err);
    }

    if (p == NULL) {
        printf("HTTP close, p is NULL\n");
        altcp_close(pcb);
    } else {
        printf("HTTP close, p is not NULL\n");
        altcp_recved(pcb, p->len);
        pbuf_free(p);
    }
    printf("http_recv done\n");
    return ERR_OK;
}

err_t http_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    printf("http_connected start\n");
    if (err != ERR_OK) {
        printf("Error connecting: %d\n", err);
        return err;
    }
    
    printf("HTTP connected\n");

    char *request = (char *)arg;
    printf("Sending HTTP request:\n%s\n", request);

    altcp_recv(pcb, http_recv);
    altcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    altcp_output(pcb);
    printf("http_connected done\n");
    return ERR_OK;
}

err_t http_request(
    http_method_t method,
    const char *host,
    uint16_t server_port,
    const char *path,
    const char *headers,
    const char *body,
    unsigned int buf_len,
    http_client_callback_t callback
) {
    if (!host || !path || !headers || !callback) {
        return ERR_VAL;
    }

    printf("http_request start\n");
    user_http_callback = callback;

    struct altcp_pcb *pcb = altcp_tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) {
        return ERR_MEM;
    }

    ip_addr_t host_ip;
    if (!ipaddr_aton(host, &host_ip)) {
        printf("Invalid IP address\n");

        // attempt DNS resolution
        dns_init();
        err_t dns_resolution_err = dns_gethostbyname(host, (ip_addr_t *)&host_ip, http_dns_callback, NULL);
        switch (dns_resolution_err) {
            case ERR_OK:
                printf("DNS resolution successful\n");
                break;
            case ERR_INPROGRESS:
                printf("DNS resolution in progress\n");
                dns_resolution_time_start = clock() * 1000 / CLOCKS_PER_SEC;
                wait_for_dns_resolution = true;
                while (wait_for_dns_resolution)
                {
                    if ((clock() * 1000 / CLOCKS_PER_SEC) - dns_resolution_time_start > 16000) // 16 second timeout
                    {
                        printf("DNS resolution timed out\n");
                        return ERR_TIMEOUT;
                    }
                    http_handle_all_events();
                }
                printf("DNS resolution done\n");
                break;
            default:
                printf("DNS resolution failed, code: %d\n", dns_resolution_err);
            return dns_resolution_err;
        }
    }

    char ip_addr_str[IPADDR_STRLEN_MAX];
    ipaddr_ntoa_r(&host_ip, ip_addr_str, IPADDR_STRLEN_MAX);
    printf("Host IP: %s\n", ip_addr_str);

    const char *method_str;
    switch (method) {
        case HTTP_GET: method_str = "GET"; break;
        case HTTP_POST: method_str = "POST"; break;
        case HTTP_PUT: method_str = "PUT"; break;
        case HTTP_DELETE: method_str = "DELETE"; break;
        case HTTP_HEAD: method_str = "HEAD"; break;
        default: return ERR_VAL;
    }

    char request[buf_len];
    snprintf(
        request,
        sizeof(request),
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: tkbstudios/lwIP-CE-HTTP-Client version 1.0 (TI-84 Plus CE)\r\n"
        "%s"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        method_str, path, host, headers, body ? (int)strlen(body) : 0, body ? body : ""
    );
    printf("Request:\n%s\n", request);

    printf("http_request sending\n");
    altcp_connect(pcb, &host_ip, server_port, http_connected);
    altcp_arg(pcb, request);

    printf("http_request done\n");
    return ERR_OK;
}
