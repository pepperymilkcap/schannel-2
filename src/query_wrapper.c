#include "../include/schannel_wrapper.h"
#include <stdio.h>
#include <string.h>

// QueryContextAttributesA - Query context attributes
SECURITY_STATUS SEC_ENTRY QueryContextAttributesA(
    PCtxtHandle phContext,
    unsigned long ulAttribute,
    void *pBuffer
) {
    LOG("QueryContextAttributesA called, attribute: 0x%x", ulAttribute);
    
    if (!phContext || phContext->dwUpper != 0xBEEFDEAD) {
        return SEC_E_INVALID_HANDLE;
    }
    
    PSCHANNEL_CONTEXT pCtx = (PSCHANNEL_CONTEXT)phContext->dwLower;
    if (!pCtx || !pCtx->ssl) {
        return SEC_E_INVALID_HANDLE;
    }
    
    switch (ulAttribute) {
        case SECPKG_ATTR_STREAM_SIZES: {
            SecPkgContext_StreamSizes *pSizes = (SecPkgContext_StreamSizes*)pBuffer;
            if (!pSizes) {
                return SEC_E_INVALID_PARAMETER;
            }
            
            // Set typical TLS sizes
            pSizes->cbHeader = SSL3_RT_HEADER_LENGTH;
            pSizes->cbTrailer = SSL3_RT_MAX_ENCRYPTED_OVERHEAD;
            pSizes->cbMaximumMessage = SSL3_RT_MAX_PLAIN_LENGTH;
            pSizes->cBuffers = 4;
            pSizes->cbBlockSize = 1;
            
            LOG("Returned stream sizes");
            return SEC_E_OK;
        }
        
        case SECPKG_ATTR_REMOTE_CERT_CONTEXT: {
            X509 *peer_cert = SSL_get_peer_certificate(pCtx->ssl);
            if (!peer_cert) {
                LOG("No peer certificate");
                return SEC_E_NO_CREDENTIALS;
            }
            
            // Convert X509 to Windows CERT_CONTEXT
            // This is simplified - full implementation would need proper conversion
            PCCERT_CONTEXT *ppCertContext = (PCCERT_CONTEXT*)pBuffer;
            
            // Get DER encoding
            int der_len = i2d_X509(peer_cert, NULL);
            if (der_len <= 0) {
                X509_free(peer_cert);
                return SEC_E_INTERNAL_ERROR;
            }
            
            unsigned char *der_buf = (unsigned char*)malloc(der_len);
            unsigned char *p = der_buf;
            i2d_X509(peer_cert, &p);
            
            // Create certificate context from DER
            *ppCertContext = CertCreateCertificateContext(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                der_buf,
                der_len
            );
            
            free(der_buf);
            X509_free(peer_cert);
            
            if (*ppCertContext) {
                LOG("Returned remote certificate");
                return SEC_E_OK;
            } else {
                return SEC_E_INTERNAL_ERROR;
            }
        }
        
        case SECPKG_ATTR_CONNECTION_INFO: {
            SecPkgContext_ConnectionInfo *pInfo = (SecPkgContext_ConnectionInfo*)pBuffer;
            if (!pInfo) {
                return SEC_E_INVALID_PARAMETER;
            }
            
            // Get protocol version
            int version = SSL_version(pCtx->ssl);
            switch (version) {
                case SSL3_VERSION:
                    pInfo->dwProtocol = SP_PROT_SSL3_CLIENT;
                    break;
                case TLS1_VERSION:
                    pInfo->dwProtocol = SP_PROT_TLS1_CLIENT;
                    break;
                case TLS1_1_VERSION:
                    pInfo->dwProtocol = SP_PROT_TLS1_1_CLIENT;
                    break;
                case TLS1_2_VERSION:
                    pInfo->dwProtocol = SP_PROT_TLS1_2_CLIENT;
                    break;
                default:
                    pInfo->dwProtocol = 0;
            }
            
            // Get cipher info
            const SSL_CIPHER *cipher = SSL_get_current_cipher(pCtx->ssl);
            if (cipher) {
                pInfo->aiCipher = 0;  // Would need mapping
                pInfo->dwCipherStrength = SSL_CIPHER_get_bits(cipher, NULL);
                pInfo->aiHash = 0;    // Would need mapping
                pInfo->dwHashStrength = 0;
                pInfo->aiExch = 0;    // Would need mapping
                pInfo->dwExchStrength = 0;
            }
            
            LOG("Returned connection info");
            return SEC_E_OK;
        }
        
        case SECPKG_ATTR_CIPHER_INFO: {
            SecPkgContext_CipherInfo *pInfo = (SecPkgContext_CipherInfo*)pBuffer;
            if (!pInfo) {
                return SEC_E_INVALID_PARAMETER;
            }
            
            const SSL_CIPHER *cipher = SSL_get_current_cipher(pCtx->ssl);
            if (cipher) {
                const char *name = SSL_CIPHER_get_name(cipher);
                if (name) {
                    strncpy(pInfo->szCipherSuite, name, sizeof(pInfo->szCipherSuite) - 1);
                    pInfo->szCipherSuite[sizeof(pInfo->szCipherSuite) - 1] = '\0';
                }
                pInfo->dwCipherSuite = 0;
                pInfo->dwBaseCipherSuite = 0;
            }
            
            LOG("Returned cipher info");
            return SEC_E_OK;
        }
        
        default:
            LOG("Unsupported attribute: 0x%x", ulAttribute);
            return SEC_E_UNSUPPORTED_FUNCTION;
    }
}

// QueryContextAttributesW - Unicode version
SECURITY_STATUS SEC_ENTRY QueryContextAttributesW(
    PCtxtHandle phContext,
    unsigned long ulAttribute,
    void *pBuffer
) {
    // Most attributes are the same for A and W versions
    return QueryContextAttributesA(phContext, ulAttribute, pBuffer);
}

// ApplyControlToken - Apply control token to context
SECURITY_STATUS SEC_ENTRY ApplyControlToken(
    PCtxtHandle phContext,
    PSecBufferDesc pInput
) {
    LOG("ApplyControlToken called");
    
    if (!phContext || phContext->dwUpper != 0xBEEFDEAD) {
        return SEC_E_INVALID_HANDLE;
    }
    
    // In a full implementation, this would handle things like
    // shutdown notifications, renegotiation, etc.
    
    return SEC_E_OK;
}

// CompleteAuthToken - Complete authentication token
SECURITY_STATUS SEC_ENTRY CompleteAuthToken(
    PCtxtHandle phContext,
    PSecBufferDesc pToken
) {
    LOG("CompleteAuthToken called");
    
    // For TLS/SSL, this is typically not needed
    // as tokens are complete when generated
    
    return SEC_E_OK;
}

// QuerySecurityContextToken - Query security context token
SECURITY_STATUS SEC_ENTRY QuerySecurityContextToken(
    PCtxtHandle phContext,
    void **Token
) {
    LOG("QuerySecurityContextToken called");
    
    if (!phContext || phContext->dwUpper != 0xBEEFDEAD) {
        return SEC_E_INVALID_HANDLE;
    }
    
    // This would return a Windows access token
    // Not applicable for TLS contexts
    
    return SEC_E_UNSUPPORTED_FUNCTION;
}

// RevertSecurityContext - Revert impersonation
SECURITY_STATUS SEC_ENTRY RevertSecurityContext(
    PCtxtHandle phContext
) {
    LOG("RevertSecurityContext called");
    
    // Not applicable for TLS contexts
    return SEC_E_UNSUPPORTED_FUNCTION;
}

// EnumerateSecurityPackagesA - Enumerate available security packages
SECURITY_STATUS SEC_ENTRY EnumerateSecurityPackagesA(
    unsigned long *pcPackages,
    PSecPkgInfoA *ppPackageInfo
) {
    LOG("EnumerateSecurityPackagesA called");
    
    // Return information about Schannel package
    *pcPackages = 1;
    
    PSecPkgInfoA pInfo = (PSecPkgInfoA)malloc(sizeof(SecPkgInfoA));
    if (!pInfo) {
        return SEC_E_INSUFFICIENT_MEMORY;
    }
    
    pInfo->fCapabilities = SECPKG_FLAG_STREAM | SECPKG_FLAG_PRIVACY | 
                          SECPKG_FLAG_CONNECTION | SECPKG_FLAG_MULTI_REQUIRED;
    pInfo->wVersion = 1;
    pInfo->wRPCID = RPC_C_AUTHN_GSS_SCHANNEL;
    pInfo->cbMaxToken = 0x4000;
    pInfo->Name = _strdup(UNISP_NAME_A);
    pInfo->Comment = _strdup("OpenSSL-backed Schannel Security Package");
    
    *ppPackageInfo = pInfo;
    
    return SEC_E_OK;
}

// EnumerateSecurityPackagesW - Unicode version
SECURITY_STATUS SEC_ENTRY EnumerateSecurityPackagesW(
    unsigned long *pcPackages,
    PSecPkgInfoW *ppPackageInfo
) {
    LOG("EnumerateSecurityPackagesW called");
    
    *pcPackages = 1;
    
    PSecPkgInfoW pInfo = (PSecPkgInfoW)malloc(sizeof(SecPkgInfoW));
    if (!pInfo) {
        return SEC_E_INSUFFICIENT_MEMORY;
    }
    
    pInfo->fCapabilities = SECPKG_FLAG_STREAM | SECPKG_FLAG_PRIVACY | 
                          SECPKG_FLAG_CONNECTION | SECPKG_FLAG_MULTI_REQUIRED;
    pInfo->wVersion = 1;
    pInfo->wRPCID = RPC_C_AUTHN_GSS_SCHANNEL;
    pInfo->cbMaxToken = 0x4000;
    pInfo->Name = _wcsdup(L"Schannel");
    pInfo->Comment = _wcsdup(L"OpenSSL-backed Schannel Security Package");
    
    *ppPackageInfo = pInfo;
    
    return SEC_E_OK;
}

// QuerySecurityPackageInfoA - Query package information
SECURITY_STATUS SEC_ENTRY QuerySecurityPackageInfoA(
    SEC_CHAR *pszPackageName,
    PSecPkgInfoA *ppPackageInfo
) {
    LOG("QuerySecurityPackageInfoA called for: %s", pszPackageName);
    
    if (!pszPackageName || (strcmp(pszPackageName, UNISP_NAME_A) != 0 &&
                           strcmp(pszPackageName, "Schannel") != 0)) {
        return SEC_E_SECPKG_NOT_FOUND;
    }
    
    return EnumerateSecurityPackagesA(&(unsigned long){1}, ppPackageInfo);
}

// QuerySecurityPackageInfoW - Unicode version
SECURITY_STATUS SEC_ENTRY QuerySecurityPackageInfoW(
    SEC_WCHAR *pszPackageName,
    PSecPkgInfoW *ppPackageInfo
) {
    LOG("QuerySecurityPackageInfoW called");
    
    return EnumerateSecurityPackagesW(&(unsigned long){1}, ppPackageInfo);
}
