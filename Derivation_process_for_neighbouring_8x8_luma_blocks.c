#include "h264.h"

// 6.4.11.2 Derivation process for neighbouring 8x8 luma blocks.
void Derivation_process_for_neighbouring_8x8_luma_blocks(
    const int luma8x8BlkIdx,
    int* mbAddrA, int* luma8x8BlkIdxA,
    int* mbAddrB, int* luma8x8BlkIdxB)
{
    int xA = (luma8x8BlkIdx % 2) * 8 - 1;
    int yA = (luma8x8BlkIdx / 2) * 8;

    int xB = (luma8x8BlkIdx % 2) * 8;
    int yB = (luma8x8BlkIdx / 2) * 8 - 1;

    int xW = 0, yW = 0;

    Derivation_process_for_neighbouring_locations(1, xA, yA, mbAddrA, &xW, &yW);
    if (*mbAddrA > -1)
        *luma8x8BlkIdxA = Derivation_process_for_8x8_luma_block_indices(xW, yW);
    else
        *luma8x8BlkIdxA = -1;

    Derivation_process_for_neighbouring_locations(1, xB, yB, mbAddrB, &xW, &yW);
    if (*mbAddrB > -1)
        *luma8x8BlkIdxB = Derivation_process_for_8x8_luma_block_indices(xW, yW);
    else
        *luma8x8BlkIdxB = -1;
}