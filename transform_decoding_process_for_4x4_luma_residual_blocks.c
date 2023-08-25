#include "h264.h"

// 8.5.1 Specification of transform decoding process for 4x4 luma residual blocks.
void transform_decoding_process_for_4x4_luma_residual_blocks(int luma4x4BlkIdx)
{
    int coeff_matrix[4][4];
    int dequant_matrix[4][4];
    int residual_matrix[4][4];
    int construct_matrix[4][4];
    int xO = -1, yO = -1;
    int i = 0, j = 0;

    // ZigZag
    Inverse_scanning_process_for_4x4_transform(Slice.MB[Slice.CurrMbAddr].level4x4[luma4x4BlkIdx], coeff_matrix);

    // Dequant
    Scaling_process_for_residual_4x4_blocks(Slice.MB[Slice.CurrMbAddr].QPY, Component_Y, coeff_matrix, dequant_matrix);

#ifdef DEBUG
    printf("\nDequant:\n");
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            printf("%8d ", dequant_matrix[i][j]);
        }
        printf("\n");
    }
#endif

    // IDCT
    Transformation_process_for_residual_4x4_blocks(dequant_matrix, residual_matrix);

    // Inverse Scan: Copy MB to SliceData
    // 0  1  4  5
    // 2  3  6  7
    // 8  9  12 13
    // 10 11 14 15
    Inverse_4x4_luma_block_scanning_process(luma4x4BlkIdx, &xO, &yO);

    // Save the residual matrix for debug
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            Slice.MB[Slice.CurrMbAddr].residual[yO + i][xO + j] = residual_matrix[i][j];

    // image = pred + residual
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            construct_matrix[i][j] = Clip1Y8((int)Slice.MB[Slice.CurrMbAddr].predL[xO + j][yO + i] + residual_matrix[i][j]);

    // 8.5.14: Picture construction process prior to deblocking filter process
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            Slice.MB[Slice.CurrMbAddr].SprimeL[xO + j][yO + i] = (unsigned char)construct_matrix[i][j];
        }
    }

#if 0
    printf("\nResidual:\n");
    printf("---------------------------\n");
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            printf("%8d ", Slice.MB[Slice.CurrMbAddr].residual[xO + j][yO + i]);
        }
        printf("\n");
    }
#endif

#if 0
    printf("\nPre-LoopFilter:\n");
    printf("---------------------------\n");
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            printf("%8d ", Slice.MB[Slice.CurrMbAddr].SprimeL[xO + j][yO + i]);
        }
        printf("\n");
    }
#endif
}