#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define STR_LIT(s) (Str){s, sizeof(s) - 1}
#define STR_LIT_PTR(s) (Str *) &(Str){s, sizeof(s) - 1}

// Uses memcmp to find memory of needle in haystack - Returns pointer to first byte in needle or NULL
char *my_memmem(const char *haystack, size_t hay_len, const char *needle, size_t needle_len);
// Returns the address needle's location in haystack
int find_offset(Str haystack, Str needle);
// Returns struct Str {needle.data, needle.len} found in Str {haystack.data}
Str find_substring(Str haystack, Str needle);
size_t wrapper_ssl_read(void *wrap_ssl, void *buf, size_t len);
size_t wrapper_ssl_write(void *wrap_ssl, const void *buf, size_t len);
void wrapper_ssl_free(void *wrap_ssl);
void wrapper_ssl_ctx_free(void *wrap_ssl_ctx);
int imap_send_request(void *wrap_ssl, char *request);
int imap_download_attach(void *wrap_ssl, char *filename, Str buff, size_t bytes_read);
int receive_imap_response(void *wrap_ssl, Str *tag);
int imap_prompt_login(void *wrap_ssl);
int imap_init_connection(struct Connection_wrapper *cwrap);

#endif