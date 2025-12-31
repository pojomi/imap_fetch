#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>

struct Connection_wrapper
{
    void *wrap_ssl_ctx;
    void *wrap_ssl;
    size_t (*wrap_ssl_read)(void *wrap_ssl, void *buffer, size_t len);
    size_t (*wrap_ssl_write)(void *wrap_ssl, const void *buffer, size_t len);
    int sockfd;
    void (*wrap_ssl_free)(void *wrap_ssl);
    void (*wrap_ssl_ctx_free)(void *wrap_ssl_ctx);
};

typedef struct
{
    char *data;
    size_t len;

} Str;

typedef struct
{
    char *data;
    size_t capacity;
    size_t len;
} buffer;

#endif