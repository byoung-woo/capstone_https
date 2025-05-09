
#ifndef STATIC_FILE_H
#define STATIC_FILE_H

#include <openssl/ssl.h>

/* filename에 해당하는 정적 파일을 SSL로 전송 */
void serve_static_file(SSL *ssl, const char *filename);

#endif /* STATIC_FILE_H */

