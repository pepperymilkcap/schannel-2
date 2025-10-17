# Installing the Schannel OpenSSL Wrapper

## ⚠️ IMPORTANT WARNINGS

- **This modifies system files.** Incorrect installation can break SSL/TLS functionality system-wide.
- **Create a system restore point** before proceeding.
- **Backup your original schannel.dll** before replacing it.
- **Test in a VM first** before deploying to production systems.
- **This is intended for Windows XP/2000 only** - modern Windows versions don't need this.

## Installation Methods

### Method 1: DLL Hijacking (Recommended)

This method doesn't require replacing system files and is safer.

#### For Internet Explorer

1. Copy `schannel.dll` to the same directory as `iexplore.exe`:
   ```
   C:\Program Files\Internet Explorer\schannel.dll
   ```

2. Windows will load the local DLL before the system one.

#### For Other Applications

Place `schannel.dll` in the application's directory (where the .exe is located).

**Advantages:**
- Safer - doesn't modify system files
- Per-application control
- Easy to revert

**Disadvantages:**
- Must be done for each application
- May not work for all programs

### Method 2: System-wide Replacement

This replaces the system schannel.dll globally.

#### Prerequisites

- Administrator access
- System restore point created
- Original schannel.dll backed up

#### Steps

1. **Boot into Safe Mode** (recommended):
   - Press F8 during boot
   - Select "Safe Mode with Command Prompt"

2. **Backup original schannel.dll:**
   ```cmd
   cd C:\Windows\System32
   copy schannel.dll schannel.dll.bak
   ```

3. **Rename original to schannel_orig.dll:**
   ```cmd
   ren schannel.dll schannel_orig.dll
   ```
   
   *Note: Our wrapper looks for schannel_orig.dll to forward non-crypto calls*

4. **Copy new schannel.dll:**
   ```cmd
   copy X:\path\to\built\schannel.dll C:\Windows\System32\
   ```

5. **Verify:**
   ```cmd
   dir schannel*
   ```
   
   You should see both:
   - `schannel.dll` (new wrapper)
   - `schannel_orig.dll` (original)

6. **Set permissions:**
   ```cmd
   icacls schannel.dll /grant Administrators:F System:F
   ```

7. **Reboot normally**

#### For Windows 2000

Windows 2000 may also need the DLL in `C:\WINNT\System32`:
```cmd
copy schannel.dll C:\WINNT\System32\
ren schannel.dll schannel_orig.dll
```

## OpenSSL DLL Distribution

The wrapper requires OpenSSL DLLs at runtime.

### Option 1: System Directory

Copy OpenSSL DLLs to System32:
```cmd
copy libeay32.dll C:\Windows\System32\
copy ssleay32.dll C:\Windows\System32\
```

Or for OpenSSL 1.0.2:
```cmd
copy libcrypto-1_1.dll C:\Windows\System32\
copy libssl-1_1.dll C:\Windows\System32\
```

### Option 2: Application Directory

For per-application deployment, copy OpenSSL DLLs alongside schannel.dll:
```
C:\Program Files\Internet Explorer\
├── iexplore.exe
├── schannel.dll        (wrapper)
├── libcrypto-1_1.dll   (OpenSSL)
└── libssl-1_1.dll      (OpenSSL)
```

## Verification

### Test with Internet Explorer

1. Open Internet Explorer
2. Navigate to: https://www.howsmyssl.com/
3. Check that connection succeeds
4. Should show TLS 1.2 support

### Test with Command Line

```cmd
rundll32 schannel.dll,SpGetInfo
```

No error = DLL loaded successfully.

### Check Event Viewer

1. Open Event Viewer (eventvwr.msc)
2. Check Application logs for any schannel errors

## Troubleshooting

### Error: "schannel.dll not found"

- Verify DLL is in correct location
- Check that System32 is in PATH

### Error: "The ordinal 42 could not be located"

- Missing exports - verify .def file
- Rebuild DLL with correct exports

### SSL connections fail

1. **Check OpenSSL DLLs:**
   ```cmd
   where libssl-1_1.dll
   where libcrypto-1_1.dll
   ```

2. **Check for schannel_orig.dll:**
   ```cmd
   dir C:\Windows\System32\schannel_orig.dll
   ```
   
   If missing, the wrapper can't forward calls.

3. **Enable debug logging:**
   - Rebuild with DEBUG flag
   - Check stderr output or use DebugView

### Application crashes

1. **Check DLL architecture:**
   - 32-bit apps need 32-bit DLL
   - Verify with dumpbin or objdump

2. **Check dependencies:**
   ```cmd
   dumpbin /DEPENDENTS schannel.dll
   ```

3. **Revert to original:**
   ```cmd
   cd C:\Windows\System32
   del schannel.dll
   ren schannel_orig.dll schannel.dll
   ```

### Specific Application Issues

**Internet Explorer:**
- Clear SSL state: Tools → Internet Options → Content → Clear SSL State
- Reset IE settings if needed

**Other browsers:**
- Firefox/Chrome use their own SSL libraries (NSS/BoringSSL)
- This wrapper only affects applications using Windows SSPI/Schannel

## Uninstallation

### From System Directory

1. Boot to Safe Mode
2. Delete wrapper:
   ```cmd
   cd C:\Windows\System32
   del schannel.dll
   ```
3. Restore original:
   ```cmd
   ren schannel_orig.dll schannel.dll
   ```
4. Reboot

### From Application Directory

Simply delete `schannel.dll` from the application folder.

## Security Considerations

- **Certificate Validation:** The wrapper validates certificates using OpenSSL's mechanisms
- **Cipher Suites:** Only strong ciphers are enabled by default
- **Protocol Versions:** SSL 2.0 and 3.0 are disabled
- **Updates:** Keep OpenSSL updated to latest 1.0.2 release for security patches

## Performance

- **Overhead:** Minimal - OpenSSL is highly optimized
- **Memory:** Slightly higher than native schannel due to BIO buffers
- **Compatibility:** Should work with any SSPI-based application

## Getting Help

If you encounter issues:

1. Check logs (if debug build)
2. Verify OpenSSL is properly installed
3. Test with simple application first (IE)
4. Check GitHub Issues for similar problems

## License

This wrapper is provided as-is. Use at your own risk. See LICENSE file for details.
