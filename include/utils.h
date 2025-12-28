#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>

int hexval(char c);
int get_b64_value(char c);
size_t decode_64(char *d, size_t d_len, char *s, size_t s_len, size_t *leftover);
size_t decode_qp(char *s, size_t s_len, size_t *leftover);
int imap_send_request(SSL *ssl, char *request);
int imap_download_attach(SSL *ssl, char *filename, char *buffer, size_t buffer_size, int *bytes_read);
int receive_imap_response(SSL *ssl, char *tag);
int imap_prompt_login(SSL *ssl);


#endif