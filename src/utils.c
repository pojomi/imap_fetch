#include "../include/utils.h"
#include "../include/decoders.h"
#include <openssl/ssl.h>


char *my_memmem(const char *haystack, size_t hay_len, const char *needle, size_t needle_len)
{
    if (needle_len > hay_len) return NULL;

    for (size_t i = 0; i < hay_len - needle_len; i++)
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
    if (needle.len == 0)
    {
        return -1;
    }

    char *found = my_memmem(haystack.data, haystack.len, needle.data, needle.len);

    if (found)
    {
        return found - haystack.data;
    }

    return -1;
}

Str find_subsbring(Str haystack, Str needle)
{
    if (needle.len == 0)
    {
        return (Str){NULL, 0};
    }

    char *found = my_memmem(haystack.data, haystack.len, needle.data, needle.len);

    if (found)
    {
        return (Str){found, haystack.len - (found - haystack.data)};
    }
    return (Str){NULL, 0};
}

int wrapper_ssl_read(void *wrap_ssl, void *buf, size_t len)
{
    int bytes_read = SSL_read((SSL *)wrap_ssl, buf, len);
    if (bytes_read > 0)
    {
        return bytes_read;
    }
    else
    {
        return -1;
    }
}

int wrapper_ssl_write(void *wrap_ssl, const void *buf, size_t len)
{
    int bytes_written = SSL_write((SSL *)wrap_ssl, buf, len);
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

int imap_download_attach(void *wrap_ssl, char *filename, char *buffer, size_t buffer_size, int bytes_read)
{
    
    FILE *f = fopen(filename, "wb");
    if (!f)
    {
        perror("Failed to open file\n");
        return -1;
    }
    // strchr points to the address of the first match in the string
    // so that gives us the starting point of the download size value in the curly braces
    // char *open_brace = memchr(buffer, '{', sizeof(buffer));
    // int *close_brace = memchr(buffer, '}', sizeof(buffer));
    char *open_brace = strchr(buffer, '{');
    char *close_brace = strchr(buffer, '}');

    // printf("close_brace: %s\n", close_brace);
    // atol only retrieves the ASCII in the string
    // returns as type long
    int download_size = atol(open_brace + 1);
    int downloaded_bytes = 0;
    char *data_start = close_brace + 3;
    int initial_response_data = bytes_read - (data_start - buffer);

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
    char *decoded_data = malloc(buffer_size);
    memset(decoded_data, 0, buffer_size);
    int b64_file = 0;
    int qp_file = 0;
    if (strstr(buffer, "JVBE"))
    {
        initial_decoded_len = decode_64(decoded_data, buffer_size, data_start, initial_response_data, &leftover);
        b64_file = 1;
        if (initial_decoded_len <= 0)
        {
            b64_file = 0;
            free(decoded_data);
            fclose(f);
            return -1;
        }
    }
    else if (strstr(buffer, "PDF-"))
    {
        initial_decoded_len = decode_qp(data_start, initial_response_data, &leftover);
        qp_file = 1;
    }
    else 
    {
        perror("Decoding for this type not supported\n");
        free(decoded_data);
        fclose(f);
        return -1;
    }
    if (leftover > 0)
    {
        memcpy(pending, data_start + initial_response_data - leftover, leftover);
        pending_len += leftover;
    }

    // Check for download_size which is provided in the initial buffer response header
    
    if (initial_response_data > 0)
    {
        if (b64_file == 1)
        {
            fwrite(decoded_data, 1, initial_decoded_len, f);
        }
        else if (qp_file == 1)
        {
            fwrite(data_start, 1, initial_decoded_len, f);
        }
        else
        {
            printf("No valid encoding found. Exiting\n");
            free(decoded_data);
            fclose(f);
            return -1;
        }
        downloaded_bytes += initial_response_data - leftover;
        printf("Downloaded %d bytes of %d\n", downloaded_bytes, download_size);
        memset(decoded_data, 0, initial_decoded_len);
    }
    
    while (downloaded_bytes < download_size)
    {
        int remaining = download_size - downloaded_bytes;
        int to_read = (remaining < buffer_size - pending_len ? remaining : buffer_size - pending_len);
        int chunk_bytes_read = wrapper_ssl_read(wrap_ssl, buffer, to_read);
        

        if (pending_len > 0)
        {
            memmove(buffer + pending_len, buffer, chunk_bytes_read);
            memcpy(buffer, pending, pending_len);
            chunk_bytes_read += pending_len;
            pending_len = 0;
        }
        if (b64_file == 1)
        {
            decoded_len = decode_64(decoded_data, buffer_size, buffer, chunk_bytes_read, &leftover);
            if (decoded_len < 0)
            {
                free(decoded_data);
                fclose(f);
                return -1;
            }
        }
        if (qp_file == 1)
        {
            decoded_len = decode_qp(buffer, chunk_bytes_read, &leftover);
            if (decoded_len < 0)
            {
                printf("Failed to decode current block. Exiting\n");
                fflush(f);
                fclose(f);
                free(decoded_data);
                return -1;
            }
        }

        if (leftover > 0)
        {
            memcpy(pending, buffer + chunk_bytes_read - leftover, leftover);
            pending_len = leftover;
        }
        
        if (downloaded_bytes + chunk_bytes_read > download_size)
        {
            decoded_len = download_size - downloaded_bytes;
        }
        int download_percent = (downloaded_bytes * 100) / download_size;
        if (b64_file == 1)
        {
            char *eof = strstr(decoded_data, "%%EOF");
            if (eof)
            {
                decoded_len = (size_t)(eof + 4);
                downloaded_bytes += chunk_bytes_read - leftover;
                break;   
            }
            fwrite(decoded_data, 1, decoded_len, f);
        }
        else if (qp_file == 1)
        {
            fwrite(buffer, 1, decoded_len, f);
        }
        downloaded_bytes += chunk_bytes_read - leftover;
        printf("\rDownloaded %d bytes of %d (%d%%)", downloaded_bytes, download_size, download_percent);
        fflush(stdout);
        printf("\n");
        memset(decoded_data, 0, decoded_len);
    }
    printf("Downloaded %d bytes of %d\n", downloaded_bytes, download_size);
    fclose(f);
    printf("File saved as %s\n", filename);
    free(decoded_data);
    b64_file = 0;
    qp_file = 0;
    return 0;
}

int receive_imap_response(void *wrap_ssl, char *tag)
{
    buffer buff = {0};
    buff.data = calloc(256, sizeof(char));
    buff.capacity = 256;

    int download_initialized = 0;
    int bytes_read;
    int total_bytes_read = 0;
    
    while (1)
    {
        bytes_read = wrapper_ssl_read(wrap_ssl, buff.data, (int)buff.capacity - 1);
        
        if (bytes_read <= 0)
        {
            printf("0 bytes found\n");
            break;
        }
        buff.capacity += bytes_read;
        if (strstr((const char *)buff.data, "BAD"))
        {
            printf("BAD response received from server\n");
            memset(buff.data, 0, buff.capacity);
            break;
        }
        if(strstr((const char *)buff.data, tag) == NULL)
        {
            total_bytes_read += bytes_read;
            
            if (download_initialized == 0)
            {
                if (buff.capacity < 2048)
                {
                    buff.capacity *= 2;
                }

                buff.data = realloc(buff.data, buff.capacity);
            }
        }
        if (strstr((const char *)buff.data ,tag))
        {
            total_bytes_read += bytes_read;
            bytes_read = total_bytes_read;
            buff.capacity = bytes_read;
            buff.data = realloc(buff.data, buff.capacity);

            // Conditions to handle file download (tag FETCH [ID] body[PART])
            // Response includes {RB_SIZE}
            // Parse that and store as download size
            if (strstr(tag, "(BODY"))
            {
                printf("Response received:\n%s\n", (const char *)buff.data);
                printf("Bytes: %d\n", bytes_read);
                char filename[128];
                printf("Name file>");
                if (fgets(filename, sizeof(filename), stdin) != NULL)
                {
                    filename[strcspn(filename, "\n")] = '\0';
                }
                printf("Beginning file creation\n");
                if (imap_download_attach(wrap_ssl, filename, buff.data, buff.capacity, bytes_read) != 0)
                {
                    download_initialized = 1;
                    break;
                }
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

    if (receive_imap_response(cwrap->wrap_ssl, "ready for requests") == 1)
    {
        perror("No initial success message\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}