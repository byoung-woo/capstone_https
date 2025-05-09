#include "header_time.h"
#include <openssl/ssl.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

void send_response_with_time(SSL *ssl) {        /* 원본: :contentReference[oaicite:6]{index=6}:contentReference[oaicite:7]{index=7} */
    time_t now = time(NULL);
    char tstr[64];
    strftime(tstr, sizeof(tstr), "%Y-%m-%d %H:%M:%S", localtime(&now));

    char body[256];
    int blen = snprintf(body, sizeof(body),
        "<html><body><h1>Current time: %s</h1></body></html>", tstr);

    char header[256];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n\r\n", blen);

    SSL_write(ssl, header, hlen);
    SSL_write(ssl, body, blen);
}


