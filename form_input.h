
#ifndef FORM_INPUT_H
#define FORM_INPUT_H

#include <openssl/ssl.h>

/* GET /greet?name=... 등 폼 입력을 파싱해 응답 생성 */
void handle_form_input(SSL *ssl, const char *path);

#endif /* FORM_INPUT_H */

