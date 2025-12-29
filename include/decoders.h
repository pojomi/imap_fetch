#ifndef DECODERS_H
#define DECODERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int hexval(char c);
int get_b64_value(char c);
size_t decode_64(char *d, size_t d_len, char *s, size_t s_len, size_t *leftover);
size_t decode_qp(char *s, size_t s_len, size_t *leftover);

#endif