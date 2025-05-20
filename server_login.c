#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define HTTPS_PORT 4433
#define BUFFER_SIZE 8192

int main(int argc, char *argv[]) {
    int port = HTTPS_PORT;
    if (argc >= 2) {
        port = atoi(argv[1]); // allow optional port argument
        if (port <= 0) port = HTTPS_PORT;
    }
    // SIGPIPE 시그널 무시 ( 크래시 방지 )
    signal(SIGPIPE, SIG_IGN);
    // OpenSSL 라이브러리 초기화
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    // TLS 서버용 SSL 컨텍스트 생성
    const SSL_METHOD *method = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        fprintf(stderr, "Failed to create SSL context\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }
    // 서버 인증서와 개인 키 로드
    if (SSL_CTX_use_certificate_file(ctx, "server.crn", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }
    // 인증서와 개인 키 검증
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "Private key does not match the public certificate\n");
        return 1;
    }
    // TCP 소켓 생성
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Unable to create socket");
        return 1;
    }
    // 주소 재사용 설정 (재시작 시 "Address already in use" 방지)
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    // 모든 인터페이스(0.0.0.0)에 지정 포트로 바인딩
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to bind");
        close(sock);
        return 1;
    }
    if (listen(sock, 1) < 0) {
        perror("Unable to listen");
        close(sock);
        return 1;
    }
    printf("HTTPS server listening on port %d...\n", port);
    // 메인 루프: 클라이언트 연결 수락 및 처리
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        // 새 TCP 연결 수락
        int client_sock = accept(sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Unable to accept");
            break;
        }
        // SSL 객체 생성 (클라이언트용)
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_sock);
        // 클라이언트와 SSL/TLS 핸드셰이크 수행
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_sock);
            continue;
        }
        // SSL 연결에서 데이터 수신 (HTTP 요청 읽기)
        char buffer[BUFFER_SIZE];
        int bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(client_sock);
            continue;
        }
        buffer[bytes_read] = '\0'; // NULL-terminate to treat as string
        // HTTP POST 요청 본문 찾기 ("\r\n\r\n" 이후)
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) {
            body += 4; // skip past the "\r\n\r\n"
        } else {
            body = buffer; // in case no headers (unlikely in valid HTTP)
        }
        // Content-Length 헤더가 있으면 본문 전체를 읽도록 처리
        char *cl_header = strcasestr(buffer, "Content-Length:");
        int content_length = 0;
        if (cl_header) {
            content_length = atoi(cl_header + strlen("Content-Length:"));
        }
        int received_body_len = bytes_read - (body - buffer);
        // 받은 바이트가 Content-Length보다 적으면 추가로 읽기
        while (content_length > 0 && received_body_len < content_length) {
            int more = SSL_read(ssl, buffer + bytes_read, sizeof(buffer) - 1 - bytes_read);
            if (more <= 0) break;
            bytes_read += more;
            buffer[bytes_read] = '\0';
            received_body_len = bytes_read - (body - buffer);
        }
        // 본문에서 "id"와 "pw" 추출
        char id_val[256] = {0};
        char pw_val[256] = {0};
        // (본문은 "id=...&pw=..." 형식이라고 가정)
        char *amp = strchr(body, '&');
        if (amp) {
            *amp = '\0'; // split into two strings at the '&'
            amp++;
            if (strncmp(body, "id=", 3) == 0) {
                strncpy(id_val, body + 3, sizeof(id_val) - 1);
            }
            if (strncmp(amp, "pw=", 3) == 0) {
                strncpy(pw_val, amp + 3, sizeof(pw_val) - 1);
            }
        } else {
            // 만약 '&'가 없으면 단일 값으로 간주 (본 예에서는 해당 없음)
            if (strncmp(body, "id=", 3) == 0) {
                strncpy(id_val, body + 3, sizeof(id_val) - 1);
            }
        }
        // 수신한 자격 증명(ID/PW) 출력
        printf("Received ID: %s\n", id_val);
        printf("Received PW: %s\n", pw_val);
        // HTTP 200 OK 응답 전송
        const char *response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: 2\r\n"
                               "\r\n"
                               "OK";
        SSL_write(ssl, response, strlen(response));
        // SSL 연결 종료 및 정리
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_sock);
        // 다음 연결 대기를 위해 루프로 복귀
        printf("Handled one connection, waiting for next...\n");
    }
    // 마무리: 서버 소켓 닫기 및 SSL 컨텍스트 해제
    close(sock);
    SSL_CTX_free(ctx);
    return 0;
}
