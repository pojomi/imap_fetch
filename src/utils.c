#include "../include/utils.h"
#include "../include/decoders.h"
#include <openssl/ssl.h>


char *my_memmem(const char *haystack, size_t hay_len, const char *needle, size_t needle_len)
{
    if (hay_len < needle_len) 
    {
        return NULL;
    }
    
    for (size_t i = 0; i <= hay_len - needle_len; i++)
    {
        if (memcmp(haystack + i, needle, needle_len) == 0)
        {
            return (char *)(haystack + i);
        }
    }
    return NULL;
}

int find_offset(Str haystack, Str needle)
{
    printf("Looking for %s\n", needle.data);
    if (needle.len == 0)
    {
        return -1;
    }

    char *found = my_memmem(haystack.data, haystack.len, needle.data, needle.len);

    if (found)
    {
        return (found - haystack.data);
    }

    return -1;
}

Str find_substring(Str haystack, Str needle)
{
    if (needle.len == 0)
    {
        printf("Invalid substring search\n");
        return (Str){NULL, 0};
    }

    char *found = my_memmem(haystack.data, haystack.len, needle.data, needle.len);
    
    if (found != NULL)
    {
        return (Str){found, haystack.len - (found - haystack.data)};
    }
    return (Str){NULL, 0};
}

size_t wrapper_ssl_read(void *wrap_ssl, void *buf, size_t len)
{
    size_t bytes_read = (size_t)SSL_read((SSL *)wrap_ssl, buf, (int)len);
    if (bytes_read > 0)
    {
        return bytes_read;
    }
    else
    {
        return -1;
    }
}

size_t wrapper_ssl_write(void *wrap_ssl, const void *buf, size_t len)
{
    size_t bytes_written = (size_t)SSL_write((SSL *)wrap_ssl, buf, (int)len);
    if (bytes_written > 0)
    {
        return bytes_written;
    }
    else
    {
        return -1;
    }
}

void wrapper_ssl_free(void *wrap_ssl)
{
    SSL_free((SSL *)wrap_ssl);
}

void wrapper_ssl_ctx_free(void *wrapper_ssl_ctx)
{
    SSL_CTX_free((SSL_CTX *)wrapper_ssl_ctx);
}

int imap_send_request(void *wrap_ssl, char *request)
{
    printf("Sending: %s\n", request);
    if (wrapper_ssl_write(wrap_ssl, request, strlen(request)) <= 0)
    {
        perror("Failed to send message\n");
        return -1;
    }
    printf("Successfully sent\n");
    return 0;
}

int imap_download_attach(void *wrap_ssl, char *filename, Str buff, size_t bytes_read)
{
    
    FILE *f = fopen(filename, "wb");
    if (!f)
    {
        perror("Failed to open file\n");
        return -1;
    }

    // Download size (encoded) is found between {} in the server response
    int open_brace = find_offset(buff, STR_LIT("{"));
    int close_brace = find_offset(buff, STR_LIT("}"));


    // atol only retrieves the ASCII in the string
    // returns as type long
    // int download_size = atol(open_brace + 1);
    size_t download_size = atol(buff.data + open_brace + 1);
    size_t downloaded_bytes = 0;
    char *data_start = (buff.data + close_brace + 3);
    size_t initial_response_data = bytes_read - (data_start - buff.data);
    if (initial_response_data > 0)

    if (initial_response_data > download_size)
    {
        initial_response_data = download_size;
    }
    // 4 byte allocation since base64 encodes in chunks of 4 and quoted-printable in chunks of 3
    char pending[4];
    size_t leftover = 0;
    size_t pending_len = 0;
    size_t initial_decoded_len;
    size_t decoded_len = 0;
    buffer decoded = {0};
    decoded.data = calloc(buff.len, sizeof(char));
    decoded.capacity = buff.len;
    decoded.len = buff.len;
    int b64_file = 0;
    int qp_file = 0;

    Str parsed_decode_ss_64 = find_substring((Str){buff.data, buff.len}, STR_LIT("JVBE"));
    Str parsed_decode_ss_qp = find_substring((Str){buff.data, buff.len}, STR_LIT("PDF"));
    if (parsed_decode_ss_64.data != NULL)
    {
        initial_decoded_len = decode_64((Str){decoded.data, decoded.len}, (Str){data_start, initial_response_data}, &leftover);
        b64_file = 1;
        if (initial_decoded_len <= 0)
        {
            b64_file = 0;
            free(decoded.data);
            fclose(f);
            return -1;
        }
    }
    else if (parsed_decode_ss_qp.data != NULL)
    {
        initial_decoded_len = decode_qp((Str){data_start, initial_response_data}, &leftover);
        qp_file = 1;
    }
    else 
    {
        perror("Decoding for this type not supported\n");
        free(decoded.data);
        fclose(f);
        return -1;
    }
    if (leftover > 0)
    {
        memcpy(pending, data_start + initial_response_data - leftover, leftover);
        pending_len += leftover;
    }

    // Check for download_size which is provided in the initial buff.data response header
    
    if (initial_response_data > 0)
    {
        if (b64_file == 1)
        {
            fwrite(decoded.data, 1, initial_decoded_len, f);
        }
        else if (qp_file == 1)
        {
            fwrite(data_start, 1, initial_decoded_len, f);
        }
        else
        {
            printf("No valid encoding found. Exiting\n");
            free(decoded.data);
            fclose(f);
            return -1;
        }
        downloaded_bytes += initial_response_data - leftover;
        if (b64_file == 1 )
        {
            memset(decoded.data, 0, initial_decoded_len);
        }
    }
    
    while (downloaded_bytes < download_size)
    {
        if (b64_file == 1) memset(decoded.data, 0, decoded.capacity);
        
        size_t remaining = download_size - downloaded_bytes;
        size_t to_read = (remaining < buff.len - pending_len ? remaining : buff.len - pending_len);
        size_t chunk_bytes_read = wrapper_ssl_read(wrap_ssl, buff.data, to_read);

        if (pending_len > 0)
        {
            memmove(buff.data + pending_len, buff.data, chunk_bytes_read);
            memcpy(buff.data, pending, pending_len);
            chunk_bytes_read += pending_len;
            pending_len = 0;
        }
        if (b64_file == 1)
        {
            decoded_len = decode_64((Str){decoded.data, decoded.len}, (Str){buff.data, chunk_bytes_read}, &leftover);

            // decoded_len == 0 is valid when chunk only contains leftover bytes
            // Only error if we got 0 decoded AND no leftover (unexpected)
            if (decoded_len == 0 && leftover == 0 && chunk_bytes_read > 0)
            {
                printf("Decode error: no output and no leftover\n");
                free(decoded.data);
                fclose(f);
                return -1;
            }
        }
        if (qp_file == 1)
        {
            decoded_len = decode_qp((Str){buff.data, chunk_bytes_read}, &leftover);
        }

        if (leftover > 0)
        {
            // Move the leftover bytes into pending array
            memcpy(pending, buff.data + chunk_bytes_read - leftover, leftover);
            pending_len = leftover;
        }
        
        if (downloaded_bytes + chunk_bytes_read > download_size)
        {
            decoded_len = download_size - downloaded_bytes;
            // printf("Math error! downloaded_bytes + chunk_bytes_read > download_size\n");
            // printf("downloaded_bytes: %ld\nchunk_bytes_read: %ld\ndownload_size: %ld\n", downloaded_bytes, chunk_bytes_read, download_size);
            // if (b64_file == 1) free(decoded.data);
            // return -1;
        }
        int download_percent = (downloaded_bytes * 100) / download_size;
        if (b64_file == 1)
        {
            fwrite(decoded.data, 1, decoded_len, f);
        }
        else if (qp_file == 1)
        {
            fwrite(buff.data, 1, decoded_len, f);
        }
        downloaded_bytes += chunk_bytes_read - leftover;
        printf("\rDownloaded %ld bytes of %ld (%d%%)", downloaded_bytes, download_size, download_percent);
        fflush(stdout);
        printf("\n");
        if (b64_file == 1) memset(decoded.data, 0, decoded_len);
    }
    printf("Downloaded %ld bytes of %ld\n", downloaded_bytes, download_size);
    fclose(f);
    printf("File saved as %s\n", filename);
    free(decoded.data);
    b64_file = 0;
    qp_file = 0;
    return 0;
}

int receive_imap_response(void *wrap_ssl, Str *tag)
{
    buffer buff = {0};
    buff.data = calloc(256, sizeof(char));
    buff.capacity = 256;
    buff.len = 0;

    int download_initialized = 0;
    // int bytes_read;
    int total_bytes_read = 0;
    
    while (1)
    {
        buff.len = wrapper_ssl_read(wrap_ssl, buff.data, buff.capacity - 1);
        
        if (buff.len <= 0)
        {
            printf("0 bytes found\n");
            break;
        }

        Str no_match = find_substring((Str){buff.data, buff.len}, *tag);
        if (no_match.data == NULL)
        {    
            if (download_initialized == 0)
            {
                if (buff.capacity < 2048)
                {
                    buff.capacity *= 2;
                }
    
                buff.data = realloc(buff.data, buff.capacity);
            }
        }
        Str match = find_substring((Str){buff.data, buff.len}, *tag);
        if (match.data != NULL)
        {
            total_bytes_read += buff.len;
            buff.len = total_bytes_read;
            buff.capacity = buff.len;
            buff.data = realloc(buff.data, buff.capacity);

            // Conditions to handle file download (tag FETCH [ID] body[PART])
            // Response includes {RB_SIZE}
            // Parse that and store as download size
            Str download_response = find_substring((Str){buff.data, buff.len}, STR_LIT("(BODY"));
            if (download_response.data != NULL)
            {
                Str filename = {0};
                filename.data = calloc(128, sizeof(char));
                filename.len = 128;
                printf("Name file>");
                if (fgets(filename.data, filename.len, stdin) != NULL)
                {
                    // Remove \n from the filename
                    int filename_offset = find_offset(filename, STR_LIT("\n"));
                    memset(filename.data + filename_offset, 0, sizeof(char));
                    
                }
                printf("Beginning file creation\n");
                if (imap_download_attach(wrap_ssl, filename.data, (Str){buff.data, buff.len}, total_bytes_read) != 0)
                {
                    download_initialized = 1;
                    continue;
                }
                free(filename.data);
            }
            if (download_initialized == 0)
            {
                printf("Response received:\n%s\n", (const char *)buff.data);
                break;
            }
        }
    }
    download_initialized = 0;
    free(buff.data);
    return 0;
}

int imap_prompt_login(void *wrap_ssl)
{
    while (1)
    {
        printf("Email address>");
        char email_buffer[128];
        if (fgets(email_buffer, sizeof(email_buffer), stdin) == NULL)
        {
            perror("Input error. Enter email\n");
            continue;
        }
        email_buffer[strcspn(email_buffer, "\n")] = '\0';
        char password_buffer[128];
        printf("Enter password>");
        if (fgets(password_buffer, sizeof(password_buffer), stdin) == NULL)
        {
            perror("Input error. Enter password\n");
            continue;
        }
        
        password_buffer[strcspn(password_buffer, "\n")] = '\0';
        char imap_formatted_message[320];
        snprintf(imap_formatted_message, sizeof(imap_formatted_message), "a1 LOGIN %s %s\r\n", email_buffer, password_buffer);
        if (imap_send_request(wrap_ssl, imap_formatted_message) != 0)
        {
            perror("Message sending failure\n");
            continue;
        }
        return 0;
    }
}

int imap_init_connection(struct Connection_wrapper *cwrap)
{
    SSL_library_init();
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);

    cwrap->wrap_ssl_ctx = (void *)ctx;
    cwrap->wrap_ssl = (void *)ssl;
    cwrap->wrap_ssl_read = wrapper_ssl_read;
    cwrap->wrap_ssl_write = wrapper_ssl_write;

    const char *hostname = "imap.gmail.com";
    cwrap->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    SSL_set_fd(ssl, cwrap->sockfd);
    struct sockaddr_in host;
    struct hostent *he = gethostbyname("imap.gmail.com");
    memcpy(&host.sin_addr, he->h_addr_list[0], he->h_length);
    host.sin_family = AF_INET;
    host.sin_port = htons(993);

    if (connect(cwrap->sockfd, (const struct sockaddr *)&host, sizeof(host)) < 0)
    {
        perror("failed to connect\n");
        exit(EXIT_FAILURE);
    }

    if (!SSL_set_tlsext_host_name(ssl, hostname))
    {
        perror("failed set SSL host\n");
        exit(EXIT_FAILURE);
    }
    if (!SSL_set1_host(ssl, hostname))
    {
        perror("certificate failure\n");
        exit(EXIT_FAILURE);
    }
    if (SSL_connect(ssl) < 1)
    {
        perror("Failed to connect the socket to SSL\n");
        exit(EXIT_FAILURE);
    }

    if (receive_imap_response(cwrap->wrap_ssl, STR_LIT_PTR("OK")) == 1)
    {
        perror("No initial success message\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}