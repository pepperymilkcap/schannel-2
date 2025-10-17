# Makefile for schannel.dll wrapper
# Targets Windows XP/2000 using MinGW

CC = gcc
WINDRES = windres
DLLTOOL = dlltool

# Target Windows XP
CFLAGS = -Wall -O2 -D_WIN32_WINNT=0x0501 -DWINVER=0x0501 -DUNICODE -D_UNICODE
CFLAGS += -I./include -I/usr/local/include -I/usr/include/openssl

# Link flags
LDFLAGS = -shared -Wl,--enable-stdcall-fixup -Wl,--kill-at
LDFLAGS += -L/usr/local/lib -L/usr/lib

# Libraries
LIBS = -lssl -lcrypto -lws2_32 -lcrypt32 -lsecur32 -lkernel32 -ladvapi32

# Source files
SOURCES = src/dllmain.c \
          src/openssl_helpers.c \
          src/sspi_wrapper.c \
          src/context_wrapper.c \
          src/crypto_wrapper.c \
          src/query_wrapper.c \
          src/spusermode.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Output
TARGET = schannel.dll
DEF_FILE = src/schannel.def

# Default target
all: $(TARGET)

# Build the DLL
$(TARGET): $(OBJECTS) $(DEF_FILE)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(DEF_FILE) $(LIBS)
	@echo "Build complete: $(TARGET)"
	@echo "Note: Rename original schannel.dll to schannel_orig.dll in System32"

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

# Install (copy to Windows system directory - requires admin)
install: $(TARGET)
	@echo "WARNING: This will replace system schannel.dll"
	@echo "1. Backup current schannel.dll"
	@echo "2. Rename it to schannel_orig.dll"
	@echo "3. Copy $(TARGET) to System32"
	@echo "Run manually with administrator privileges"

# Help
help:
	@echo "Schannel OpenSSL Wrapper Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build schannel.dll (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  install  - Show installation instructions"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Requirements:"
	@echo "  - MinGW GCC toolchain"
	@echo "  - OpenSSL development libraries"
	@echo "  - Windows SDK headers"

.PHONY: all clean install help
