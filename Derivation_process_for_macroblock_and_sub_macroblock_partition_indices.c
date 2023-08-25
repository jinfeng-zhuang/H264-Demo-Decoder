#include "h264.h"

/*
 * 6.4.13.4: Derivation process for macroblock and sub-macroblock partition indices

             ┌──────────┬─────┬─────┐
             │          │     │     │
             │          │     │     │
             ├──────────┼─────┼─────┤
             │    xP,yP |     │     │
             │          │     │     │
             ├────┬─────┼─────┼─────┤
             │    │     │     │     │
             │    │     │     │     │
             │    │     │     │     │
             │    │     │     │     │
             └────┴─────┴─────┴─────┘
*/

void Derivation_process_for_macroblock_and_sub_macroblock_partition_indices(
    int xP, int yP,
    int mbType,
    int subMbType[4],
    int *mbPartIdx, int *subMbPartIdx)
{
    // mbPartIdx
    if ((mbType >= I_NxN) && (mbType <= I_PCM)) {
        *mbPartIdx = 0;
    }
    else {
        *mbPartIdx = (16 / MbPartWidth(mbType)) * (yP / MbPartHeight(mbType)) +
            (xP / MbPartWidth(mbType)); // (6-41)
    }

    // subMbPartIdx
    if ((mbType != P_8x8) || (mbType != P_8x8ref0) || (P_8x8ref0 != mbType) ||
        (mbType != B_Skip) || (mbType != B_Direct_16x16)) {
        *subMbPartIdx = 0;
    }
    else if ((mbType == B_Skip) || (mbType == B_Direct_16x16)) {
        *subMbPartIdx = 2 * ((yP % 8) / 4) + ((xP % 8) / 4);
    }
    else {
        *subMbPartIdx = (8 / SubMbPartWidth(subMbType[*mbPartIdx])) *
            (( yP % 8) / SubMbPartHeight(subMbType[*mbPartIdx])) +
            (( xP % 8) / SubMbPartWidth(subMbType[*mbPartIdx]));
    }
}