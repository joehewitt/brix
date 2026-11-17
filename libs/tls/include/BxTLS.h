#ifndef BX_TLS_H
#define BX_TLS_H

#include <openssl/ssl.h>

extern "C" SSL_CTX* brixInitSSL(const char* clientCert, const char* clientKey, const char* caCert);
extern "C" int xSSL_is_init_finished(SSL* ssl);
extern "C" int xSSL_set_tlsext_host_name(SSL* s, const char* name);
extern "C" int xSSL_set1_host(SSL* s, const char* name);

#endif
