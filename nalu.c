#include <string.h>

int is_nalu_prefix(unsigned char* buf)
{
    if ((buf[0] == 0) && (buf[1] == 0) && (buf[2] == 1))
        return 1;
    if ((buf[0] == 0) && (buf[1] == 0) && (buf[2] == 0) && (buf[3] == 1))
        return 1;
    return 0;
}

int nalu_read(unsigned char* raw, unsigned char* nalu)
{
    unsigned char* p1 = raw;
    unsigned char* p2;

    while (!is_nalu_prefix(p1))
        p1++;

    p2 = p1 + 3;

    while (!is_nalu_prefix(p2))
        p2++;

    memcpy(nalu, p1, p2 - p1);

    return (int)(p2 - p1);
}
