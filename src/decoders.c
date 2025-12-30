#include "../include/decoders.h"

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

size_t decode_qp(char *data, size_t s_len, size_t *leftover)
{
    // Read position
    size_t i = 0;
    // Write position
    size_t j = 0;
    *leftover = 0;
    
    while (i < s_len)
    {
        if (data[i] == '=')
        {
            if (i + 2 >= s_len)
            {
                *leftover = s_len - i;
                break;
            }
            // For \r, disregard
            if (i + 2 < s_len && data[i + 1] == '\r' && data[i + 2] == '\n')
            {
                i += 3;
                continue;
            }
            // Hex conversion
            // Verify that there are 2 bytes after index position
            // For \n, disregard
            if (i + 2 < s_len && data[i + 1] == '\n')
            {
                // Only increment by 2 since there's only 2 characters here
                i += 2;
                continue;
            }
            if (i + 2 < s_len)
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