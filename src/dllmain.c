#include "../include/schannel_wrapper.h"
#include <stdio.h>

// Global variables
static HMODULE g_hOriginalSchannel = NULL;
static BOOL g_bOpenSSLInitialized = FALSE;

// Get path to original schannel.dll (renamed or in system32)
HMODULE GetOriginalSchannelDLL(void) {
    if (g_hOriginalSchannel != NULL) {
        return g_hOriginalSchannel;
    }

    char systemPath[MAX_PATH];
    char dllPath[MAX_PATH];
    
    // Try to load the original DLL (renamed to schannel_orig.dll)
    GetSystemDirectoryA(systemPath, MAX_PATH);
    sprintf(dllPath, "%s\\schannel_orig.dll", systemPath);
    
    g_hOriginalSchannel = LoadLibraryA(dllPath);
    
    if (g_hOriginalSchannel == NULL) {
        LOG("Failed to load original schannel.dll from %s", dllPath);
        // Could try other locations or fail gracefully
    } else {
        LOG("Loaded original schannel.dll from %s", dllPath);
    }
    
    return g_hOriginalSchannel;
}

// Initialize OpenSSL
BOOL InitializeOpenSSL(void) {
    if (g_bOpenSSLInitialized) {
        return TRUE;
    }

    LOG("Initializing OpenSSL...");
    
    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    g_bOpenSSLInitialized = TRUE;
    LOG("OpenSSL initialized successfully");
    
    return TRUE;
}

// Cleanup OpenSSL
void CleanupOpenSSL(void) {
    if (!g_bOpenSSLInitialized) {
        return;
    }

    LOG("Cleaning up OpenSSL...");
    
    EVP_cleanup();
    ERR_free_strings();
    
    g_bOpenSSLInitialized = FALSE;
}

// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            LOG("schannel wrapper DLL loaded");
            DisableThreadLibraryCalls(hinstDLL);
            
            // Initialize OpenSSL
            if (!InitializeOpenSSL()) {
                LOG("Failed to initialize OpenSSL");
                return FALSE;
            }
            
            // Load original schannel.dll for forwarding non-crypto calls
            GetOriginalSchannelDLL();
            break;
            
        case DLL_PROCESS_DETACH:
            LOG("schannel wrapper DLL unloaded");
            CleanupOpenSSL();
            
            if (g_hOriginalSchannel != NULL) {
                FreeLibrary(g_hOriginalSchannel);
                g_hOriginalSchannel = NULL;
            }
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    
    return TRUE;
}
