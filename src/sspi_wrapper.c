#include "../include/schannel_wrapper.h"
#include <stdio.h>
#include <string.h>

// Forward declarations
SECURITY_STATUS MapOpenSSLErrorToSecStatus(void);

// AcquireCredentialsHandleA - Acquire credentials for authentication
SECURITY_STATUS SEC_ENTRY AcquireCredentialsHandleA(
    SEC_CHAR *pszPrincipal,
    SEC_CHAR *pszPackage,
    unsigned long fCredentialUse,
    void *pvLogonId,
    void *pAuthData,
    SEC_GET_KEY_FN pGetKeyFn,
    void *pvGetKeyArgument,
    PCredHandle phCredential,
    PTimeStamp ptsExpiry
) {
    LOG("AcquireCredentialsHandleA called for package: %s", pszPackage ? pszPackage : "NULL");
    
    // Check if this is for Schannel
    if (pszPackage && (strcmp(pszPackage, UNISP_NAME_A) == 0 || 
                       strcmp(pszPackage, "Schannel") == 0)) {
        
        // Allocate our credential structure
        PSCHANNEL_CRED pCred = (PSCHANNEL_CRED)malloc(sizeof(SCHANNEL_CRED));
        if (!pCred) {
            return SEC_E_INSUFFICIENT_MEMORY;
        }
        
        memset(pCred, 0, sizeof(SCHANNEL_CRED));
        
        // Determine if this is server or client
        pCred->is_server = (fCredentialUse == SECPKG_CRED_INBOUND);
        
        // Create OpenSSL context
        pCred->ssl_ctx = CreateSSLContext(pCred->is_server);
        if (!pCred->ssl_ctx) {
            free(pCred);
            return SEC_E_INTERNAL_ERROR;
        }
        
        // Store handle
        phCredential->dwLower = (ULONG_PTR)pCred;
        phCredential->dwUpper = 0xDEADBEEF; // Magic value to identify our handles
        
        // Set expiry to far future
        if (ptsExpiry) {
            ptsExpiry->LowPart = 0xFFFFFFFF;
            ptsExpiry->HighPart = 0x7FFFFFFF;
        }
        
        LOG("Credentials acquired successfully (OpenSSL-backed)");
        return SEC_E_OK;
    }
    
    // For non-Schannel packages, forward to original DLL
    HMODULE hOriginal = GetOriginalSchannelDLL();
    if (hOriginal) {
        typedef SECURITY_STATUS (SEC_ENTRY *AcquireCredentialsHandleA_t)(
            SEC_CHAR*, SEC_CHAR*, unsigned long, void*, void*,
            SEC_GET_KEY_FN, void*, PCredHandle, PTimeStamp);
        
        AcquireCredentialsHandleA_t pOrigFunc = 
            (AcquireCredentialsHandleA_t)GetProcAddress(hOriginal, "AcquireCredentialsHandleA");
        
        if (pOrigFunc) {
            return pOrigFunc(pszPrincipal, pszPackage, fCredentialUse, pvLogonId,
                           pAuthData, pGetKeyFn, pvGetKeyArgument, phCredential, ptsExpiry);
        }
    }
    
    return SEC_E_UNSUPPORTED_FUNCTION;
}

// AcquireCredentialsHandleW - Unicode version
SECURITY_STATUS SEC_ENTRY AcquireCredentialsHandleW(
    SEC_WCHAR *pszPrincipal,
    SEC_WCHAR *pszPackage,
    unsigned long fCredentialUse,
    void *pvLogonId,
    void *pAuthData,
    SEC_GET_KEY_FN pGetKeyFn,
    void *pvGetKeyArgument,
    PCredHandle phCredential,
    PTimeStamp ptsExpiry
) {
    LOG("AcquireCredentialsHandleW called");
    
    // Convert to ANSI and call A version
    char szPackage[256] = {0};
    if (pszPackage) {
        WideCharToMultiByte(CP_ACP, 0, pszPackage, -1, szPackage, sizeof(szPackage), NULL, NULL);
    }
    
    // For simplicity, call the A version
    return AcquireCredentialsHandleA(
        NULL,  // Principal - convert if needed
        szPackage[0] ? szPackage : NULL,
        fCredentialUse,
        pvLogonId,
        pAuthData,
        pGetKeyFn,
        pvGetKeyArgument,
        phCredential,
        ptsExpiry
    );
}

// FreeCredentialsHandle - Release credentials
SECURITY_STATUS SEC_ENTRY FreeCredentialsHandle(
    PCredHandle phCredential
) {
    LOG("FreeCredentialsHandle called");
    
    if (!phCredential) {
        return SEC_E_INVALID_HANDLE;
    }
    
    // Check if this is our handle
    if (phCredential->dwUpper == 0xDEADBEEF) {
        PSCHANNEL_CRED pCred = (PSCHANNEL_CRED)phCredential->dwLower;
        if (pCred) {
            if (pCred->ssl_ctx) {
                SSL_CTX_free(pCred->ssl_ctx);
            }
            free(pCred);
        }
        
        phCredential->dwLower = 0;
        phCredential->dwUpper = 0;
        
        LOG("Credentials freed successfully");
        return SEC_E_OK;
    }
    
    // Forward to original
    HMODULE hOriginal = GetOriginalSchannelDLL();
    if (hOriginal) {
        typedef SECURITY_STATUS (SEC_ENTRY *FreeCredentialsHandle_t)(PCredHandle);
        FreeCredentialsHandle_t pOrigFunc = 
            (FreeCredentialsHandle_t)GetProcAddress(hOriginal, "FreeCredentialsHandle");
        
        if (pOrigFunc) {
            return pOrigFunc(phCredential);
        }
    }
    
    return SEC_E_INVALID_HANDLE;
}

// FreeContextBuffer - Free memory allocated by SSPI
SECURITY_STATUS SEC_ENTRY FreeContextBuffer(
    void *pvContextBuffer
) {
    LOG("FreeContextBuffer called");
    
    if (pvContextBuffer) {
        free(pvContextBuffer);
        return SEC_E_OK;
    }
    
    return SEC_E_INVALID_HANDLE;
}
