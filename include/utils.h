#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

struct Connection_wrapper
{
    void *wrap_ssl_ctx;
    void *wrap_ssl;
    int (*wrap_ssl_read)(void *wrap_ssl, void *buffer, int len);
    int (*wrap_ssl_write)(void *wrap_ssl, const void *buffer, int len);
    int sockfd;
    void (*wrap_ssl_free)(void *wrap_ssl);
    void (*wrap_ssl_ctx_free)(void *wrap_ssl_ctx);
};


int wrapper_ssl_read(void *wrap_ssl, void *buf, int len);
int wrapper_ssl_write(void *wrap_ssl, const void *buf, int len);
void wrapper_ssl_free(void *wrap_ssl);
void wrapper_ssl_ctx_free(void *wrap_ssl_ctx);
int imap_send_request(void *wrap_ssl, char *request);
int imap_download_attach(void *wrap_ssl, char *filename, char *buffer, size_t buffer_size, int *bytes_read);
int receive_imap_response(void *wrap_ssl, char *tag);
int imap_prompt_login(void *wrap_ssl);
int imap_init_connection(struct Connection_wrapper *cwrap);

#endif