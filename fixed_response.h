
#ifndef FIXED_RESPONSE_H
#define FIXED_RESPONSE_H

#include <openssl/ssl.h>

/* 기본 200 OK HTML 페이지 전송 */
void send_fixed_response(SSL *ssl);

#endif /* FIXED_RESPONSE_H */

