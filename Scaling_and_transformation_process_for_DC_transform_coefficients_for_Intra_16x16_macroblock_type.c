#include "h264.h"

const int idct_dccoeff_4x4[4][4] =
{
    { 1, 1, 1, 1},
    { 1, 1,-1,-1},
    { 1,-1,-1, 1},
    { 1,-1, 1,-1}
};

void Scaling_and_transformation_process_for_DC_transform_coefficients_for_Intra_16x16_macroblock_type(
    int c[4][4], int dcY[4][4])
{
    int i, j, k;
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];
    int qP = MB->qP;
    int qPmod6 = qP % 6;
    int qPdiv6 = qP / 6;

    if (MB->TransformBypassModeFlag) {
        // TODO
    }
    else {
        int f1[4][4] = { {0} };
        int f2[4][4] = { {0} };

        // Transform - round 1
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                for (k = 0; k < 4; k++)
                    f1[i][j] = idct_dccoeff_4x4[i][k] * c[k][j] + f1[i][j];

        // Transform - round 2
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                for (k = 0; k < 4; k++)
                    f2[i][j] = f1[i][k] * idct_dccoeff_4x4[k][j] + f2[i][j];

        // Quantization
        if (qP > 36)
        {
            for (i = 0; i < 4; i++)
                for (j = 0; j < 4; j++)
                    dcY[i][j] = (f2[i][j] * SPS.LevelScale4x4[0][qPmod6][0][0]) << (qPdiv6 - 6);
        }
        else
        {
            for (i = 0; i < 4; i++)
                for (j = 0; j < 4; j++)
                    dcY[i][j] = (f2[i][j] * SPS.LevelScale4x4[0][qPmod6][0][0] + (1 << (5 - qPdiv6))) >> (6 - qPdiv6);
        }

#ifdef DEBUG
        // Dump Residual
        printf("-------------------------------\n");
        printf("DC:\n");
        for (i = 0; i < 4; i++)
            printf("%-8d %-8d %-8d %-8d\n", dcY[i][0], dcY[i][1], dcY[i][2], dcY[i][3]);
#endif
    }
}