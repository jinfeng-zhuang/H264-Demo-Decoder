#include "h264.h"

/*
----------------------------------------------------------------
6.4.3 Inverse 4x4 luma block scanning process.

Figure 6-10: Scan for 4x4 luma blocks
----------------------------------------------------------------
    -----------------
    | 0 | 1 | 4 | 5 |
    -----------------
    | 2 | 3 | 6 | 7 |
    -----------------
    | 8 | 9 | 12| 13|
    -----------------
    | 10| 11| 14| 15|
    -----------------
*/

void Inverse_4x4_luma_block_scanning_process(const int luma4x4BlkIdx, int* x, int* y)
{
    // Formula (6-17)
    *x = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 0) +
        InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 0);

    // Formula (6-18)
    *y = InverseRasterScan(luma4x4BlkIdx / 4, 8, 8, 16, 1) +
        InverseRasterScan(luma4x4BlkIdx % 4, 4, 4, 8, 1);
}