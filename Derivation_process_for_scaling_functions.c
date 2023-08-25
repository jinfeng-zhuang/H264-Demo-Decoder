#include "h264.h"

// 8.5.9: Derivation process for scaling functions
static void Init_normAdjust4x4(void)
{
    int q, i, j;

    // 8-315
    int v4x4[6][3] =
    {
        {10, 16, 13},
        {11, 18, 14},
        {13, 20, 16},
        {14, 23, 18},
        {16, 25, 20},
        {18, 29, 23},
    };

    // 8-314
    for (q = 0; q < 6; q++) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                if ((i % 2 == 0) && (j % 2 == 0))
                    H264.normAdjust4x4[q][i][j] = v4x4[q][0];
                else if ((i % 2 == 1) && (j % 2 == 1))
                    H264.normAdjust4x4[q][i][j] = v4x4[q][1];
                else
                    H264.normAdjust4x4[q][i][j] = v4x4[q][2];
            }
        }
    }
}

// Called in SPS
// 8-313
void Derivation_process_for_scaling_functions(void)
{
    int YCbCr;
    int q;
    int i, j;

    Init_normAdjust4x4();

    for (YCbCr = 0; YCbCr < 3; YCbCr++)
        for (q = 0; q < 6; q++)
            for (i = 0; i < 4; i++)
                for (j = 0; j < 4; j++)
                    SPS.LevelScale4x4[YCbCr][q][i][j] = SPS.ScalingMatrix4x4[YCbCr][i][j] * H264.normAdjust4x4[q][i][j];
}