#include "../include/schannel_wrapper.h"
#include <stdio.h>
#include <string.h>

// Forward declarations
SECURITY_STATUS MapOpenSSLErrorToSecStatus(void);

// InitializeSecurityContextA - Client-side context initialization
SECURITY_STATUS SEC_ENTRY InitializeSecurityContextA(
    PCredHandle phCredential,
    PCtxtHandle phContext,
    SEC_CHAR *pszTargetName,
    unsigned long fContextReq,
    unsigned long Reserved1,
    unsigned long TargetDataRep,
    PSecBufferDesc pInput,
    unsigned long Reserved2,
    PCtxtHandle phNewContext,
    PSecBufferDesc pOutput,
    unsigned long *pfContextAttr,
    PTimeStamp ptsExpiry
) {
    LOG("InitializeSecurityContextA called, target: %s", pszTargetName ? pszTargetName : "NULL");
    
    PSCHANNEL_CONTEXT pCtx = NULL;
    BOOL isFirstCall = (phContext == NULL || phContext->dwLower == 0);
    
    if (isFirstCall) {
        // First call - create new context
        pCtx = (PSCHANNEL_CONTEXT)malloc(sizeof(SCHANNEL_CONTEXT));
        if (!pCtx) {
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        memset(pCtx, 0, sizeof(SCHANNEL_CONTEXT));
        
        // Get credentials
        if (phCredential && phCredential->dwUpper == 0xDEADBEEF) {
            PSCHANNEL_CRED pCred = (PSCHANNEL_CRED)phCredential->dwLower;
            if (pCred && pCred->ssl_ctx) {
                // Create SSL object
                pCtx->ssl = SSL_new(pCred->ssl_ctx);
                if (!pCtx->ssl) {
                    free(pCtx);
                    return SEC_E_INTERNAL_ERROR;
                }
                
                // Set up BIOs for non-blocking operation
                pCtx->rbio = BIO_new(BIO_s_mem());
                pCtx->wbio = BIO_new(BIO_s_mem());
                SSL_set_bio(pCtx->ssl, pCtx->rbio, pCtx->wbio);
                
                // Set as client
                SSL_set_connect_state(pCtx->ssl);
                pCtx->is_server = FALSE;
                
                // Set SNI if target name provided
                if (pszTargetName) {
                    SSL_set_tlsext_host_name(pCtx->ssl, pszTargetName);
                }
                
                LOG("Created new SSL context for client");
            }
        }
        
        // Store handle
        phNewContext->dwLower = (ULONG_PTR)pCtx;
        phNewContext->dwUpper = 0xBEEFDEAD; // Magic for context
    } else {
        // Continuation call
        pCtx = (PSCHANNEL_CONTEXT)phContext->dwLower;
        if (!pCtx || phContext->dwUpper != 0xBEEFDEAD) {
            return SEC_E_INVALID_HANDLE;
        }
    }
    
    // Process input if provided
    if (pInput && pInput->cBuffers > 0) {
        for (unsigned long i = 0; i < pInput->cBuffers; i++) {
            if (pInput->pBuffers[i].BufferType == SECBUFFER_TOKEN) {
                // Write data to SSL's input BIO
                BIO_write(pCtx->rbio, 
                         pInput->pBuffers[i].pvBuffer, 
                         pInput->pBuffers[i].cbBuffer);
                LOG("Wrote %d bytes to input BIO", pInput->pBuffers[i].cbBuffer);
            }
        }
    }
    
    // Perform handshake
    int ret = SSL_do_handshake(pCtx->ssl);
    int ssl_error = SSL_get_error(pCtx->ssl, ret);
    
    LOG("SSL_do_handshake returned %d, error: %d", ret, ssl_error);
    
    // Read output from SSL's write BIO
    if (pOutput && pOutput->cBuffers > 0) {
        for (unsigned long i = 0; i < pOutput->cBuffers; i++) {
            if (pOutput->pBuffers[i].BufferType == SECBUFFER_TOKEN) {
                int pending = BIO_ctrl_pending(pCtx->wbio);
                if (pending > 0) {
                    int to_read = (pending < (int)pOutput->pBuffers[i].cbBuffer) ? 
                                  pending : (int)pOutput->pBuffers[i].cbBuffer;
                    
                    int bytes_read = BIO_read(pCtx->wbio, 
                                             pOutput->pBuffers[i].pvBuffer, 
                                             to_read);
                    pOutput->pBuffers[i].cbBuffer = bytes_read;
                    
                    LOG("Read %d bytes from output BIO", bytes_read);
                } else {
                    pOutput->pBuffers[i].cbBuffer = 0;
                }
            }
        }
    }
    
    // Determine return status
    if (ret == 1) {
        // Handshake complete
        pCtx->handshake_complete = TRUE;
        LOG("TLS handshake completed successfully");
        
        if (pfContextAttr) {
            *pfContextAttr = ISC_RET_REPLAY_DETECT | ISC_RET_SEQUENCE_DETECT |
                           ISC_RET_CONFIDENTIALITY | ISC_RET_INTEGRITY;
        }
        
        return SEC_E_OK;
    } else if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
        // Need more data
        LOG("TLS handshake needs more data");
        return SEC_I_CONTINUE_NEEDED;
    } else {
        // Error
        LOG("TLS handshake failed");
        return MapOpenSSLErrorToSecStatus();
    }
}

// InitializeSecurityContextW - Unicode version
SECURITY_STATUS SEC_ENTRY InitializeSecurityContextW(
    PCredHandle phCredential,
    PCtxtHandle phContext,
    SEC_WCHAR *pszTargetName,
    unsigned long fContextReq,
    unsigned long Reserved1,
    unsigned long TargetDataRep,
    PSecBufferDesc pInput,
    unsigned long Reserved2,
    PCtxtHandle phNewContext,
    PSecBufferDesc pOutput,
    unsigned long *pfContextAttr,
    PTimeStamp ptsExpiry
) {
    LOG("InitializeSecurityContextW called");
    
    // Convert target name to ANSI
    char szTargetName[256] = {0};
    if (pszTargetName) {
        WideCharToMultiByte(CP_ACP, 0, pszTargetName, -1, szTargetName, sizeof(szTargetName), NULL, NULL);
    }
    
    return InitializeSecurityContextA(
        phCredential, phContext, szTargetName[0] ? szTargetName : NULL,
        fContextReq, Reserved1, TargetDataRep, pInput, Reserved2,
        phNewContext, pOutput, pfContextAttr, ptsExpiry
    );
}

// DeleteSecurityContext - Clean up context
SECURITY_STATUS SEC_ENTRY DeleteSecurityContext(
    PCtxtHandle phContext
) {
    LOG("DeleteSecurityContext called");
    
    if (!phContext || phContext->dwUpper != 0xBEEFDEAD) {
        return SEC_E_INVALID_HANDLE;
    }
    
    PSCHANNEL_CONTEXT pCtx = (PSCHANNEL_CONTEXT)phContext->dwLower;
    if (pCtx) {
        if (pCtx->ssl) {
            SSL_free(pCtx->ssl);
            // BIOs are freed automatically by SSL_free
        }
        free(pCtx);
    }
    
    phContext->dwLower = 0;
    phContext->dwUpper = 0;
    
    LOG("Context deleted successfully");
    return SEC_E_OK;
}

// AcceptSecurityContext - Server-side context initialization
SECURITY_STATUS SEC_ENTRY AcceptSecurityContext(
    PCredHandle phCredential,
    PCtxtHandle phContext,
    PSecBufferDesc pInput,
    unsigned long fContextReq,
    unsigned long TargetDataRep,
    PCtxtHandle phNewContext,
    PSecBufferDesc pOutput,
    unsigned long *pfContextAttr,
    PTimeStamp ptsExpiry
) {
    LOG("AcceptSecurityContext called");
    
    PSCHANNEL_CONTEXT pCtx = NULL;
    BOOL isFirstCall = (phContext == NULL || phContext->dwLower == 0);
    
    if (isFirstCall) {
        // Create new server context
        pCtx = (PSCHANNEL_CONTEXT)malloc(sizeof(SCHANNEL_CONTEXT));
        if (!pCtx) {
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        memset(pCtx, 0, sizeof(SCHANNEL_CONTEXT));
        
        if (phCredential && phCredential->dwUpper == 0xDEADBEEF) {
            PSCHANNEL_CRED pCred = (PSCHANNEL_CRED)phCredential->dwLower;
            if (pCred && pCred->ssl_ctx) {
                pCtx->ssl = SSL_new(pCred->ssl_ctx);
                if (!pCtx->ssl) {
                    free(pCtx);
                    return SEC_E_INTERNAL_ERROR;
                }
                
                pCtx->rbio = BIO_new(BIO_s_mem());
                pCtx->wbio = BIO_new(BIO_s_mem());
                SSL_set_bio(pCtx->ssl, pCtx->rbio, pCtx->wbio);
                
                SSL_set_accept_state(pCtx->ssl);
                pCtx->is_server = TRUE;
                
                LOG("Created new SSL context for server");
            }
        }
        
        phNewContext->dwLower = (ULONG_PTR)pCtx;
        phNewContext->dwUpper = 0xBEEFDEAD;
    } else {
        pCtx = (PSCHANNEL_CONTEXT)phContext->dwLower;
    }
    
    // Process similar to InitializeSecurityContext
    if (pInput && pInput->cBuffers > 0) {
        for (unsigned long i = 0; i < pInput->cBuffers; i++) {
            if (pInput->pBuffers[i].BufferType == SECBUFFER_TOKEN) {
                BIO_write(pCtx->rbio, 
                         pInput->pBuffers[i].pvBuffer, 
                         pInput->pBuffers[i].cbBuffer);
            }
        }
    }
    
    int ret = SSL_do_handshake(pCtx->ssl);
    int ssl_error = SSL_get_error(pCtx->ssl, ret);
    
    if (pOutput && pOutput->cBuffers > 0) {
        for (unsigned long i = 0; i < pOutput->cBuffers; i++) {
            if (pOutput->pBuffers[i].BufferType == SECBUFFER_TOKEN) {
                int pending = BIO_ctrl_pending(pCtx->wbio);
                if (pending > 0) {
                    int bytes_read = BIO_read(pCtx->wbio, 
                                             pOutput->pBuffers[i].pvBuffer, 
                                             pOutput->pBuffers[i].cbBuffer);
                    pOutput->pBuffers[i].cbBuffer = bytes_read;
                }
            }
        }
    }
    
    if (ret == 1) {
        pCtx->handshake_complete = TRUE;
        return SEC_E_OK;
    } else if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
        return SEC_I_CONTINUE_NEEDED;
    } else {
        return MapOpenSSLErrorToSecStatus();
    }
}
