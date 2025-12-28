#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>

int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;

    return -1;
}

int get_b64_value(char c)
{
    switch(c)
    {
        case 'A': return 0; case 'B':  return 1; case 'C': return 2; case 'D': return 3; case 'E': return 4; case 'F': return 5; case 'G': return 6;
        case 'H': return  7; case 'I': return 8; case 'J': return 9; case 'K': return 10; case 'L': return 11; case 'M': return 12; case 'N': return 13;
        case 'O': return  14; case 'P': return 15; case 'Q': return 16; case 'R': return 17; case 'S': return 18; case 'T': return 19; case 'U': return 20;
        case 'V': return  21; case 'W': return 22; case 'X': return 23; case 'Y': return 24; case 'Z': return 25; case 'a': return 26; case 'b': return 27;
        case 'c': return  28; case 'd': return 29; case 'e': return 30; case 'f': return 31; case 'g': return 32; case 'h': return 33; case 'i': return 34;
        case 'j': return  35; case 'k': return 36; case 'l': return 37; case 'm': return 38; case 'n': return 39; case 'o': return 40; case 'p': return 41;
        case 'q': return  42; case 'r': return 43; case 's': return 44; case 't': return 45; case 'u': return 46; case 'v': return 47; case 'w': return 48;
        case 'x': return  49; case 'y': return 50; case 'z': return 51; case '0': return 52; case '1': return 53; case '2': return 54; case '3': return 55;
        case '4': return  56; case '5': return 57; case '6': return 58; case '7': return 59; case '8': return 60; case '9': return 61; case '+': return 62; 
        case '/': return  63;

        default: return -1;
    }
}

size_t decode_64(char *d, size_t d_len, char *s, size_t s_len, size_t *leftover)
{
    size_t i = 0;
    size_t j = 0;
    int b64v1, b64v2, b64v3, b64v4;
    *leftover = 0;
    
    while (i < s_len)
    {
        if (i + 3 < s_len)
        {
            if (s[i] == '\r' || s[i] == '\n')
            {
                i++;
                continue;
            }
            if (s[i+2] == '=' && s[i+3] == '=')
            {
                // Decode double padded
                b64v1 = get_b64_value(s[i]);
                if (b64v1 == -1) 
                {
                    printf("Failed on first byte from double padded block\n");
                    return -1;
                }
                b64v2 = get_b64_value(s[i+1]);
                if (b64v2 == -1) 
                {
                    printf("Failed on second byte of double padded block\n");
                    return -1;
                }

                d[j++] = (b64v1 << 2) | (b64v2 >> 4);
                // memmove(d + i + 2, d + i + 3, (strlen(d) - i + 2));
                i+=4;
                // j+=2;

                continue;
            }

            if (s[i+3] == '=')
            {
                // Decode single padded
                b64v1 = get_b64_value(s[i]);
                if (b64v1 == -1) 
                {
                    printf("Failed on first byte of single padded block\n");
                    return -1;
                }
                b64v2 = get_b64_value(s[i+1]);
                if (b64v2 == -1) 
                {
                    printf("Failed on second byte of single padded block\n");
                    return -1;
                }
                b64v3 = get_b64_value(s[i+2]);
                if (b64v3 == -1) 
                {
                    printf("Failed on third byte of single padded block\n");
                    return -1;
                }

                d[j++] = (b64v1 << 2) | (b64v2 >> 4);
                d[j++] = (b64v2 << 4) | (b64v3 >> 2);
                // memmove(d + i + 3, d + i + 4, (strlen(d) - i + 3));
                i+=4;
                // j+=3;

                continue;
            }
            b64v1 = get_b64_value(s[i]);
            if (b64v1 == -1) 
            {
                printf("Failed on first byte\n");
                return -1;
            }
            b64v2 = get_b64_value(s[i+1]);
            if (b64v2 == -1) 
            {
                printf("Failed on second byte\n");
                return -1;
            } 
                
            b64v3 = get_b64_value(s[i+2]);
            if (b64v3 == -1) 
            {
                printf("Failed on third byte\n");
                return -1;
            } 
                
            b64v4 = get_b64_value(s[i+3]);
            if (b64v4 == -1) 
            {
                printf("Failed on fourth block\n");
                return -1;
            }
        
            d[j++] = (b64v1 << 2) | (b64v2 >> 4);
            d[j++] = (b64v2 << 4) | (b64v3 >> 2);
            d[j++] = (b64v3 << 6) | (b64v4);
            // memmove(d + i + 3, d + i + 4, (strlen(d) - i + 3));
            i+=4;
            // j+=3;
            continue;
        }
        else
        {
            printf("End of buffered data\n");
            // printf("Leftover: %lu, i: %lu", len, i);
            // Use leftover as data index reference for the return
            *leftover = s_len - i;
            i+=4;
            break;
        }
    }
    return j;
}

size_t decode_qp(char *data, size_t len, size_t *leftover)
{
    // Read position
    size_t i = 0;
    // Write position
    size_t j = 0;
    *leftover = 0;
    while (i < len)
    {
        if (data[i] == '=')
        {
            if (i + 2 >= len)
            {
                *leftover = len - i;
                break;
            }
            // For \r, disregard
            if (i + 2 < len && data[i + 1] == '\r' && data[i + 2] == '\n')
            {
                i += 3;
                continue;
            }
            // Hex conversion
            // Verify that there are 2 bytes after index position
            // For \n, disregard
            if (i + 2 < len && data[i + 1] == '\n')
            {
                // Only increment by 2 since there's only 2 characters here
                i += 2;
                continue;
            }
            if (i + 2 < len)
            {
                int hi = hexval(data[i + 1]);
                int low = hexval(data[i + 2]);

                if (hi >= 0 && low >= 0)
                {
                    // Write string at read_pos then ++
                    // Since unsigned char ==  8 bits, shift hi by 4 bits, then OR low into last 4
                    data[j++] = (unsigned char)((hi << 4) | low);
                    // Move read_pos up by 3 chars to next =XX 0 index
                    i += 3;
                    continue;
                }
            }
            // Keep write_pos at the '=' (0 index) of read_pos
            data[j++] = data[i++];
        }
        else
        {
            data[j++] = data[i++];
        }
    }
    return j;
}

int imap_send_request(SSL *ssl, char *request)
{
    printf("Sending: %s\n", request);
    if (SSL_write(ssl, request, strlen(request)) <= 0)
    {
        perror("Failed to send message\n");
        return 1;
    }
    printf("Successfully sent\n");
    return 0;
}

int imap_download_attach(SSL *ssl, char *filename, char *buffer, size_t buffer_size, int *bytes_read)
{
    
    FILE *f = fopen(filename, "wb");
    if (!f)
    {
        perror("Failed to open file\n");
        return 1;
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
    int initial_response_data = *bytes_read - (data_start - buffer);

    if (initial_response_data > download_size)
    {
        initial_response_data = download_size;
    }
    char pending[4];
    size_t leftover = 0;
    size_t pending_len = 0;
    size_t initial_decoded_len;
    size_t decoded_len = 0;
    char decoded_data[4096];
    int b64_file = 0;
    int qp_file = 0;
    if (strstr(buffer, "JVBE"))
    {
        initial_decoded_len = decode_64(decoded_data, sizeof(decoded_data), data_start, initial_response_data, &leftover);
        b64_file = 1;
        if (initial_decoded_len <= 0)
        {
            b64_file = 0;
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
        return 1;
    }
    if (leftover > 0)
    {
        memcpy(pending, data_start + initial_response_data - leftover, leftover);
        pending_len += leftover;
    }
    printf("Successfully decoded first block\n");

    // Check for download_size which is provided in the initial buffer response header
    
    if (initial_response_data > 0)
    {
        b64_file == 1 ? fwrite(decoded_data, 1, initial_decoded_len, f) : fwrite(data_start, 1, initial_decoded_len, f);
        downloaded_bytes += initial_response_data - leftover;
        printf("Downloading initial buffer attachment bytes\n");
        printf("Downloaded %d bytes of %d\n", downloaded_bytes, download_size);
        printf("Clearing the buffer\n");
        memset(decoded_data, 0, sizeof(decoded_data));
    }
    
    while (downloaded_bytes < download_size)
    {
        int remaining = download_size - downloaded_bytes;
        int to_read = (remaining < buffer_size - pending_len ? remaining : buffer_size - pending_len);
        int chunk_bytes_read = SSL_read(ssl, buffer, to_read);
        

        if (pending_len > 0)
        {
            memmove(buffer + pending_len, buffer, chunk_bytes_read);
            memcpy(buffer, pending, pending_len);
            chunk_bytes_read += pending_len;
            pending_len = 0;
        }
        if (b64_file == 1)
        {
            printf("Calling decode_64 from main loop\n");
            decoded_len = decode_64(decoded_data, sizeof(decoded_data), buffer, chunk_bytes_read, &leftover);
            if (decoded_len < 0)
            {
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
                break;
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
        char *eof = strstr(decoded_data, "%%EOF");
        if (eof)
        {
            decoded_len = eof + 5 - decoded_data;
            fwrite(decoded_data, 1, decoded_len, f);
            downloaded_bytes += decoded_len;
            break;   
        }

        b64_file == 1 ? fwrite(decoded_data, 1, decoded_len, f) : fwrite(buffer, 1, decoded_len, f);
        downloaded_bytes += chunk_bytes_read - leftover;
        memset(decoded_data, 0, sizeof(decoded_data));
    }
    printf("Downloaded %d bytes of %d\n", downloaded_bytes, download_size);
    fclose(f);
    printf("File closed\n");
    b64_file = 0;
    qp_file = 0;
    // const char *request_start = "tag LOGIN potter.jonathan.m@gmail.com evfqtsocoqdsmkww\r\n";
    return 0;
}

int receive_imap_response(SSL *ssl, char *tag)
{
    char buffer[4096];
    int bytes_read;
    
    while (1)
    {
        bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);

        if (bytes_read <= 0)
        {
            printf("0 bytes found\n");
            break;
        }
        
        buffer[bytes_read] = '\0';

        if(strstr((const char *)buffer, tag) == NULL)
        {
            if (strstr((const char *)buffer, "BAD"))
            {
                printf("%s\n", buffer);
                break;
            }
            printf("%s\n", buffer);
            continue;
        }
        
        if (strstr((const char *)buffer, tag))
        {
            // Conditions to handle file download (tag FETCH [ID] body[PART])
            // Response includes {RB_SIZE}
            // Parse that and store as download size
            if (strstr(tag, "(BODY"))
            {
                printf("Response received:\n%s\n", buffer);
                printf("Bytes: %d\n", bytes_read);
                char filename[128];
                printf("Name file>");
                if (fgets(filename, sizeof(filename), stdin) != NULL)
                {
                    filename[strcspn(filename, "\n")] = '\0';
                }
                printf("Beginning file creation\n");
                if (imap_download_attach(ssl, filename, buffer, sizeof(buffer), &bytes_read) != 0)
                {
                    break;
                }
            }
            printf("Response received:\n%s\n", buffer);
            printf("Bytes: %d\n", bytes_read);
            printf("done\n");
            break;
        }
    }
    return 0;
}

int imap_prompt_login(SSL *ssl)
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
        char imap_formatted_message[384];
        snprintf(imap_formatted_message, sizeof(imap_formatted_message), "a1 LOGIN %s %s\r\n", email_buffer, password_buffer);
        if (imap_send_request(ssl, imap_formatted_message) != 0)
        {
            perror("Message sending failure\n");
            continue;
        }
        return 0;
    }
}