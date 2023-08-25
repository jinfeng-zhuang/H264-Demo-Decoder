#include "h264.h"

static const int raster_4x4_2d[16][2] =
{
    {0,0}, {0,1}, {1,0}, {1,1},
    {0,2}, {0,3}, {1,2}, {1,3},
    {2,0}, {2,1}, {3,0}, {3,1},
    {2,2}, {2,3}, {3,2}, {3,3}
};

void transform_decoding_process_for_luma_samples_of_Intra_16x16_macroblock_prediction_mode(void)
{
    int dc_sort[4][4];
    int dc_inverse[4][4];
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];
    int luma4x4BlkIdx = 0;
    int luma4x4[16];
    int luma4x4_sort[4][4];
    int luma4x4_dequant[4][4];
    int luma4x4_inverse[4][4];
    int luma16x16_residual[16][16];
    int luma16x16[16][16];
    int i, j;
    int xO = -1, yO = -1;

    // DC
    Inverse_scanning_process_for_4x4_transform(MB->i16x16DClevel, dc_sort);

    Scaling_and_transformation_process_for_DC_transform_coefficients_for_Intra_16x16_macroblock_type(dc_sort, dc_inverse);

    // AC
    for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++) {

        /*---------------
        1 2     5 6
        3 4     7 8
        ...
        ...
        ---------------*/
        for (i = 1; i < 16; i++) {
            luma4x4[i] = MB->i16x16AClevel[luma4x4BlkIdx][i - 1];
        }

        Inverse_scanning_process_for_4x4_transform(luma4x4, luma4x4_sort);

        Scaling_process_for_residual_4x4_blocks(MB->qP, 0, luma4x4_sort, luma4x4_dequant);

        // Copy DC after dequant
        luma4x4_dequant[0][0] = dc_inverse[raster_4x4_2d[luma4x4BlkIdx][0]][raster_4x4_2d[luma4x4BlkIdx][1]];

#ifdef DEBUG
        printf("\nluma 16x16's 4x4 dequant block:\n");
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                printf("%4d ", luma4x4_dequant[i][j]);
            }
            printf("\n");
        }
#endif

        // IDCT
        Transformation_process_for_residual_4x4_blocks(luma4x4_dequant, luma4x4_inverse);

        // RasterScan Position
        Inverse_4x4_luma_block_scanning_process(luma4x4BlkIdx, &xO, &yO);

        // Copy to 16x16 MB Residual Matrix
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                luma16x16_residual[xO + i][yO + j] = luma4x4_inverse[j][i];
            }
        }
    }

#ifdef DEBUG
    // Dump luma residual data
    printf("\nLuma 16x16 Residual:\n");
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            printf("%4d ", luma16x16_residual[j][i]);
        }
        printf("\n");
    }
#endif

    // Bypass Check: Ignore

    // Prediction + Residual
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            luma16x16[i][j] = luma16x16_residual[j][i] + MB->predL[j][i];
        }
    }

    // Copy to MB luma area
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            MB->SprimeL[j][i] = luma16x16[i][j];
        }
    }
}