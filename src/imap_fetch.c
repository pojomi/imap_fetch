#include "../include/utils.h"

int main() {

    struct Connection_wrapper *cwrap;
    cwrap = calloc(1, sizeof(*cwrap));
    
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

            if (receive_imap_response(cwrap->wrap_ssl, STR_LIT_PTR("Success")) != 0)
            {
                printf("Failed login\n");
                continue;
            }
            logged_in = 1;
        }
        
        printf("IMAP>");
        Str input = {0};
        input.data = calloc(256, sizeof(char));
        input.len = 256;
        
        if (fgets(input.data, input.len, stdin) == NULL)
        {
            continue;
        }

        size_t offset = find_offset(input, STR_LIT("\n"));
        memmove(input.data + offset + 1, input.data + offset, 1);
        input.data[offset] = '\r';

        if (imap_send_request(cwrap->wrap_ssl, input.data) == 1)
        {
            continue;
        }
        
        Str user_submit_response = find_substring(input, STR_LIT("body["));
        if (user_submit_response.data != NULL)
        {
            printf("Looking for download response\n");
            
            if (receive_imap_response(cwrap->wrap_ssl, STR_LIT_PTR("(BODY")) == 1)
            {
                continue;
            }
            
            memset(input.data, 0, input.len);
            break;
        }
        
        // memset(input.data, 0, input.len);
        free(input.data);
        printf("Looking for input submission response\n");
        int submission_response = receive_imap_response(cwrap->wrap_ssl, STR_LIT_PTR("Success"));
        if (submission_response != 0)
        {
            printf("Unsuccessful action\n");
            continue;
        }   
        
        // Good note to remember: use stdin macro == FILE *stdin
        // stdin is the predefined standard input stream for any program
        // Cheat for GMAIL command: tag SEARCH x-gm-raw "from: has:"
    }
    wrapper_ssl_free(cwrap->wrap_ssl);
    wrapper_ssl_ctx_free(cwrap->wrap_ssl_ctx);
    close(cwrap->sockfd);
    free(cwrap);
    return 0;
} 