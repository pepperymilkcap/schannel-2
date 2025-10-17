#include "../include/schannel_wrapper.h"
#include <stdio.h>

// Create SSL context for client or server
SSL_CTX* CreateSSLContext(BOOL is_server) {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    // Use TLS method that supports all versions
    if (is_server) {
        method = SSLv23_server_method();
    } else {
        method = SSLv23_client_method();
    }

    ctx = SSL_CTX_new(method);
    if (!ctx) {
        LOG("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    // Set options for better security
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    
    // Enable all modern cipher suites
    SSL_CTX_set_cipher_list(ctx, "HIGH:!aNULL:!MD5:!RC4");
    
    // Set minimum protocol version to TLS 1.0 for XP compatibility
    // but allow up to TLS 1.2
    SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);
    
    LOG("SSL context created successfully");
    return ctx;
}

// Map Schannel algorithm IDs to OpenSSL equivalents
int MapSchannelAlgsToOpenSSL(DWORD alg_id) {
    switch (alg_id) {
        case CALG_3DES:
            return NID_des_ede3_cbc;
        case CALG_AES_128:
            return NID_aes_128_cbc;
        case CALG_AES_256:
            return NID_aes_256_cbc;
        case CALG_RC4:
            return NID_rc4;
        case CALG_DES:
            return NID_des_cbc;
        case CALG_RSA_KEYX:
            return NID_rsaEncryption;
        case CALG_SHA1:
            return NID_sha1;
        case CALG_MD5:
            return NID_md5;
        case CALG_SHA_256:
            return NID_sha256;
        case CALG_SHA_384:
            return NID_sha384;
        case CALG_SHA_512:
            return NID_sha512;
        default:
            LOG("Unknown algorithm ID: 0x%x", alg_id);
            return 0;
    }
}

// Convert OpenSSL error to Windows error
SECURITY_STATUS MapOpenSSLErrorToSecStatus(void) {
    unsigned long err = ERR_get_error();
    
    if (err == 0) {
        return SEC_E_OK;
    }
    
    LOG("OpenSSL error: %s", ERR_error_string(err, NULL));
    
    // Map common errors
    int reason = ERR_GET_REASON(err);
    
    switch (reason) {
        case SSL_R_CERTIFICATE_VERIFY_FAILED:
            return SEC_E_CERT_EXPIRED;
        case SSL_R_UNKNOWN_PROTOCOL:
            return SEC_E_UNSUPPORTED_FUNCTION;
        case SSL_R_DECRYPTION_FAILED:
            return SEC_E_DECRYPT_FAILURE;
        case SSL_R_BAD_SIGNATURE:
            return SEC_E_MESSAGE_ALTERED;
        default:
            return SEC_E_INTERNAL_ERROR;
    }
}
