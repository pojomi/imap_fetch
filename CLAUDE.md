# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Run

```bash
# Build the project
make

# Clean build artifacts
make clean

# Run the application
./imap_fetch
```

## Architecture

This is an IMAP client for downloading and decoding PDF attachments from Gmail. The application uses OpenSSL for TLS connections on port 993.

### Core Components

**src/imap_fetch.c**: Main application entry point. Establishes SSL connection to imap.gmail.com, provides interactive IMAP command prompt after login. Detects `body[` commands to trigger attachment download workflow.

**src/utils.c**: Protocol and encoding utilities:
- `receive_imap_response()`: Reads IMAP responses, dynamically grows buffer (capped at 2048 bytes), triggers download on `(BODY` tag
- `imap_download_attach()`: Handles chunked downloads with encoding detection and decoding
- `decode_64()` / `decode_qp()`: Base64 and Quoted-Printable decoders with leftover handling for chunk boundaries

### Key Design Patterns

**Encoding Detection**: Uses heuristics to detect encoding type by searching for magic bytes:
- Base64: Looks for "JVBE" (common in PDF Base64 start: `JVBERi...`)
- Quoted-Printable: Looks for "PDF-" literal

**Chunked Decoding**: Uses a 4-byte `pending` buffer to handle incomplete Base64/QP sequences at chunk boundaries. The `leftover` parameter tracks bytes that need to carry over to next chunk.

**Byte Accounting**: `downloaded_bytes` tracks **raw encoded bytes** from SSL (not decoded bytes) to match against the IMAP server's `{SIZE}` literal. Formula: `downloaded_bytes += chunk_bytes_read - leftover` where leftover from iteration N becomes pending in iteration N+1.

**Buffer Growth Strategy**: Buffers double in size (not shrink) to minimize reallocation overhead during IMAP response parsing.

### Error Handling Convention

All utility functions return `-1` on error, `0` on success. File handles must be closed (`fclose(f)`) before returning on all error paths.

### IMAP Protocol Notes

Gmail-specific search: `tag SEARCH x-gm-raw "from: has:"`

IMAP SEARCH cannot filter by MIME part properties (e.g., "PDFs with Quoted-Printable encoding"). Client-side filtering after FETCH is required.
