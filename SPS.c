#include "h264.h"

struct SPS_t SPS;

/*
 * We don't need to parse, just copy from H264Visa
 */

void SPS_parse(unsigned char* buf)
{
    int q,i,j;

    SPS.BitDepthY = 8;
    SPS.log2_max_frame_num_minus4 = 0;
    SPS.log2_max_pic_order_cnt_lsb_minus4 = 2;
    SPS.frame_mbs_only_flag = 1;

    // TODO
    if (SPS.frame_mbs_only_flag == 0) {
        SPS.direct_8x8_inference_flag = 1;
    }
    SPS.direct_8x8_inference_flag = 1;

    // Table 6-1: SubWidthC, and SubHeightC values derived
    SPS.SubWidthC = 2;
    SPS.SubHeightC = 2;
    SPS.NumC8x8 = 4 / (SPS.SubWidthC * SPS.SubHeightC);

    SPS.MbWidthC = 16 / SPS.SubWidthC;
    SPS.MbHeightC = 16 / SPS.SubHeightC;

    SPS.pic_width_in_mbs_minus1 = 119;
    SPS.PicWidthInMbs = SPS.pic_width_in_mbs_minus1 + 1;

    SPS.width = (SPS.pic_width_in_mbs_minus1 + 1) * 16;
    SPS.height = 1088;

    // Default Value: YUV420
    SPS.chroma_format_idc = 1;

    if (0 == SPS.separate_colour_plane_flag) {
        SPS.ChromaArrayType = SPS.chroma_format_idc;
    }
    else {
        SPS.ChromaArrayType = 0;
    }
    
    /* 8.5.9: Derivation process for scaling functions
     * 
     * (8-313):
     * LevelScale4x4(m, i, j) = weightScale4x4(i, j) * normAdjust4x4(m, i, j)
     */
    for (q = 0; q < 6; q++)
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                SPS.ScalingMatrix4x4[q][i][j] = 16;

    Derivation_process_for_scaling_functions();
}