#ifndef SCHANNEL_WRAPPER_H
#define SCHANNEL_WRAPPER_H

#include <windows.h>
#include <wincrypt.h>
#include <schannel.h>
#include <sspi.h>

// OpenSSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

// Logging and debugging
#ifdef DEBUG
#define LOG(fmt, ...) fprintf(stderr, "[schannel-wrapper] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...) ((void)0)
#endif

// Context structure to hold OpenSSL state
typedef struct _SCHANNEL_CONTEXT {
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    BIO *rbio;  // Read BIO (network -> SSL)
    BIO *wbio;  // Write BIO (SSL -> network)
    BOOL is_server;
    BOOL handshake_complete;
    CtxtHandle original_context;
} SCHANNEL_CONTEXT, *PSCHANNEL_CONTEXT;

// Credentials structure
typedef struct _SCHANNEL_CRED {
    CredHandle original_cred;
    SSL_CTX *ssl_ctx;
    BOOL is_server;
} SCHANNEL_CRED, *PSCHANNEL_CRED;

// Function prototypes for internal use
BOOL InitializeOpenSSL(void);
void CleanupOpenSSL(void);
HMODULE GetOriginalSchannelDLL(void);

// Helper functions for OpenSSL integration
SSL_CTX* CreateSSLContext(BOOL is_server);
int MapSchannelAlgsToOpenSSL(DWORD alg_id);

#endif // SCHANNEL_WRAPPER_H
