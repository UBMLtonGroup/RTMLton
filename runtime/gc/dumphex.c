
#include "dumphex.h"

#include <stdio.h>
#include <ctype.h>

void dump_hex(char *str, int len)
{
    int loop, l2, start;
    start = 1;
    str--;
    fprintf(stderr, "0x%8x ", (unsigned int)(str+1));
    for (loop=1;loop<=len;loop++)
    {
        fprintf(stderr, "0x%2.2X ", str[loop]&0xff);
        if (((loop%12) == 0) && loop)
        {
            fprintf(stderr, "|");
            for (l2 = start; l2 <= loop; l2++)
                if (isprint(str[l2]) && !iscntrl(str[l2]) && isascii(str[l2]))
                    fprintf(stderr, "%c", str[l2]);
                else
                    fprintf(stderr, ".");
            fprintf(stderr, "|\n");
            start = loop+1;
            fprintf(stderr, "0x%8x ", (unsigned int)(str+loop+1));
        }
    }
    if (((loop-1)%12) != 0)
    {
        fprintf(stderr, "|");
        for (l2 = start; l2 < len; l2++)
            if (isprint(str[l2]) && !iscntrl(str[l2]) && isascii(str[l2]))
                fprintf(stderr, "%c", str[l2]);
            else
                fprintf(stderr, ".");
        fprintf(stderr, "|\n");
        start = loop;
    }
}

