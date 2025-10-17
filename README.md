# Schannel OpenSSL Wrapper

## Project Overview
This project aims to replace the Windows `schannel.dll` with an OpenSSL-backed implementation for Windows XP/2000. It provides a wrapper around OpenSSL functionalities to ensure compatibility with existing applications that depend on Schannel.

## Architecture
The architecture involves creating a wrapper DLL that utilizes OpenSSL for handling SSL/TLS communications. This wrapper will intercept calls to Schannel and redirect them to OpenSSL, providing a seamless transition for applications.

## Build Instructions
1. Install OpenSSL on your system.
2. Clone this repository.
3. Navigate to the `src` directory.
4. Compile the wrapper using your preferred compiler (e.g., GCC, MSVC).

## Goals
- Provide a fully functional replacement for `schannel.dll`.
- Ensure compatibility with existing applications.
- Support SSL/TLS protocols using OpenSSL.
- Target Windows XP/2000 platforms.