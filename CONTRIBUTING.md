# Contributing to Schannel OpenSSL Wrapper

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## Code of Conduct

- Be respectful and professional
- Focus on constructive feedback
- Help maintain a welcoming environment

## How to Contribute

### Reporting Issues

When reporting bugs or issues:

1. **Check existing issues** first
2. **Provide details:**
   - Windows version (XP/2000)
   - Application affected (IE, WinHTTP, etc.)
   - Error messages or logs
   - Steps to reproduce
3. **Include logs** if available (debug build output)

### Feature Requests

For feature requests:

1. Describe the use case
2. Explain why it's needed
3. Provide examples if possible

### Code Contributions

#### Setting Up Development Environment

1. **Fork the repository**
2. **Clone your fork:**
   ```bash
   git clone https://github.com/your-username/schannel-2.git
   ```
3. **Create a branch:**
   ```bash
   git checkout -b feature/your-feature-name
   ```

4. **Set up build environment:**
   - Install MinGW or Visual Studio
   - Install OpenSSL 1.0.2u
   - Verify build works: `make`

#### Code Style

Follow these conventions:

**C Code Style:**
```c
// Function names: Use Windows convention (PascalCase)
SECURITY_STATUS SEC_ENTRY MyFunction(void)

// Variables: Use camelCase or snake_case consistently
PSCHANNEL_CONTEXT pContext;
SSL_CTX *ssl_ctx;

// Indentation: 4 spaces (no tabs)
if (condition) {
    do_something();
}

// Comments: Explain WHY, not WHAT
// Need to handle continuation because SSL state machine requires it
if (ssl_error == SSL_ERROR_WANT_READ) {
    return SEC_I_CONTINUE_NEEDED;
}

// Error handling: Always check and log
if (!ptr) {
    LOG("Failed to allocate memory");
    return SEC_E_INSUFFICIENT_MEMORY;
}
```

**Function Organization:**
```c
// 1. Include headers
#include "../include/schannel_wrapper.h"

// 2. Forward declarations
SECURITY_STATUS MapError(int err);

// 3. Function implementations
SECURITY_STATUS SEC_ENTRY MyFunction(...) {
    // Local variables
    PSCHANNEL_CONTEXT pCtx = NULL;
    
    // Input validation
    if (!phContext) {
        return SEC_E_INVALID_HANDLE;
    }
    
    // Main logic
    // ...
    
    // Cleanup and return
    return SEC_E_OK;
}
```

#### Testing

Before submitting:

1. **Build without errors:**
   ```bash
   make clean
   make
   ```

2. **Test on Windows XP:**
   - Test with Internet Explorer
   - Test with a WinHTTP application
   - Verify no crashes or hangs

3. **Check for memory leaks:**
   - Use valgrind on Linux (for basic checks)
   - Test on Windows with task manager

4. **Verify exports:**
   ```bash
   objdump -p schannel.dll | grep "Export"
   ```

#### Documentation

- Update relevant .md files
- Add comments for complex logic
- Update USAGE.md with examples if adding features

#### Commit Messages

Use clear, descriptive commit messages:

```
Add support for TLS 1.3

- Implement TLS 1.3 handshake
- Update cipher suite selection
- Add TLS 1.3 specific extensions

Fixes #123
```

Format:
```
<type>: <subject>

<body>

<footer>
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `refactor`: Code refactoring
- `test`: Testing
- `build`: Build system changes

### Pull Request Process

1. **Update your branch:**
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Push to your fork:**
   ```bash
   git push origin feature/your-feature-name
   ```

3. **Create Pull Request:**
   - Clear title and description
   - Reference any related issues
   - Explain what changed and why

4. **PR Checklist:**
   - [ ] Code builds without errors
   - [ ] Tested on Windows XP
   - [ ] Documentation updated
   - [ ] Commit messages are clear
   - [ ] No unrelated changes included

5. **Review process:**
   - Address feedback promptly
   - Be open to suggestions
   - Update PR as needed

## Development Areas

### High Priority

- **Better error handling** - Improve error messages and recovery
- **Memory leak fixes** - Audit and fix any leaks
- **Performance optimization** - Profile and optimize hot paths
- **Certificate validation** - Better Windows cert store integration

### Medium Priority

- **Automated testing** - Unit tests and integration tests
- **Build improvements** - CMake support, easier builds
- **Logging** - Better debug output and tracing
- **Documentation** - More examples and troubleshooting

### Low Priority

- **TLS 1.3 support** - Requires OpenSSL 1.1.1 (XP incompatible?)
- **Additional SSPI functions** - Implement less-common functions
- **Alternative SSL libraries** - Support for other TLS implementations

## Architecture Guidelines

### Adding New Functions

1. **Check SSPI documentation** for function behavior
2. **Add to .def file** for exports
3. **Implement in appropriate .c file:**
   - Credentials → `sspi_wrapper.c`
   - Context → `context_wrapper.c`
   - Crypto → `crypto_wrapper.c`
   - Query → `query_wrapper.c`
4. **Add OpenSSL mapping** if needed in `openssl_helpers.c`

### File Organization

```
include/schannel_wrapper.h    - Shared definitions, structures
src/dllmain.c                 - DLL entry point, initialization
src/sspi_wrapper.c            - Credential management functions
src/context_wrapper.c         - Context initialization/deletion
src/crypto_wrapper.c          - Encryption/decryption
src/query_wrapper.c           - Query functions
src/openssl_helpers.c         - OpenSSL utility functions
src/spusermode.c              - LSA integration functions
```

### Error Handling

Always map OpenSSL errors to SECURITY_STATUS:

```c
SECURITY_STATUS MapOpenSSLError(void) {
    unsigned long err = ERR_get_error();
    int reason = ERR_GET_REASON(err);
    
    switch (reason) {
        case SSL_R_CERTIFICATE_VERIFY_FAILED:
            return SEC_E_CERT_EXPIRED;
        // ... more mappings
        default:
            return SEC_E_INTERNAL_ERROR;
    }
}
```

### Logging

Use LOG macro consistently:

```c
LOG("InitializeSecurityContext called, target: %s", targetName);
LOG("Handshake complete, cipher: %s", cipher_name);
LOG("Error: Failed to allocate context");
```

## Resources

### Documentation

- [SSPI Reference](https://docs.microsoft.com/en-us/windows/win32/secauthn/sspi-functions)
- [Schannel Reference](https://docs.microsoft.com/en-us/windows/win32/secauthn/schannel)
- [OpenSSL 1.0.2 Docs](https://www.openssl.org/docs/man1.0.2/)

### Tools

- **dumpbin** (MSVC) - Inspect DLL exports and dependencies
- **objdump** (MinGW) - Same as dumpbin for MinGW
- **Dependency Walker** - View DLL dependencies
- **Process Explorer** - Monitor loaded DLLs
- **DebugView** - Capture debug output

### Testing

- **Virtual Machine** - Windows XP VM for testing
- **howsmyssl.com** - Test TLS version support
- **badssl.com** - Test various SSL scenarios

## Questions?

- Open an issue for questions
- Check existing documentation
- Review code comments

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (MIT License).

Thank you for contributing!
