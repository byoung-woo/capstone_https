#include "form_input.h"
#include <openssl/ssl.h>
#include <stdio.h>
#include <string.h>

#define MAX_VAL 128

void handle_form_input(SSL *ssl, const char *path) {   /* 원본: :contentReference[oaicite:8]{index=8}:contentReference[oaicite:9]{index=9} */
    const char *q = strchr(path, '?');
    q = q ? q + 1 : "";

    char name[MAX_VAL] = "Guest", age[MAX_VAL] = "N/A", lang[MAX_VAL] = "en";

    char qs[512]; strncpy(qs, q, sizeof(qs)-1); qs[sizeof(qs)-1] = '\0';
    char *tok = strtok(qs, "&");
    while (tok) {
        char k[64], v[MAX_VAL];
        if (sscanf(tok, "%63[^=]=%127s", k, v) == 2) {
            if (!strcmp(k,"name")) strncpy(name,v,MAX_VAL-1);
            else if (!strcmp(k,"age")) strncpy(age,v,MAX_VAL-1);
            else if (!strcmp(k,"lang")) strncpy(lang,v,MAX_VAL-1);
        }
        tok = strtok(NULL,"&");
    }

    char body[512];
    int blen = snprintf(body,sizeof(body),
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body>"
        "<h1>Hello, %s!</h1><p>Age: %s</p><p>Language: %s</p></body></html>",
        name, age, lang);

    char header[256];
    int hlen = snprintf(header,sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: %d\r\n\r\n", blen);

    SSL_write(ssl, header, hlen);
    SSL_write(ssl, body, blen);
}


