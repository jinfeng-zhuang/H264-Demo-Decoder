#include "h264.h"

extern int slice_base;

void bitstream_status(void)
{
    //printf("Bitstream: %#X:%d\n", slice_base + (H264.bs.bitpos >> 3), H264.bs.bitpos & 0x7);
}

void bitstream_init(unsigned char* buf, unsigned int len)
{
    H264.bs.buf = buf;
    H264.bs.buflen = len;
    H264.bs.bitpos = 0;
}

int bitstream_is_align(void)
{
    return (H264.bs.bitpos & 0x7) ? 0 : 1;
}

unsigned int bitstream_read(int count)
{
    int idx;
    int offset;
    int bit;
    unsigned int value = 0;
    
    assert(count <= 32);

    while (count--) {
        idx = H264.bs.bitpos >> 3;
        offset = H264.bs.bitpos & 0x7;

        bit = (H264.bs.buf[idx] & (1 << (7 - offset))) ? 1 : 0;
        value <<= 1;
        value += bit;

        H264.bs.bitpos++;
    }

    return value;
}