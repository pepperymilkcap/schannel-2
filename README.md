# Schannel OpenSSL Wrapper for Windows XP/2000

A DLL wrapper that redirects Windows Schannel (SSPI) SSL/TLS calls to OpenSSL, enabling modern TLS 1.2+ connections on legacy Windows XP/2000 systems.

## 🎯 Project Overview

Windows XP and Windows 2000 only support SSL/TLS up to TLS 1.0, making them unable to connect to modern websites that require TLS 1.2 or higher. This project provides a drop-in replacement for `schannel.dll` that uses OpenSSL to handle SSL/TLS operations, enabling:

- **TLS 1.2 support** on Windows XP/2000
- **Modern cipher suites** (AES-256, SHA-256, etc.)
- **Compatibility** with applications using Windows SSPI (Internet Explorer, WinHTTP, etc.)
- **Security updates** through OpenSSL rather than discontinued Windows updates

## 🏗️ Architecture

The wrapper DLL implements the Windows SSPI (Security Support Provider Interface) API:

```
Application (IE, etc.)
       ↓
  SSPI Functions
       ↓
schannel.dll (wrapper) ──→ OpenSSL (TLS 1.2+)
       ↓
schannel_orig.dll (for non-crypto calls)
```

### Key Components

1. **SSPI Implementation** (`sspi_wrapper.c`)
   - `AcquireCredentialsHandle` - Initialize credentials
   - `InitializeSecurityContext` - Client handshake
   - `AcceptSecurityContext` - Server handshake

2. **Cryptographic Operations** (`crypto_wrapper.c`)
   - `EncryptMessage` / `DecryptMessage` - Data encryption
   - Uses OpenSSL's BIO pairs for non-blocking I/O

3. **Context Management** (`context_wrapper.c`)
   - SSL context lifecycle
   - Handshake state machine

4. **Forwarding Layer** (`dllmain.c`)
   - Loads original `schannel_orig.dll`
   - Forwards non-SSL/TLS calls

## 🚀 Quick Start

### Prerequisites

- Windows XP/2000 target system
- OpenSSL 1.0.2u (last version supporting XP)
- MinGW or Visual Studio 2015+ (with XP toolset)

### Building

**With MinGW:**
```bash
make
```

**With Visual Studio:**
```cmd
Open schannel.sln
Build → Build Solution
```

See [BUILD.md](BUILD.md) for detailed build instructions.

### Installation

**Quick test (per-application):**
```cmd
copy schannel.dll "C:\Program Files\Internet Explorer\"
```

**System-wide (requires admin):**
```cmd
cd C:\Windows\System32
ren schannel.dll schannel_orig.dll
copy X:\path\to\schannel.dll .
```

⚠️ **See [INSTALL.md](INSTALL.md) for complete installation guide and safety precautions.**

## 📋 Features

- ✅ **TLS 1.0, 1.1, 1.2 support** via OpenSSL
- ✅ **Modern cipher suites** (AES-GCM, ChaCha20-Poly1305)
- ✅ **Certificate validation** using OpenSSL trust store
- ✅ **SNI (Server Name Indication)** support
- ✅ **Non-blocking I/O** using BIO pairs
- ✅ **Compatible with existing applications** (no source changes needed)
- ✅ **Fallback to original DLL** for non-crypto operations

## 🧪 Testing

Test with Internet Explorer:
1. Open IE on Windows XP
2. Navigate to https://www.howsmyssl.com/
3. Should report TLS 1.2 capability

## 📁 Project Structure

```
schannel-2/
├── include/
│   └── schannel_wrapper.h    # Main header file
├── src/
│   ├── dllmain.c              # DLL entry point
│   ├── sspi_wrapper.c         # SSPI credential functions
│   ├── context_wrapper.c      # Context management
│   ├── crypto_wrapper.c       # Encrypt/decrypt functions
│   ├── query_wrapper.c        # Query attributes
│   ├── openssl_helpers.c      # OpenSSL utilities
│   ├── spusermode.c           # SpUserMode functions
│   └── schannel.def           # Export definitions
├── Makefile                   # MinGW build
├── schannel.sln               # Visual Studio solution
├── schannel.vcxproj           # VS project file
├── BUILD.md                   # Build instructions
├── INSTALL.md                 # Installation guide
└── README.md                  # This file
```

## 🔧 Configuration

The wrapper can be configured by editing `include/schannel_wrapper.h`:

- **Enable debug logging:** Define `DEBUG` during compilation
- **Adjust cipher suites:** Modify `CreateSSLContext()` in `openssl_helpers.c`
- **Certificate store:** Configure in OpenSSL context creation

## ⚠️ Limitations

- **32-bit only** - Windows XP x64 requires 64-bit build
- **OpenSSL 1.0.2 max** - Newer versions don't support XP
- **No TLS 1.3** - OpenSSL 1.0.2 doesn't support it
- **Application-specific issues** - Some apps may have hardcoded checks
- **Performance overhead** - Minimal, but BIO buffering adds some latency

## 🛡️ Security

- **Strong ciphers only** - Weak ciphers (RC4, MD5) disabled
- **SSL 2.0/3.0 disabled** - Only TLS 1.0+ enabled
- **Certificate validation** - Full chain validation via OpenSSL
- **Security updates** - Update OpenSSL to patch vulnerabilities

**Note:** Windows XP itself is unsupported and insecure. This project only addresses TLS/SSL limitations, not other security issues.

## 🤝 Contributing

Contributions welcome! Areas for improvement:

- Better error handling and logging
- Performance optimization
- Additional SSPI function coverage
- Automated testing
- Certificate store integration

## 📄 License

This project is provided as-is for educational and compatibility purposes. See LICENSE file for details.

## 🙏 Acknowledgments

- OpenSSL Project for the SSL/TLS library
- Microsoft for SSPI/Schannel documentation
- Community contributors

## 📞 Support

For issues, questions, or contributions, please use GitHub Issues.

**Disclaimer:** This is a compatibility solution for legacy systems. Modern, supported operating systems should be used whenever possible.