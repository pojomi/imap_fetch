#ifndef DECODERS_H
#define DECODERS_H

#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int hexval(char c);
int get_b64_value(char c);
size_t decode_64(Str buff_d, Str buff_s, size_t *leftover);
size_t decode_qp(Str buff_d, size_t *leftover);

#endif