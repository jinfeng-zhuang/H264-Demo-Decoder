#include "h264.h"

/*
-----------------------------------------------------------------------------------------------
ITU-T H.264: 6.4.11.5

Figure 6-14: Determination of the neighbouring macroblock, blocks, and partitions (informative)
-----------------------------------------------------------------------------------------------
    D       B           C
    A       (Current)

Table 6-2: Specification of input and output assignments for subclauses 6.4.11.1 to 6.4.11.7
-----------------------------------------------------------------------------------------------
    N      xD       yD
    A      -1        0
    B       0       -1
    C predPartWidth -1
    D      -1       -1
-----------------------------------------------------------------------------------------------
*/
void Derivation_process_for_neighbouring_4x4_chroma_blocks(
    const int luma4x4BlkIdx,
    int* mbAddrA, int* luma4x4BlkIdxA,
    int* mbAddrB, int* luma4x4BlkIdxB
    )
{
    int x = 0, y = 0;
    Inverse_4x4_luma_block_scanning_process(luma4x4BlkIdx, &x, &y);

    int xA = x - 1;
    int yA = y;

    int xB = x;
    int yB = y - 1;

    int xW = 0, yW = 0;

    // luma4x4BlkIdxA derivation
    Derivation_process_for_neighbouring_locations(0, xA, yA, mbAddrA, &xW, &yW);
    if (*mbAddrA > -1)
    {
        *luma4x4BlkIdxA = Derivation_process_for_4x4_luma_block_indices(xW, yW);
    }
    else
    {
        *luma4x4BlkIdxA = -1;
    }

    // luma4x4BlkIdxB derivation
    Derivation_process_for_neighbouring_locations(0, xB, yB, mbAddrB, &xW, &yW);
    if (*mbAddrB > -1)
    {
        *luma4x4BlkIdxB = Derivation_process_for_4x4_luma_block_indices(xW, yW);
    }
    else
    {
        *luma4x4BlkIdxB = -1;
    }
}