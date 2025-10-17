#include "../include/schannel_wrapper.h"
#include <stdio.h>
#include <string.h>

// EncryptMessage - Encrypt application data
SECURITY_STATUS SEC_ENTRY EncryptMessage(
    PCtxtHandle phContext,
    unsigned long fQOP,
    PSecBufferDesc pMessage,
    unsigned long MessageSeqNo
) {
    LOG("EncryptMessage called");
    
    if (!phContext || phContext->dwUpper != 0xBEEFDEAD) {
        return SEC_E_INVALID_HANDLE;
    }
    
    PSCHANNEL_CONTEXT pCtx = (PSCHANNEL_CONTEXT)phContext->dwLower;
    if (!pCtx || !pCtx->ssl || !pCtx->handshake_complete) {
        return SEC_E_INVALID_HANDLE;
    }
    
    // Find data buffer
    PSecBuffer pDataBuffer = NULL;
    PSecBuffer pTokenBuffer = NULL;
    
    for (unsigned long i = 0; i < pMessage->cBuffers; i++) {
        if (pMessage->pBuffers[i].BufferType == SECBUFFER_DATA) {
            pDataBuffer = &pMessage->pBuffers[i];
        } else if (pMessage->pBuffers[i].BufferType == SECBUFFER_TOKEN) {
            pTokenBuffer = &pMessage->pBuffers[i];
        }
    }
    
    if (!pDataBuffer || !pDataBuffer->pvBuffer) {
        return SEC_E_INVALID_TOKEN;
    }
    
    // Write plaintext to SSL
    int bytes_written = SSL_write(pCtx->ssl, 
                                  pDataBuffer->pvBuffer, 
                                  pDataBuffer->cbBuffer);
    
    if (bytes_written <= 0) {
        LOG("SSL_write failed");
        return SEC_E_ENCRYPT_FAILURE;
    }
    
    LOG("SSL_write succeeded, %d bytes", bytes_written);
    
    // Read encrypted data from write BIO
    if (pTokenBuffer) {
        int pending = BIO_ctrl_pending(pCtx->wbio);
        if (pending > 0) {
            int bytes_read = BIO_read(pCtx->wbio, 
                                     pTokenBuffer->pvBuffer, 
                                     pTokenBuffer->cbBuffer);
            pTokenBuffer->cbBuffer = bytes_read;
            LOG("Read %d encrypted bytes from BIO", bytes_read);
        } else {
            pTokenBuffer->cbBuffer = 0;
        }
    }
    
    return SEC_E_OK;
}

// DecryptMessage - Decrypt application data
SECURITY_STATUS SEC_ENTRY DecryptMessage(
    PCtxtHandle phContext,
    PSecBufferDesc pMessage,
    unsigned long MessageSeqNo,
    unsigned long *pfQOP
) {
    LOG("DecryptMessage called");
    
    if (!phContext || phContext->dwUpper != 0xBEEFDEAD) {
        return SEC_E_INVALID_HANDLE;
    }
    
    PSCHANNEL_CONTEXT pCtx = (PSCHANNEL_CONTEXT)phContext->dwLower;
    if (!pCtx || !pCtx->ssl || !pCtx->handshake_complete) {
        return SEC_E_INVALID_HANDLE;
    }
    
    // Find data and token buffers
    PSecBuffer pDataBuffer = NULL;
    PSecBuffer pTokenBuffer = NULL;
    
    for (unsigned long i = 0; i < pMessage->cBuffers; i++) {
        if (pMessage->pBuffers[i].BufferType == SECBUFFER_DATA) {
            pDataBuffer = &pMessage->pBuffers[i];
        } else if (pMessage->pBuffers[i].BufferType == SECBUFFER_TOKEN) {
            pTokenBuffer = &pMessage->pBuffers[i];
        }
    }
    
    if (!pDataBuffer) {
        return SEC_E_INVALID_TOKEN;
    }
    
    // Write encrypted data to SSL's read BIO
    if (pDataBuffer->pvBuffer && pDataBuffer->cbBuffer > 0) {
        BIO_write(pCtx->rbio, pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);
        LOG("Wrote %d bytes to read BIO", pDataBuffer->cbBuffer);
    }
    
    // Read decrypted data from SSL
    char buffer[16384];
    int bytes_read = SSL_read(pCtx->ssl, buffer, sizeof(buffer));
    
    if (bytes_read > 0) {
        LOG("SSL_read succeeded, %d bytes", bytes_read);
        
        // Copy to output buffer
        if (pDataBuffer->cbBuffer >= (unsigned long)bytes_read) {
            memcpy(pDataBuffer->pvBuffer, buffer, bytes_read);
            pDataBuffer->cbBuffer = bytes_read;
        } else {
            return SEC_E_BUFFER_TOO_SMALL;
        }
        
        if (pfQOP) {
            *pfQOP = 0;
        }
        
        return SEC_E_OK;
    } else {
        int ssl_error = SSL_get_error(pCtx->ssl, bytes_read);
        
        if (ssl_error == SSL_ERROR_WANT_READ) {
            LOG("SSL_read needs more data");
            return SEC_E_INCOMPLETE_MESSAGE;
        } else {
            LOG("SSL_read failed, error: %d", ssl_error);
            return SEC_E_DECRYPT_FAILURE;
        }
    }
}

// SealMessage - Alias for EncryptMessage
SECURITY_STATUS SEC_ENTRY SealMessage(
    PCtxtHandle phContext,
    unsigned long fQOP,
    PSecBufferDesc pMessage,
    unsigned long MessageSeqNo
) {
    return EncryptMessage(phContext, fQOP, pMessage, MessageSeqNo);
}

// UnsealMessage - Alias for DecryptMessage
SECURITY_STATUS SEC_ENTRY UnsealMessage(
    PCtxtHandle phContext,
    PSecBufferDesc pMessage,
    unsigned long MessageSeqNo,
    unsigned long *pfQOP
) {
    return DecryptMessage(phContext, pMessage, MessageSeqNo, pfQOP);
}

// MakeSignature - Create message signature
SECURITY_STATUS SEC_ENTRY MakeSignature(
    PCtxtHandle phContext,
    unsigned long fQOP,
    PSecBufferDesc pMessage,
    unsigned long MessageSeqNo
) {
    LOG("MakeSignature called - forwarding to EncryptMessage");
    // For TLS, signing is part of encryption
    return EncryptMessage(phContext, fQOP, pMessage, MessageSeqNo);
}

// VerifySignature - Verify message signature
SECURITY_STATUS SEC_ENTRY VerifySignature(
    PCtxtHandle phContext,
    PSecBufferDesc pMessage,
    unsigned long MessageSeqNo,
    unsigned long *pfQOP
) {
    LOG("VerifySignature called - forwarding to DecryptMessage");
    // For TLS, verification is part of decryption
    return DecryptMessage(phContext, pMessage, MessageSeqNo, pfQOP);
}
