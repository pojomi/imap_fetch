# IMAP PDF Fetcher

A lightweight C application for downloading PDF attachments from Gmail via IMAP. Supports Base64 and Quoted-Printable encoded attachments with automatic encoding detection and decoding.

## Features

- Secure TLS/SSL connection to Gmail IMAP server (port 993)
- Interactive IMAP command prompt for full protocol control
- Automatic encoding detection (Base64 and Quoted-Printable)
- Chunked download with real-time progress tracking
- Dynamic memory management for efficient resource usage

## Requirements

- GCC compiler
- OpenSSL development libraries (`libssl-dev` or `openssl-devel`)
- Make

### Installing Dependencies

**Debian/Ubuntu:**
```bash
sudo apt-get install build-essential libssl-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc make openssl-devel
```

**macOS:**
```bash
brew install openssl
```

## Build and Installation

```bash
# Clone or download the repository
cd imap_fetch

# Build the project
make

# Clean build artifacts (if needed)
make clean

# Run the application
./imap_fetch
```

## Usage

### 1. Login

When you start the application, you'll be prompted to enter your email credentials:

```
Email address> your.email@gmail.com
Enter password> your_app_password
```

**Important:** For Gmail, you must use an [App Password](https://support.google.com/accounts/answer/185833), not your regular account password.

### 2. IMAP Commands

After successful login, you'll see the `IMAP>` prompt. All IMAP commands follow this format:

```
<tag> <COMMAND> <arguments>
```

- `<tag>` can be any identifier (e.g., `A1`, `A2`, `tag1`). It's recommended to increment it for each command to track requests.
- Commands are **case-insensitive**
- Server responses will indicate `OK` (success) or `BAD` (error)

#### Common IMAP Commands

| Command | Description | Example |
|---------|-------------|---------|
| `SELECT` | Select a mailbox | `A1 SELECT INBOX` |
| `EXAMINE` | Select mailbox (read-only) | `A2 EXAMINE INBOX` |
| `LIST` | List available mailboxes | `A3 LIST "" "*"` |
| `SEARCH` | Search for messages | `A4 SEARCH FROM "sender@example.com"` |
| `FETCH` | Retrieve message data | `A5 FETCH 1 BODYSTRUCTURE` |
| `LOGOUT` | Close connection | `A6 LOGOUT` |

#### Gmail-Specific Commands

Gmail provides extended IMAP search capabilities:

```
A1 SEARCH x-gm-raw "from:sender@example.com has:attachment"
A2 SEARCH x-gm-raw "subject:invoice has:attachment filename:pdf"
A3 SEARCH x-gm-raw "after:2024/01/01 has:attachment"
```

### 3. Finding PDF Attachments

To download a PDF, you first need to locate it in an email. Here's the workflow:

#### Step 1: Search for emails with attachments

```
A1 SELECT INBOX
A2 SEARCH FROM "sender@example.com"
```

This returns message IDs, e.g., `* SEARCH 1 5 12 23`

#### Step 2: Examine the message structure

Use `FETCH BODYSTRUCTURE` to view the MIME structure of a message:

```
A3 FETCH 5 BODYSTRUCTURE
```

**Understanding MIME and BODYSTRUCTURE:**

MIME (Multipurpose Internet Mail Extensions) is a standard that extends email to support:
- Text in character sets other than ASCII
- Non-text attachments (images, PDFs, etc.)
- Multi-part message bodies
- Header information in non-ASCII character sets

The `BODYSTRUCTURE` response shows the hierarchical structure of a message's parts:

```
* 5 FETCH (BODYSTRUCTURE (
  ("TEXT" "PLAIN" NIL NIL NIL "7BIT" 123 4)
  ("APPLICATION" "PDF" NIL NIL NIL "BASE64" 45678 NIL ("ATTACHMENT" ("FILENAME" "invoice.pdf")))
  "MIXED"))
```

**Breaking down the structure:**

1. **Part 1**: Text/Plain (the email body)
   - Type: TEXT
   - Subtype: PLAIN
   - Encoding: 7BIT
   - Size: 123 bytes

2. **Part 2**: Application/PDF (the attachment)
   - Type: APPLICATION
   - Subtype: PDF
   - Encoding: BASE64
   - Size: 45678 bytes (encoded)
   - Disposition: ATTACHMENT
   - Filename: invoice.pdf

**Determining the part number:**
- Single-part messages: Use `BODY[1]`
- Multi-part messages: Count the parts in order
  - `BODY[1]` = First part (usually text)
  - `BODY[2]` = Second part (often the attachment)
  - Nested parts use dot notation: `BODY[1.2]`

#### Step 3: Download the PDF

Once you know the part number (e.g., part 2 for the PDF):

```
A4 FETCH 5 BODY[2]
```

You'll be prompted to enter a filename:

```
Name file> invoice.pdf
```

**Important:** You **must** include the `.pdf` extension in your filename.

The download will begin with real-time progress:

```
Downloaded 5000 bytes of 45678 (10%)
Downloaded 10000 bytes of 45678 (21%)
...
Downloaded 45678 bytes of 45678 (100%)
File saved as invoice.pdf
```

The file will be saved in the application's root directory.

## IMAP Command Reference

### Search Criteria

Combine search criteria for powerful queries:

```
A1 SEARCH FROM "boss@company.com" SUBJECT "report"
A2 SEARCH SINCE 1-Jan-2024 FROM "client@example.com"
A3 SEARCH UNSEEN                           # Unread messages
A4 SEARCH FLAGGED                          # Starred/flagged messages
A5 SEARCH LARGER 1000000                   # Messages > 1MB
A6 SEARCH HEADER "Content-Type" "pdf"      # Messages with PDFs
```

### Fetch Options

Retrieve different parts of a message:

```
A1 FETCH 1 BODY[HEADER]                    # Headers only
A2 FETCH 1 BODY[TEXT]                      # Body text only
A3 FETCH 1 (FLAGS INTERNALDATE SIZE)       # Metadata
A4 FETCH 1 BODY[1.MIME]                    # MIME headers for part 1
A5 FETCH 1:5 BODYSTRUCTURE                 # Structure for messages 1-5
```

## Technical Details

### Supported Encodings

- **Base64**: Automatically detected by looking for `JVBE` (Base64-encoded PDF signature `JVBERi`)
- **Quoted-Printable**: Automatically detected by looking for `PDF-` literal

### Download Process

1. Server responds with `{SIZE}` indicating total encoded bytes
2. Application detects encoding type from initial chunk
3. Data is downloaded in chunks, decoded on-the-fly
4. Progress is tracked against the encoded byte count
5. For PDFs, download terminates early if `%%EOF` marker is found

### Memory Management

- Response buffers start at 256 bytes, dynamically grow to 2048 bytes max
- Download buffers match the response size for efficiency
- 4-byte pending buffer handles incomplete encoding sequences at chunk boundaries

## Troubleshooting

### Common Issues

**Login fails:**
- Ensure you're using a Gmail App Password, not your regular account password
- Enable IMAP access in Gmail settings (Settings > Forwarding and POP/IMAP)

**"BAD response received from server":**
- Check command syntax (use `FETCH ID BODYSTRUCTURE` to verify structure)
- Verify the message ID exists in the selected mailbox
- Ensure you've selected a mailbox with `SELECT` before searching/fetching

**Download fails or corrupt PDF:**
- Ensure you included `.pdf` in the filename
- Verify the part number is correct using `BODYSTRUCTURE`
- Check that the attachment is actually a PDF (not embedded image or other format)

**"Decoding for this type not supported":**
- The attachment uses an encoding other than Base64 or Quoted-Printable
- Try a different attachment or message

### Debug Tips

1. Use `BODYSTRUCTURE` to inspect message structure before downloading
2. Start with simple searches: `SEARCH ALL` to list all messages
3. Test with known messages containing PDF attachments
4. Check server responses for error details

## Example Session

```
$ ./imap_fetch
Email address> user@gmail.com
Enter password> your_app_password
Successfully logged in

IMAP> A1 SELECT INBOX
Response received:
* FLAGS (\Answered \Flagged \Deleted \Seen \Draft)
* 42 EXISTS
A1 OK [READ-WRITE] INBOX selected

IMAP> A2 SEARCH FROM "accountant@company.com"
Response received:
* SEARCH 15 23
A2 OK SEARCH completed

IMAP> A3 FETCH 23 BODYSTRUCTURE
Response received:
* 23 FETCH (BODYSTRUCTURE (("TEXT" "PLAIN"...)("APPLICATION" "PDF"...)))
A3 OK FETCH completed

IMAP> A4 FETCH 23 BODY[2]
Name file> Q4_report.pdf
Beginning file creation
Downloaded 125000 bytes of 125000 (100%)
File saved as Q4_report.pdf
```

## Known Limitations

- Currently only supports PDF attachments
- Filename must include `.pdf` extension (manual input)
- Files are saved to application root directory only
- Hardcoded to connect to Gmail IMAP server (can be modified in source)

## License

This project is provided as-is for educational purposes.

## Contributing

This is a learning project. Feel free to fork and modify for your own use.
