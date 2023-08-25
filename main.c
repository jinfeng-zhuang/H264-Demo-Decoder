/*
 * This project decode 3 frames: I,P,B
 * 
 */

#include <stdio.h>
#include <assert.h>
#include "h264.h"

unsigned char Luma[SLICE_SIZE];
unsigned char Luma_Pad[HEIGHT_PAD][WIDTH_PAD];
int slice_base = 0;
int nalu_type;
int nal_ref_idc;

static unsigned char h264_data[1<<20];
static unsigned char nalu_data[1<<20];

void file_read(void)
{
    FILE* fp = fopen("test.h264", "rb");
    assert(fp != NULL);

    fread(h264_data, 1, 1 << 20, fp);
    fclose(fp);
}

int main(void)
{
    int nalu_len;
    int nalu_header_size;
    int rp = 0;

    printf("Slice Size = %lld MB\n", sizeof(Slice) / 1024 / 1024);
    printf("Slice.MB Size = %lld MB\n", sizeof(Slice.MB) / 1024 / 1024);
    printf("Slice.MB[0] Size = %lld KB\n", sizeof(Slice.MB[0]) / 1024);

    file_read();

    while (1) {
        // I Frame
        slice_base = rp;
        nalu_len = nalu_read(&h264_data[rp], nalu_data);
        rp += nalu_len;

        if (0x01 == nalu_data[2]) {
            nalu_header_size = 4;
            nalu_type = nalu_data[3] & 0x1F;
            nal_ref_idc = (nalu_data[3] >> 5) & 0x3;
        }
        else {
            nalu_header_size = 5;
            nalu_type = nalu_data[4] & 0x1F;
            nal_ref_idc = (nalu_data[4] >> 5) & 0x3;
        }

        slice_base += nalu_header_size;

        switch (nalu_type) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            slice_parse(&nalu_data[nalu_header_size], nalu_len);
            break;
        case NALU_SPS:
            SPS_parse(&nalu_data[nalu_header_size]);
            break;
        case NALU_PPS:
            PPS_parse(&nalu_data[nalu_header_size]);
            break;
        default:
            break;
        }
    }

    return 0;
}