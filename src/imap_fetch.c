#include "../include/utils.h"

int main() 
{
    int connect_imap = imap_init();
    
    if (connect_imap != 0)
    {
        perror("Failed init\n");
        return -1;
    }
return 0;
} 