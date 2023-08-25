#include "h264.h"

/*
Paritions:

 --------         --------        --------          --------
                      0                              0    1
    0                              0    1           
                      1                              2    3
 --------         --------        --------          --------
 */

// 6.4.2.1
void Inverse_macroblock_partition_scanning_process(
    int mb_type, int mbPartIdx,
    int *x, int *y)
{
    *x = InverseRasterScan(mbPartIdx, MbPartWidth(mb_type), MbPartHeight(mb_type), 16, 0);
    *y = InverseRasterScan(mbPartIdx, MbPartWidth(mb_type), MbPartHeight(mb_type), 16, 1);
}

// 6.4.2.2
// TODO, do the same thing of '6.4.2.1'
void Inverse_sub_macroblock_partition_scanning_process(int mbPartIdx, int subMbPartIdx, int* x, int* y)
{
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];

    if ((P_8x8 == MB->mb_type) || (P_8x8ref0 == MB->mb_type) || (B_8x8 == MB->mb_type)) {
        *x = InverseRasterScan(subMbPartIdx,
            SubMbPartWidth(MB->sub_mb_type[mbPartIdx]),
            SubMbPartHeight(MB->sub_mb_type[mbPartIdx]),
            8, 0);
        *y = InverseRasterScan(subMbPartIdx,
            SubMbPartWidth(MB->sub_mb_type[mbPartIdx]),
            SubMbPartHeight(MB->sub_mb_type[mbPartIdx]),
            8, 1);
    }
    else {
        *x = InverseRasterScan(subMbPartIdx, 4, 4, 8, 0);
        *y = InverseRasterScan(subMbPartIdx, 4, 4, 8, 1);
    }
}