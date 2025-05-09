#include "path_response.h"
#include "fixed_response.h"
#include "static_file.h"
#include "header_time.h"
#include "form_input.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

void handle_request_path(SSL *ssl, const char *request) {
    /* 원본: :contentReference[oaicite:0]{index=0}:contentReference[oaicite:1]{index=1} */
    char method[8], url[256], protocol[16];
    sscanf(request, "%s %s %s", method, url, protocol);

    /* 1) PUT /upload/<filename> */
    if (strcmp(method, "PUT") == 0 && strncmp(url, "/upload/", 8) == 0) {
        const char *cl_header = strstr(request, "Content-Length:");
        int content_length = 0;
        if (cl_header) sscanf(cl_header, "Content-Length: %d", &content_length);

        const char *body = strstr(request, "\r\n\r\n");
        if (body) body += 4;

        /* 파일명 sanitizing */
        const char *raw = url + 8;
        char filename[256];
        int i;
        for (i = 0; i < (int)sizeof(filename)-1 && raw[i]; ++i)
            filename[i] = (raw[i] == '/' || raw[i] == '\\') ? '_' : raw[i];
        filename[i] = '\0';

        /* upload 디렉터리에 저장 */
        char path[512];
        snprintf(path, sizeof(path), "uploads/%s", filename);

        int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            const char *resp = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            SSL_write(ssl, resp, strlen(resp));
            return;
        }
        write(fd, body, content_length);   /* 파일 저장은 write 그대로 */
        close(fd);

        const char *resp = "HTTP/1.1 201 Created\r\n\r\n";
        SSL_write(ssl, resp, strlen(resp));
        return;
    }

    /* 2) GET /greet?...  */
    if (strncmp(url, "/greet?", 7) == 0) {
        handle_form_input(ssl, url);
        return;
    }

    /* 3) 기타 GET */
    if (strcmp(url, "/") == 0) {
        send_fixed_response(ssl);
    } else if (strcmp(url, "/hello") == 0) {
        const char *body = "<html><body><h1>Hello, User!</h1></body></html>";
        char resp[512];
        int len = snprintf(resp, sizeof(resp),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=UTF-8\r\n"
             "Content-Length: %zu\r\n\r\n%s",
             strlen(body), body);
        SSL_write(ssl, resp, len);
    } else if (strcmp(url, "/index.html") == 0) {
        serve_static_file(ssl, "index.html");
    } else if (strcmp(url, "/time") == 0) {
        send_response_with_time(ssl);
    } else {
        const char *body = "<html><body><h1>404 - Page Not Found</h1></body></html>";
        char resp[512];
        int len = snprintf(resp, sizeof(resp),
             "HTTP/1.1 404 Not Found\r\n"
             "Content-Type: text/html; charset=UTF-8\r\n"
             "Content-Length: %zu\r\n\r\n%s",
             strlen(body), body);
        SSL_write(ssl, resp, len);
    }
}


