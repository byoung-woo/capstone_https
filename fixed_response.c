
#include "fixed_response.h"
#include <openssl/ssl.h>
#include <string.h>
#include <stdio.h>

void send_fixed_response(SSL *ssl) {
    /* 원본: :contentReference[oaicite:4]{index=4}:contentReference[oaicite:5]{index=5} */
    const char *body =
        "<html><body><h1>You connected well to the server!</h1></body></html>";
    char resp[512];
    int len = snprintf(resp, sizeof(resp),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=UTF-8\r\n"
             "Content-Length: %zu\r\n\r\n%s",
             strlen(body), body);
    SSL_write(ssl, resp, len);
}


