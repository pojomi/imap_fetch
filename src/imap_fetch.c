#include "../include/utils.h"

int main() {

    struct Connection_wrapper *cwrap;
    cwrap = calloc(7, sizeof(*cwrap));
    
    if (imap_init_connection(cwrap) != 0)
    {
        printf("Failed to initialize\n");
        return -1;
    }
    int logged_in = 0;

    while (1)
    {
        if (logged_in == 0)
        {
            if (imap_prompt_login(cwrap->wrap_ssl) == 1)
            {
                continue;
            }

            if (receive_imap_response(cwrap->wrap_ssl, "Success") == 1)
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
        
        if (imap_send_request(cwrap->wrap_ssl, input_buffer) == 1)
        {
            continue;
        }
        
        if (strstr(input_buffer, "body["))
        {
            printf("Looking for response\n");
            
            if (receive_imap_response(cwrap->wrap_ssl, "(BODY") == 1)
            {
                continue;
            }
            
            memset(input_buffer, 0, sizeof(input_buffer));
            break;
        }
        
        memset(input_buffer, 0, sizeof(input_buffer));
        printf("Looking for response\n");
        if (receive_imap_response(cwrap->wrap_ssl, "Success") == 1)
        {
            printf("Unsuccessful action\n");
            continue;
        }   
        
        // Good note to remember: use stdin macro == FILE *stdin
        // stdin is the predefined standard input stream for any program
        // Cheat for GMAIL command: tag SEARCH x-gm-raw "from: has:"
        
        // Create buffer for file writing
        
    }
    wrapper_ssl_free(cwrap->wrap_ssl);
    wrapper_ssl_ctx_free(cwrap->wrap_ssl_ctx);
    close(cwrap->sockfd);
    free(cwrap);
    return 0;
} 