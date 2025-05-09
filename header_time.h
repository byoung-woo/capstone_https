#ifndef HEADER_TIME_H
#define HEADER_TIME_H

#include <openssl/ssl.h>

/* 현재 시간을 포함한 HTML 페이지 전송 */
void send_response_with_time(SSL *ssl);

#endif /* HEADER_TIME_H */