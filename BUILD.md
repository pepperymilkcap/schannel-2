# Building the Schannel OpenSSL Wrapper

This document provides detailed instructions for building the schannel.dll wrapper for Windows XP/2000.

## Prerequisites

### Required Software

1. **Compiler Options:**
   - **Option A: MinGW** (Recommended for XP compatibility)
     - MinGW-w64 with GCC 4.8 or later
     - Make utility
   - **Option B: Microsoft Visual Studio**
     - Visual Studio 2015 or later with Windows XP toolset (v140_xp)
     - Windows SDK

2. **OpenSSL**
   - OpenSSL 1.0.2u (last version supporting Windows XP)
   - OpenSSL development libraries and headers
   - Download from: https://www.openssl.org/source/old/1.0.2/

### OpenSSL Installation

#### Building OpenSSL for Windows XP

1. Download OpenSSL 1.0.2u source
2. Extract to a directory (e.g., `C:\openssl-1.0.2u`)
3. Open a MinGW or MSVC command prompt
4. Configure for Windows:

   **For MinGW:**
   ```bash
   perl Configure mingw --prefix=/usr/local
   make
   make install
   ```

   **For MSVC:**
   ```cmd
   perl Configure VC-WIN32 --prefix=C:\OpenSSL
   ms\do_ms
   nmake -f ms\ntdll.mak
   nmake -f ms\ntdll.mak install
   ```

## Building with MinGW

1. **Set up environment:**
   ```bash
   export PATH=/mingw/bin:$PATH
   ```

2. **Configure OpenSSL paths** (if not in default location):
   Edit `Makefile` and update:
   ```makefile
   CFLAGS += -I/path/to/openssl/include
   LDFLAGS += -L/path/to/openssl/lib
   ```

3. **Build:**
   ```bash
   make clean
   make
   ```

4. **Output:**
   - `schannel.dll` - The wrapper DLL

## Building with Visual Studio

1. **Set OpenSSL path:**
   - Set environment variable: `OPENSSL_DIR=C:\OpenSSL`
   - Or edit `schannel.vcxproj` and update OpenSSL paths

2. **Open solution:**
   ```
   schannel.sln
   ```

3. **Select configuration:**
   - Configuration: Release
   - Platform: Win32

4. **Build:**
   - Build → Build Solution (Ctrl+Shift+B)

5. **Output:**
   - `bin\Release\schannel.dll`

## Build Verification

After building, verify the DLL:

1. **Check exports:**
   ```bash
   dumpbin /EXPORTS schannel.dll     # MSVC
   objdump -p schannel.dll           # MinGW
   ```

2. **Check dependencies:**
   ```bash
   dumpbin /DEPENDENTS schannel.dll  # MSVC
   objdump -x schannel.dll           # MinGW
   ```

3. **Expected exports include:**
   - InitializeSecurityContextA/W
   - AcceptSecurityContext
   - AcquireCredentialsHandleA/W
   - FreeCredentialsHandle
   - EncryptMessage
   - DecryptMessage
   - And others...

## Troubleshooting

### Common Build Errors

1. **OpenSSL headers not found:**
   - Verify OpenSSL is installed
   - Check include paths in Makefile or project settings

2. **Linking errors with OpenSSL:**
   - Ensure OpenSSL libraries (libssl, libcrypto) are built
   - Verify library paths in linker settings

3. **Windows SDK headers missing:**
   - Install Windows SDK
   - Ensure schannel.h and sspi.h are available

4. **Target platform errors:**
   - Verify _WIN32_WINNT=0x0501 is defined
   - For MSVC, ensure v140_xp toolset is installed

### MinGW-specific Issues

- **Missing stdcall decorations:**
  - Use: `-Wl,--enable-stdcall-fixup`
  
- **DLL export errors:**
  - Verify schannel.def is correct
  - Check for typos in function names

### MSVC-specific Issues

- **XP toolset not available:**
  - Install "Windows XP Support for C++" in Visual Studio installer
  
- **LNK2001 unresolved external:**
  - Check OpenSSL library names (libssl.lib vs ssleay32.lib)
  - Verify library paths

## Cross-compilation

To build on Linux for Windows:

1. **Install MinGW cross-compiler:**
   ```bash
   sudo apt-get install mingw-w64
   ```

2. **Build OpenSSL for Windows:**
   ```bash
   ./Configure mingw64 --cross-compile-prefix=x86_64-w64-mingw32-
   make
   ```

3. **Update Makefile:**
   ```makefile
   CC = x86_64-w64-mingw32-gcc
   ```

4. **Build:**
   ```bash
   make
   ```

## Debug Build

To build with debug symbols and logging:

1. **MinGW:**
   ```bash
   make CFLAGS="-g -DDEBUG" clean all
   ```

2. **MSVC:**
   - Select "Debug" configuration
   - Build solution

Debug builds will output detailed logs to stderr.

## Next Steps

After building successfully, proceed to [INSTALL.md](INSTALL.md) for installation instructions.
