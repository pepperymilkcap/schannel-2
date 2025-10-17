# Troubleshooting Guide

This guide helps diagnose and fix common issues with the Schannel OpenSSL wrapper.

## Quick Diagnostics

### Check if Wrapper is Loaded

**Method 1: Process Explorer**
1. Download Process Explorer from Sysinternals
2. Run the application (e.g., Internet Explorer)
3. Find the process in Process Explorer
4. View → Lower Pane View → DLLs
5. Look for:
   - `schannel.dll` (should be your wrapper)
   - `libssl-1_1.dll` or `ssleay32.dll`
   - `libcrypto-1_1.dll` or `libeay32.dll`

**Method 2: Command Line**
```cmd
tasklist /m schannel.dll
```

### Verify DLL Exports

```cmd
dumpbin /EXPORTS C:\Windows\System32\schannel.dll
```

Should show functions like:
- `InitializeSecurityContextA`
- `AcceptSecurityContext`
- `EncryptMessage`
- etc.

### Check OpenSSL Version

```cmd
openssl version
```

Should be 1.0.2u (or compatible version).

## Common Issues and Solutions

### Issue 1: Application Won't Start

**Symptoms:**
- Application crashes immediately
- Error: "The application failed to initialize properly"
- Missing DLL error

**Solutions:**

1. **Check for missing OpenSSL DLLs:**
   ```cmd
   where libssl-1_1.dll
   where libcrypto-1_1.dll
   ```
   
   If not found, copy them to:
   - Same directory as application
   - Or C:\Windows\System32

2. **Check DLL architecture:**
   ```cmd
   dumpbin /HEADERS schannel.dll | find "machine"
   ```
   
   Should show `(x86)` for 32-bit Windows XP.

3. **Verify all dependencies:**
   ```cmd
   dumpbin /DEPENDENTS schannel.dll
   ```
   
   All listed DLLs must be present.

### Issue 2: SSL/TLS Connection Fails

**Symptoms:**
- "Page cannot be displayed" in IE
- "The certificate is invalid" error
- Connection timeouts

**Diagnosis:**

1. **Test with simple site:**
   ```
   http://www.google.com  (works? If no, problem isn't SSL)
   https://www.google.com (fails? SSL issue confirmed)
   ```

2. **Check if schannel_orig.dll exists:**
   ```cmd
   dir C:\Windows\System32\schannel_orig.dll
   ```
   
   If missing, wrapper can't forward calls.

3. **Test OpenSSL directly:**
   ```cmd
   openssl s_client -connect www.google.com:443 -tls1_2
   ```
   
   If this fails, OpenSSL installation is broken.

**Solutions:**

1. **Certificate validation issue:**
   - OpenSSL needs CA certificates
   - Copy `ca-bundle.crt` to OpenSSL directory
   - Or set environment: `set SSL_CERT_FILE=C:\path\to\ca-bundle.crt`

2. **Cipher suite mismatch:**
   - Rebuild wrapper with different cipher list
   - Edit `src/openssl_helpers.c`:
     ```c
     SSL_CTX_set_cipher_list(ctx, "DEFAULT:!aNULL:!eNULL");
     ```

3. **Protocol version issue:**
   - Some servers require TLS 1.2 minimum
   - Verify OpenSSL supports it:
     ```cmd
     openssl s_client -tls1_2 -connect site.com:443
     ```

### Issue 3: Application Hangs

**Symptoms:**
- Application freezes during connection
- No response, must force close
- 100% CPU usage

**Diagnosis:**

Enable debug logging:
```cmd
set DEBUG=1
iexplore.exe 2> debug.log
```

Check `debug.log` for:
- Last function called before hang
- Repeated calls to same function (infinite loop)

**Solutions:**

1. **Handshake deadlock:**
   - Check BIO buffer handling
   - Ensure `SEC_I_CONTINUE_NEEDED` is returned properly
   - Review `context_wrapper.c` handshake logic

2. **Blocking operation:**
   - OpenSSL should use non-blocking BIOs
   - Verify in `context_wrapper.c`:
     ```c
     BIO_set_nbio(rbio, 1);
     BIO_set_nbio(wbio, 1);
     ```

3. **Memory exhaustion:**
   - Check for memory leaks
   - Monitor with Task Manager
   - Review cleanup in `DeleteSecurityContext`

### Issue 4: Intermittent Failures

**Symptoms:**
- Sometimes works, sometimes fails
- Random disconnections
- "Connection reset" errors

**Diagnosis:**

1. **Check Event Viewer:**
   ```cmd
   eventvwr.msc
   ```
   
   Look in Application logs for errors.

2. **Network capture:**
   - Use Wireshark
   - Capture traffic on port 443
   - Look for TLS alerts or RST packets

**Solutions:**

1. **Race condition:**
   - Add thread synchronization if needed
   - Check if multiple threads access same context

2. **Buffer overflow:**
   - Verify buffer sizes in `crypto_wrapper.c`
   - Check `SECBUFFER` handling

3. **Memory corruption:**
   - Run with Application Verifier
   - Check for use-after-free

### Issue 5: Specific Websites Don't Work

**Symptoms:**
- Some HTTPS sites work fine
- Others fail with certificate errors
- Pattern: specific server configurations

**Diagnosis:**

Test the site's SSL configuration:
```cmd
openssl s_client -connect problem-site.com:443 -showcerts
```

Check:
- TLS version required
- Cipher suites offered
- Certificate chain

**Solutions:**

1. **SNI required:**
   - Verify SNI is set in `InitializeSecurityContext`
   - Check `SSL_set_tlsext_host_name()` call

2. **Client certificate required:**
   - Site needs client authentication
   - Implement certificate callback
   - Load client cert in `AcquireCredentialsHandle`

3. **Specific cipher needed:**
   - Add cipher to allowed list
   - Edit `CreateSSLContext()` in `openssl_helpers.c`

### Issue 6: Memory Leaks

**Symptoms:**
- Application memory grows over time
- System becomes slow
- Eventually runs out of memory

**Diagnosis:**

Monitor memory usage:
```cmd
perfmon.msc
```

Add counter: Process → Private Bytes → iexplore

**Solutions:**

1. **Context not freed:**
   - Ensure `DeleteSecurityContext` is called
   - Check application cleanup code

2. **Credentials not freed:**
   - Ensure `FreeCredentialsHandle` is called
   - Add to application cleanup

3. **OpenSSL leaks:**
   - Call `SSL_free()` properly
   - Verify in `DeleteSecurityContext`:
     ```c
     if (pCtx->ssl) {
         SSL_free(pCtx->ssl);  // Frees BIOs too
         pCtx->ssl = NULL;
     }
     ```

### Issue 7: Performance Issues

**Symptoms:**
- Slow page loading
- High CPU usage
- Laggy responses

**Diagnosis:**

Profile the application:
1. Use Performance Monitor
2. Check CPU time in DLL
3. Compare with original schannel.dll

**Solutions:**

1. **Excessive logging:**
   - Rebuild without DEBUG flag
   - Remove `LOG()` calls from hot paths

2. **Inefficient buffer handling:**
   - Review BIO_read/BIO_write calls
   - Minimize copies in `crypto_wrapper.c`

3. **Context recreation:**
   - Reuse SSL contexts when possible
   - Check application's connection handling

### Issue 8: Windows Update Fails

**Symptoms:**
- Windows Update can't connect
- Error 0x80072EFD or similar
- Updates were working before

**Diagnosis:**

Check if Windows Update uses Schannel:
```cmd
reg query "HKLM\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate" /s
```

**Solutions:**

1. **Temporarily revert:**
   ```cmd
   cd C:\Windows\System32
   ren schannel.dll schannel_wrapper.dll
   ren schannel_orig.dll schannel.dll
   ```
   
   Run Windows Update, then swap back.

2. **Certificate validation:**
   - Windows Update servers need specific validation
   - Check certificate chain in wrapper

3. **Use original for WU:**
   - Configure Windows Update to use specific DLL
   - Or exclude update service from wrapper

## Debug Techniques

### Enable Debug Build

Rebuild with debugging:
```bash
make CFLAGS="-g -DDEBUG -O0" clean all
```

### Capture Debug Output

**Method 1: DebugView**
1. Download DebugView from Sysinternals
2. Run as Administrator
3. Capture → Capture Global Win32
4. Run your application
5. See debug messages in real-time

**Method 2: Redirect stderr**
```cmd
iexplore.exe 2> debug.log
```

### Use Logging

Add temporary logging:
```c
FILE *logfile = fopen("C:\\wrapper.log", "a");
fprintf(logfile, "Function called: %s\n", __FUNCTION__);
fflush(logfile);
fclose(logfile);
```

### Attach Debugger

**With Visual Studio:**
1. Build debug version
2. Start application
3. Debug → Attach to Process
4. Select process
5. Set breakpoints in wrapper code

**With WinDbg:**
```cmd
windbg -p <pid>
```

## Advanced Diagnostics

### Check TLS Handshake

Use Wireshark to capture handshake:
1. Start capture on network interface
2. Filter: `tcp.port == 443`
3. Connect to HTTPS site
4. Right-click packet → Follow → SSL Stream
5. Check for:
   - Client Hello (TLS version offered)
   - Server Hello (TLS version selected)
   - Certificate validation
   - Handshake completion

### Verify Cipher Suites

Check what's offered:
```cmd
openssl s_client -connect site.com:443 -cipher 'ALL:COMPLEMENTOFALL' -tls1_2
```

### Test Specific Scenarios

**Test certificate validation:**
```cmd
openssl s_client -connect expired.badssl.com:443
```
(Should fail with certificate error)

**Test protocol versions:**
```cmd
openssl s_client -connect site.com:443 -tls1
openssl s_client -connect site.com:443 -tls1_1
openssl s_client -connect site.com:443 -tls1_2
```

## Getting Help

If issues persist:

1. **Gather information:**
   - Windows version
   - OpenSSL version
   - Application name and version
   - Debug log output
   - Network capture (if possible)

2. **Search issues:**
   - Check GitHub Issues
   - Search for similar problems

3. **Create issue:**
   - Provide all gathered information
   - Include minimal reproduction steps
   - Attach logs (sanitize sensitive data)

4. **Try workarounds:**
   - Use per-application installation
   - Test with different OpenSSL versions
   - Try on different Windows XP machine

## Recovery

### Emergency Revert

If system becomes unstable:

1. **Boot to Safe Mode** (F8 during boot)

2. **Restore original DLL:**
   ```cmd
   cd C:\Windows\System32
   del schannel.dll
   ren schannel_orig.dll schannel.dll
   ```

3. **Reboot normally**

### System Restore

If you created a restore point:
1. Start → All Programs → Accessories → System Tools → System Restore
2. Choose restore point before wrapper installation
3. Follow wizard

## Prevention

To avoid issues:

1. **Always backup** original schannel.dll
2. **Test in VM** before production
3. **Create restore point** before installation
4. **Start with per-application** installation
5. **Keep OpenSSL updated** to latest 1.0.2 release
6. **Monitor logs** after installation

## Reference

### Error Codes

Common SECURITY_STATUS errors:

- `SEC_E_OK` (0x00000000) - Success
- `SEC_I_CONTINUE_NEEDED` (0x00090312) - Need more data
- `SEC_E_INCOMPLETE_MESSAGE` (0x80090318) - Incomplete data
- `SEC_E_INVALID_HANDLE` (0x80090301) - Bad handle
- `SEC_E_INVALID_TOKEN` (0x80090308) - Bad token/buffer
- `SEC_E_INTERNAL_ERROR` (0x80090304) - Internal error
- `SEC_E_CERT_EXPIRED` (0x80090328) - Certificate expired
- `SEC_E_DECRYPT_FAILURE` (0x80090330) - Decryption failed
- `SEC_E_ENCRYPT_FAILURE` (0x80090329) - Encryption failed

### OpenSSL Errors

Check OpenSSL error queue:
```c
unsigned long err = ERR_get_error();
char buf[256];
ERR_error_string_n(err, buf, sizeof(buf));
printf("OpenSSL error: %s\n", buf);
```

### Useful Commands

```cmd
# Check loaded DLLs
tasklist /m schannel.dll

# Check file version
wmic datafile where name="C:\\Windows\\System32\\schannel.dll" get version

# Check system info
systeminfo | find "OS"

# Check OpenSSL
openssl version -a

# Network test
ping www.google.com
telnet www.google.com 443
```
