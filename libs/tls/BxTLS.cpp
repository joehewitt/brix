#include "BxTLS/BxTLS.h"

#include <openssl/err.h>

extern "C" SSL_CTX*
brixInitSSL(const char* clientCert, const char* clientKey, const char* caCert) {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    SSL_CTX* ctx = SSL_CTX_new(SSLv23_method());
    if (!ctx) {
        return NULL;
    }

    SSL_CTX_set_options(ctx, SSL_OP_SINGLE_DH_USE);

    SSL_CTX_set_mode(ctx, SSL_MODE_RELEASE_BUFFERS);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    bool hasClientCert = clientCert && clientCert[0];
    bool hasClientKey = clientKey && clientKey[0];
    if (hasClientCert || hasClientKey) {
        if (!hasClientCert || !hasClientKey) {
            SSL_CTX_free(ctx);
            return NULL;
        }

        if (SSL_CTX_use_certificate_file(ctx, clientCert, SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(ctx);
            return NULL;
        }

        if (SSL_CTX_use_PrivateKey_file(ctx, clientKey, SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(ctx);
            return NULL;
        }
    }

    int loadedDefaultCA = SSL_CTX_set_default_verify_paths(ctx);
    int loadedCustomCA = 0;
    if (caCert && caCert[0]) {
        loadedCustomCA = SSL_CTX_load_verify_locations(ctx, caCert, NULL);
        if (!loadedCustomCA) {
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(ctx);
            return NULL;
        }
        if (!loadedDefaultCA) {
            ERR_clear_error();
        }
    }
    if (!loadedDefaultCA && !loadedCustomCA) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

extern "C" int
xSSL_is_init_finished(SSL* ssl) {
    return SSL_is_init_finished(ssl);
}

extern "C" int
xSSL_set_tlsext_host_name(SSL* s, const char* name) {
    return SSL_set_tlsext_host_name(s, name);
}

extern "C" int
xSSL_set1_host(SSL* s, const char* name) {
    return SSL_set1_host(s, name);
}
