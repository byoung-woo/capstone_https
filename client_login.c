#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define DEFAULT_PORT 4433

int main(int argc, char *argv[]) {
    const char *hostname = "127.0.0.1";
    int port = DEFAULT_PORT;
    if (argc >= 2) {
        hostname = argv[1]; // allow specify host
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
        if (port <= 0) port = DEFAULT_PORT;
    }
    // OpenSSL 라이브러리 초기화
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        fprintf(stderr, "Unable to create SSL context\n");
        ERR_print_errors_fp(stderr);
        return 1;
    }
    // 자체 서명 인증서 사용 시 서버 인증서 검증 생략 (데모 목적상 보안 설정 완화)
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
    // TCP 소켓 생성
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        SSL_CTX_free(ctx);
        return 1;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, hostname, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address/ Address not supported\n");
        close(sock);
        SSL_CTX_free(ctx);
        return 1;
    }
    // 서버에 TCP 연결
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        SSL_CTX_free(ctx);
        return 1;
    }
    // SSL 객체 생성 및 소켓 연결
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        close(sock);
        SSL_CTX_free(ctx);
        return 1;
    }
    // 사용자 ID와 비밀번호 입력 받기
    char id[256], pw[256];
    printf("Enter ID: ");
    fflush(stdout);
    if (!fgets(id, sizeof(id), stdin)) {
        // 입력 오류 (stdin에서 ID/PW 읽기 실패)
        close(sock);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        return 1;
    }
    // 개행 문자 제거
    size_t len = strlen(id);
    if (len > 0 && id[len-1] == '\n') id[len-1] = '\0';
    printf("Enter PW: ");
    fflush(stdout);
    if (!fgets(pw, sizeof(pw), stdin)) {
        close(sock);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        return 1;
    }
    len = strlen(pw);
    if (len > 0 && pw[len-1] == '\n') pw[len-1] = '\0';
    // HTTP POST 요청 데이터 준비 (application/x-www-form-urlencoded 사용)
    char body[512];
    snprintf(body, sizeof(body), "id=%s&pw=%s", id, pw);
    int body_len = strlen(body);
    char request[1024];
    snprintf(request, sizeof(request),
             "POST / HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             hostname, port, body_len, body);
    // SSL로 요청 보내기
    if (SSL_write(ssl, request, strlen(request)) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sock);
        SSL_CTX_free(ctx);
        return 1;
    }
    // 서버 응답 읽어서 출력
    char resp_buf[1024];
    int bytes;
    printf("Server response:\n");
    while ((bytes = SSL_read(ssl, resp_buf, sizeof(resp_buf) - 1)) > 0) {
        resp_buf[bytes] = '\0';
        printf("%s", resp_buf);
    }
    if (bytes < 0) {
        ERR_print_errors_fp(stderr);
    }
    printf("\n");
    // 마무리 작업 후 종료
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
    return 0;
}
