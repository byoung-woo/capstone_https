#include "static_file.h"
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void serve_static_file(SSL *ssl, const char *filename) {
    /* 원본: :contentReference[oaicite:2]{index=2}:contentReference[oaicite:3]{index=3} */
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        const char *resp =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: 56\r\n\r\n"
            "<html><body><h1>404 - File Not Found</h1></body></html>";
        SSL_write(ssl, resp, strlen(resp));
        return;
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char header[256];
    int hlen = snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n\r\n", filesize);
    SSL_write(ssl, header, hlen);

    /* 본문 스트리밍 */
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        SSL_write(ssl, buf, n);

    fclose(fp);
}


