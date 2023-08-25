#include "h264.h"
#include <math.h>

// 8.5.12.1
// https://blog.csdn.net/qq_42139383/article/details/118334630
void Scaling_process_for_residual_4x4_blocks(int qp, int YCbCr, const int coeff[4][4], int dequant[4][4])
{
    int i, j;
    int qPmod6 = qp % 6;
    int qPdiv6 = qp / 6;
    int mypow;

    if (qp >= 24) {
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                dequant[i][j] = (coeff[i][j] * SPS.LevelScale4x4[YCbCr][qPmod6][i][j]) << (qPdiv6 - 4);
    }
    else {
        mypow = (int)pow(2, 3 - qPdiv6);

        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                dequant[i][j] = (coeff[i][j] * SPS.LevelScale4x4[YCbCr][qPmod6][i][j] + mypow) >> (4 - qPdiv6);
    }
}