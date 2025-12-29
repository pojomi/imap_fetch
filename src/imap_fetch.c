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

int main() {
    
    SSL_library_init();
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL *ssl = SSL_new(ctx);

    const char *hostname = "imap.gmail.com";
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    SSL_set_fd(ssl, sock);
    struct sockaddr_in host;
    struct hostent *he = gethostbyname("imap.gmail.com");
    memcpy(&host.sin_addr, he->h_addr_list[0], he->h_length);
    host.sin_family = AF_INET;
    host.sin_port = htons(993);

    if (connect(sock, (const struct sockaddr *)&host, sizeof(host)) < 0)
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

    if (receive_imap_response(ssl, "ready for requests") == 1)
    {
        perror("No initial success message\n");
        exit(EXIT_FAILURE);
    }
    
    int logged_in = 0;

    while (1)
    {
        if (logged_in == 0)
        {
            if (imap_prompt_login(ssl) == 1)
            {
                continue;
            }

            if (receive_imap_response(ssl, "Success") == 1)
            {
                printf("Failed login\n");
                continue;
            }
            logged_in = 1;
        }
        
        printf("IMAP>"); 
        char input_buffer[256];
        
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL)
        {
            continue;
        }

        
        input_buffer[strcspn(input_buffer, "\n")] = '\0';
        strcat(input_buffer, "\r\n");
        
        if (imap_send_request(ssl, input_buffer) == 1)
        {
            continue;
        }
        
        if (strstr(input_buffer, "body["))
        {
            printf("Looking for response\n");
            
            if (receive_imap_response(ssl, "(BODY") == 1)
            {
                continue;
            }
            
            memset(input_buffer, 0, sizeof(input_buffer));
            break;
        }
        
        memset(input_buffer, 0, sizeof(input_buffer));
        printf("Looking for response\n");
        if (receive_imap_response(ssl, "Success") == 1)
        {
            printf("Unsuccessful action\n");
            continue;
        }   
        
        // Good note to remember: use stdin macro == FILE *stdin
        // stdin is the predefined standard input stream for any program
        // Cheat for GMAIL command: tag SEARCH x-gm-raw "from: has:"
        
        // Create buffer for file writing
        
    }
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sock);
    return 0;
} 