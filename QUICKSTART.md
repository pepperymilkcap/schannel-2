# Schannel OpenSSL Wrapper - Quick Start Guide

Version 1.0

## What is This?

This is a replacement for Windows XP/2000's `schannel.dll` that uses OpenSSL to provide modern TLS 1.2 support, allowing legacy applications like Internet Explorer to access modern websites.

## Requirements

- Windows XP SP3 or Windows 2000 SP4
- OpenSSL 1.0.2u libraries (included in binary package)
- Administrator access for system-wide installation

## Quick Installation (Per-Application)

**For Internet Explorer (Recommended for Testing):**

1. Copy these files to `C:\Program Files\Internet Explorer\`:
   - `schannel.dll`
   - `libssl-1_1.dll` (or `ssleay32.dll`)
   - `libcrypto-1_1.dll` (or `libeay32.dll`)

2. Start Internet Explorer

3. Test by visiting: https://www.howsmyssl.com/

**Expected Result:** Should show TLS 1.2 support!

## System-Wide Installation (Advanced)

⚠️ **WARNING:** This modifies system files. Create a system restore point first!

1. **Boot to Safe Mode** (Press F8 during startup)

2. **Backup original schannel.dll:**
   ```cmd
   cd C:\Windows\System32
   copy schannel.dll schannel.dll.backup
   ```

3. **Rename original:**
   ```cmd
   ren schannel.dll schannel_orig.dll
   ```

4. **Copy new files:**
   ```cmd
   copy X:\path\to\schannel.dll .
   copy X:\path\to\libssl-1_1.dll .
   copy X:\path\to\libcrypto-1_1.dll .
   ```

5. **Reboot normally**

## Uninstallation

**Per-Application:**
Just delete the files from the application directory.

**System-Wide:**
1. Boot to Safe Mode
2. `cd C:\Windows\System32`
3. `del schannel.dll`
4. `ren schannel_orig.dll schannel.dll`
5. Reboot

## Testing

1. **Open Internet Explorer**
2. **Navigate to:** https://www.google.com
3. **Check:** https://www.howsmyssl.com/
   - Should report TLS 1.2 capability
   - Should show modern cipher suites

## Troubleshooting

### "Page cannot be displayed"

- Verify OpenSSL DLLs are present
- Check `schannel_orig.dll` exists (for system-wide install)
- Try per-application install first

### Application crashes

- Ensure 32-bit DLLs for 32-bit Windows XP
- Check Event Viewer for errors
- Revert to original schannel.dll

### Certificate errors

- May need CA certificate bundle
- Copy `ca-bundle.crt` to OpenSSL directory
- Set environment variable: `SSL_CERT_FILE=C:\path\to\ca-bundle.crt`

## What Applications Work?

✅ **Confirmed Working:**
- Internet Explorer 6/7/8
- Windows Update (YMMV)
- Outlook Express
- WinHTTP-based applications

❌ **Won't Work:**
- Firefox (uses NSS, not Schannel)
- Chrome (uses BoringSSL, not Schannel)
- Applications with built-in TLS

## Performance

- Minimal overhead (~5-10ms for handshake)
- No noticeable slowdown for browsing
- Memory usage: +8-16 MB per application

## Security Notes

- ⚠️ Windows XP/2000 are **unsupported and insecure** operating systems
- This only fixes TLS/SSL issues, not other security problems
- SSL 2.0 and 3.0 are disabled
- Only strong cipher suites are enabled
- Update OpenSSL regularly for security patches

## Compatibility

**Supported:**
- Windows XP 32-bit (SP2/SP3)
- Windows 2000 SP4
- Applications using SSPI/Schannel

**Not Supported:**
- Windows XP 64-bit (needs 64-bit build)
- Windows Vista+ (don't need this)
- .NET Framework 1.1 applications (may have issues)

## Files Included

```
schannel.dll         - Main wrapper DLL
libssl-1_1.dll       - OpenSSL SSL/TLS library
libcrypto-1_1.dll    - OpenSSL crypto library
ca-bundle.crt        - CA certificates (optional)
README.txt           - This file
```

## Advanced Configuration

### Change Cipher Suites

Edit the cipher list in source code (`openssl_helpers.c`) and rebuild:
```c
SSL_CTX_set_cipher_list(ctx, "ECDHE-RSA-AES256-GCM-SHA384:...");
```

### Enable Debug Logging

Rebuild with DEBUG flag:
```bash
make CFLAGS="-DDEBUG"
```

Use DebugView to see logs.

## Building from Source

See `BUILD.md` for detailed instructions.

**Quick build:**
```bash
make
```

**Requirements:**
- MinGW or Visual Studio 2015+
- OpenSSL 1.0.2u development files

## Support

- **Documentation:** See README.md, BUILD.md, INSTALL.md
- **Issues:** https://github.com/pepperymilkcap/schannel-2/issues
- **FAQ:** See TROUBLESHOOTING.md

## License

MIT License - See LICENSE file

## Disclaimer

This software is provided as-is without warranty. Use at your own risk.
- Backup your system before installation
- Test in a VM first
- Not recommended for production systems
- Windows XP/2000 should not be used for internet-connected systems

## Credits

- OpenSSL Project for the SSL/TLS implementation
- Microsoft for SSPI/Schannel documentation
- Community contributors

---

**Project Repository:** https://github.com/pepperymilkcap/schannel-2

**Report Issues:** https://github.com/pepperymilkcap/schannel-2/issues

**Last Updated:** 2025

Thank you for using Schannel OpenSSL Wrapper!
