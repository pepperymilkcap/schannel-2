# Implementation Details

## Overview

This document describes the technical implementation of the Schannel OpenSSL wrapper.

## Architecture Layers

### Layer 1: DLL Entry Point (dllmain.c)

**Responsibilities:**
- Initialize/cleanup OpenSSL library
- Load original schannel.dll (schannel_orig.dll) for forwarding
- Manage DLL lifecycle

**Key Functions:**
- `DllMain()` - Entry point called by Windows
- `InitializeOpenSSL()` - Initialize OpenSSL library
- `CleanupOpenSSL()` - Cleanup OpenSSL resources
- `GetOriginalSchannelDLL()` - Load original DLL for forwarding

### Layer 2: SSPI Functions (sspi_wrapper.c)

**Responsibilities:**
- Implement credential management
- Handle SSPI function exports that aren't context-specific

**Key Functions:**
- `AcquireCredentialsHandleA/W()` - Create SSL_CTX and wrap in our structure
- `FreeCredentialsHandle()` - Free SSL_CTX
- `FreeContextBuffer()` - Free memory allocated by SSPI

**Data Structures:**
```c
typedef struct _SCHANNEL_CRED {
    CredHandle original_cred;  // For forwarding if needed
    SSL_CTX *ssl_ctx;          // OpenSSL context
    BOOL is_server;            // Client or server mode
} SCHANNEL_CRED;
```

### Layer 3: Context Management (context_wrapper.c)

**Responsibilities:**
- TLS handshake management
- Context initialization and deletion
- Map between SSPI and OpenSSL state machines

**Key Functions:**
- `InitializeSecurityContextA/W()` - Client TLS handshake
- `AcceptSecurityContext()` - Server TLS handshake
- `DeleteSecurityContext()` - Cleanup SSL connection

**Data Structures:**
```c
typedef struct _SCHANNEL_CONTEXT {
    SSL_CTX *ssl_ctx;           // OpenSSL context (reference)
    SSL *ssl;                   // OpenSSL SSL object
    BIO *rbio;                  // Read BIO (network -> SSL)
    BIO *wbio;                  // Write BIO (SSL -> network)
    BOOL is_server;             // Client or server
    BOOL handshake_complete;    // Handshake state
} SCHANNEL_CONTEXT;
```

**Handshake Flow:**

1. **First call to InitializeSecurityContext:**
   ```
   Application → InitializeSecurityContext(creds, NULL, ...)
   ↓
   Create SCHANNEL_CONTEXT
   Create SSL object from SSL_CTX
   Create BIO pair for non-blocking I/O
   Set client mode: SSL_set_connect_state()
   Set SNI: SSL_set_tlsext_host_name()
   Call SSL_do_handshake()
   Read output from wbio → return to application
   Return SEC_I_CONTINUE_NEEDED
   ```

2. **Continuation calls:**
   ```
   Application → InitializeSecurityContext(creds, &context, ...)
   ↓
   Write input data to rbio
   Call SSL_do_handshake()
   Read output from wbio → return to application
   Check SSL state:
     - If complete → return SEC_E_OK
     - If need more data → return SEC_I_CONTINUE_NEEDED
     - If error → return SEC_E_*
   ```

### Layer 4: Cryptographic Operations (crypto_wrapper.c)

**Responsibilities:**
- Encrypt application data
- Decrypt application data
- Handle record-layer operations

**Key Functions:**
- `EncryptMessage()` - Encrypt plaintext to TLS records
- `DecryptMessage()` - Decrypt TLS records to plaintext
- `SealMessage()` / `UnsealMessage()` - Aliases
- `MakeSignature()` / `VerifySignature()` - For message authentication

**Encryption Flow:**
```
Application provides plaintext in SECBUFFER_DATA
↓
Write plaintext to SSL: SSL_write()
↓
Read TLS record from wbio: BIO_read()
↓
Return TLS record to application in SECBUFFER_TOKEN
```

**Decryption Flow:**
```
Application provides TLS record in SECBUFFER_DATA
↓
Write TLS record to rbio: BIO_write()
↓
Read plaintext from SSL: SSL_read()
↓
Return plaintext to application in SECBUFFER_DATA
```

### Layer 5: Query Functions (query_wrapper.c)

**Responsibilities:**
- Provide context and package information
- Implement SSPI query functions

**Key Functions:**
- `QueryContextAttributesA/W()` - Get context properties
- `EnumerateSecurityPackagesA/W()` - List available packages
- `QuerySecurityPackageInfoA/W()` - Get package info

**Supported Queries:**
- `SECPKG_ATTR_STREAM_SIZES` - TLS record sizes
- `SECPKG_ATTR_REMOTE_CERT_CONTEXT` - Server certificate
- `SECPKG_ATTR_CONNECTION_INFO` - Protocol version, cipher
- `SECPKG_ATTR_CIPHER_INFO` - Detailed cipher info

### Layer 6: OpenSSL Helpers (openssl_helpers.c)

**Responsibilities:**
- OpenSSL initialization and configuration
- Error mapping
- Algorithm mapping

**Key Functions:**
- `CreateSSLContext()` - Create configured SSL_CTX
- `MapSchannelAlgsToOpenSSL()` - Map Windows ALG_ID to OpenSSL NID
- `MapOpenSSLErrorToSecStatus()` - Map OpenSSL errors to SECURITY_STATUS

**SSL Context Configuration:**
```c
SSL_CTX* CreateSSLContext(BOOL is_server) {
    // Select method (client or server)
    method = is_server ? SSLv23_server_method() : SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    
    // Disable weak protocols
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    
    // Set strong cipher suites
    SSL_CTX_set_cipher_list(ctx, "HIGH:!aNULL:!MD5:!RC4");
    
    // Other security options
    SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION);
    
    return ctx;
}
```

### Layer 7: SpUserMode Functions (spusermode.c)

**Responsibilities:**
- Implement LSA (Local Security Authority) integration functions
- Provide package initialization for security subsystem

**Key Functions:**
- `SpUserModeInitialize()` - Initialize security package
- `SpInstanceInit()` - Initialize package instance
- `SpGetInfo()` - Return package information

Most of these forward to original DLL if available, or provide minimal implementations.

## Handle Management

We use "magic values" to identify our handles:

```c
// Credential handle
phCredential->dwLower = (ULONG_PTR)our_cred_structure;
phCredential->dwUpper = 0xDEADBEEF;  // Magic value

// Context handle
phContext->dwLower = (ULONG_PTR)our_context_structure;
phContext->dwUpper = 0xBEEFDEAD;  // Different magic value
```

This allows us to:
1. Identify if handle is ours or original DLL's
2. Retrieve our internal structure from handle
3. Detect invalid handles

## BIO Pair Usage

We use OpenSSL BIO pairs for non-blocking operation:

```
┌─────────────┐
│ Application │
└──────┬──────┘
       │ Encrypted data
       ↓
┌─────────────┐     ┌─────┐     ┌──────────┐
│    wbio     │ ←── │ SSL │ ──→ │   rbio   │
│  (output)   │     └─────┘     │  (input) │
└─────────────┘                 └──────────┘
       │                              ↑
       │ BIO_read()                   │ BIO_write()
       ↓                              │
   To network                    From network
```

**Benefits:**
- Non-blocking I/O
- Matches SSPI's multi-step handshake model
- No need for socket management in DLL

## Error Handling Strategy

### OpenSSL to SECURITY_STATUS Mapping

```c
OpenSSL Error                    → SECURITY_STATUS
────────────────────────────────────────────────────
SSL_ERROR_WANT_READ              → SEC_I_CONTINUE_NEEDED
SSL_ERROR_WANT_WRITE             → SEC_I_CONTINUE_NEEDED
SSL_R_CERTIFICATE_VERIFY_FAILED  → SEC_E_CERT_EXPIRED
SSL_R_UNKNOWN_PROTOCOL           → SEC_E_UNSUPPORTED_FUNCTION
SSL_R_DECRYPTION_FAILED          → SEC_E_DECRYPT_FAILURE
SSL_R_BAD_SIGNATURE              → SEC_E_MESSAGE_ALTERED
(other errors)                   → SEC_E_INTERNAL_ERROR
```

### Logging

Debug builds include logging via `LOG()` macro:

```c
#ifdef DEBUG
#define LOG(fmt, ...) fprintf(stderr, "[schannel-wrapper] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...) ((void)0)
#endif
```

## Threading Considerations

### Thread Safety

- Each context (SSL object) is independent
- SSL_CTX can be shared (OpenSSL is thread-safe for reads)
- No global state except initialization flag

### Synchronization

Currently no explicit locking. Relies on:
1. SSPI calling convention (caller manages synchronization)
2. OpenSSL's thread safety for SSL_CTX
3. Independent context objects

**Potential Issue:** If application calls functions on same context from multiple threads, undefined behavior may occur.

**Future Enhancement:** Add critical sections around context operations.

## Memory Management

### Allocation Strategy

```c
// Credentials - allocated in AcquireCredentialsHandle
PSCHANNEL_CRED pCred = malloc(sizeof(SCHANNEL_CRED));

// Contexts - allocated in InitializeSecurityContext
PSCHANNEL_CONTEXT pCtx = malloc(sizeof(SCHANNEL_CONTEXT));

// Buffers - caller-allocated (SSPI convention)
// We only read/write to caller's buffers
```

### Cleanup

```c
// Credentials
FreeCredentialsHandle():
  SSL_CTX_free(pCred->ssl_ctx);
  free(pCred);

// Contexts  
DeleteSecurityContext():
  SSL_free(pCtx->ssl);  // Also frees BIOs
  free(pCtx);
```

### Memory Leaks

**Potential Leaks:**
1. Application doesn't call cleanup functions
2. Error paths that don't free allocated memory
3. OpenSSL internal leaks (rare)

**Mitigation:**
- Always check return values and free on error
- Document cleanup requirements
- Test with memory leak detectors

## Performance Optimizations

### Current Implementation

- No caching (creates new SSL object per connection)
- No session resumption
- Minimal buffering

### Potential Improvements

1. **Session Caching:**
   ```c
   SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_CLIENT);
   SSL_CTX_sess_set_cache_size(ctx, 128);
   ```

2. **Larger Buffers:**
   - Reduce BIO_read/write calls
   - Batch small messages

3. **Context Pooling:**
   - Reuse SSL objects for new connections
   - Requires refactoring context management

## Security Considerations

### Protocol Versions

**Enabled:** TLS 1.0, 1.1, 1.2
**Disabled:** SSL 2.0, SSL 3.0

Reasoning: XP apps may need TLS 1.0, but we want to support modern sites requiring 1.2.

### Cipher Suites

Default list: `"HIGH:!aNULL:!MD5:!RC4"`

**Allows:**
- AES (128/256)
- 3DES
- SHA-256/384
- ECDHE/DHE key exchange

**Blocks:**
- Anonymous auth (aNULL)
- MD5 hashing
- RC4 cipher

### Certificate Validation

Uses OpenSSL's validation:
- Chain verification
- Expiration check
- Hostname verification (via SNI)

**Note:** May need to configure CA certificate bundle location.

### Known Security Limitations

1. **Windows XP itself is insecure** - This only fixes TLS
2. **No protection against memory dumping** - Keys in process memory
3. **No additional hardening** - Beyond OpenSSL defaults

## Compatibility Notes

### SSPI Conventions

We follow SSPI conventions:
- Multi-step handshake (SEC_I_CONTINUE_NEEDED)
- Caller-allocated buffers
- Magic values for invalid handles

### Application Assumptions

Most apps assume:
- Schannel DLL is in System32
- Exports match Windows schannel.dll
- Return values match documented behavior

### Edge Cases

**Renegotiation:**
- Not currently supported
- Would need additional state management

**Client Certificates:**
- Partial support
- Would need credential callback

**Alert Handling:**
- Basic support via OpenSSL
- Could improve error reporting

## Testing Strategy

### Unit Testing

Currently no automated tests. Would benefit from:
- Mock BIO testing
- State machine verification
- Error path coverage

### Integration Testing

Manual testing with:
- Internet Explorer
- WinHTTP test programs
- Various HTTPS sites

### Regression Testing

Check:
- Basic connectivity
- Certificate validation
- Various cipher suites
- Protocol versions

## Build System

### Makefile (MinGW)

- Simple, portable
- Targets Windows XP
- Links against OpenSSL 1.0.2

### Visual Studio

- Project file for GUI builds
- Supports debugging
- XP toolset required

### Cross-compilation

Can build on Linux with MinGW cross-compiler.

## Future Enhancements

### High Priority

1. Add proper thread synchronization
2. Implement session caching
3. Better error messages and logging
4. Memory leak auditing

### Medium Priority

1. Client certificate support
2. Renegotiation handling
3. Performance profiling and optimization
4. Automated testing

### Low Priority

1. TLS 1.3 support (requires newer OpenSSL)
2. Alternative backends (mbedTLS, etc.)
3. Configuration file support
4. Telemetry/diagnostics

## References

- [SSPI Reference](https://docs.microsoft.com/en-us/windows/win32/secauthn/sspi)
- [Schannel Reference](https://docs.microsoft.com/en-us/windows/win32/secauthn/schannel)
- [OpenSSL 1.0.2 Documentation](https://www.openssl.org/docs/man1.0.2/)
- [BIO Pairs](https://www.openssl.org/docs/man1.0.2/man3/BIO_s_bio.html)

## Contributing

See CONTRIBUTING.md for guidelines on contributing to this implementation.
