#include "h264.h" 

/*
 * 6.4.11.7 Derivation process for neighbouring partitions
 *
 * Input:
 *   mbPartIdx = 1 (which sub-block)
 *   currSubMbType = P_4x4 (sub-block layout)
 *   subMbPartIdx = 2 (which one in layout)
 *
 * Output:
 *   mbAddrA = ? mbPartIdxA = ? subMbPartIdxA = ?
 *   mbAddrB = ? mbPartIdxB = ? subMbPartIdxB = ?
 *
             ┌──────────┬─────┬─────┐
             │          │     │     │
             │          │     │     │
             ├──────────┼─────┼─────┤
             │          │     │     │
             │          │     │     │
             ├────┬─────┼─────┼─────┤
             │    │     │     │     │
             │    │     │     │     │
             │    │     │ xN  │     │
             │    │     │ xW  │     │
             └────┴─────┴─────┴─────┘
                      (x,y)
             ┌──────────┬─────┬─────┐
             │          │     │     │
             │          │ xD  │     │
             ├──────────┼─────┼─────┤
             │       xD │xS*  │     │
             │          │     │     │
             ├────┬─────┼─────┼─────┤
             │    │     │     │     │
             │    │     │     │     │
             │    │     │     │     │
             │    │     │     │     │
             └────┴─────┴─────┴─────┘
*/
void Derivation_process_for_neighbouring_partitions(
    int mbPartIdx,                                      // Input
    int currSubMbType,                                  // Input
    int subMbPartIdx,                                   // Input
    int *mbAddrA, int *mbPartIdxA, int *subMbPartIdxA,  // Output
    int *mbAddrB, int *mbPartIdxB, int *subMbPartIdxB,  // Output
    int *mbAddrC, int *mbPartIdxC, int *subMbPartIdxC,  // Output
    int *mbAddrD, int *mbPartIdxD, int *subMbPartIdxD   // Output
    )
{
    int mb_type;
    int x, y;
    int xS, yS;
    int xD, yD;
    int xN, yN;
    int xW, yW;
    int mbTypeN;
    int subMbTypeN[4];
    int mbAddrN, mbPartIdxN, subMbPartIdxN;
    int i;
    int predPartWidth;
    struct MacroBlock_t *MB = &Slice.MB[Slice.CurrMbAddr];
    
    mb_type = MB->mb_type;

    // Step 1: Current mbPartIdx location: (x, y)
    Inverse_macroblock_partition_scanning_process(mb_type, mbPartIdx, &x, &y);

    // Step 2: Current subMbPartIdx location: (xS, yS)
    if ((mb_type == P_8x8) || (mb_type == P_8x8ref0) || (mb_type == B_8x8)) {
        Inverse_sub_macroblock_partition_scanning_process(mbPartIdx, subMbPartIdx, &xS, &yS);
    }
    else {
        xS = 0;
        yS = 0;
    }

    // Step 3: Determine predPartWidth
    if ((mb_type == P_Skip) || (mb_type == B_Skip) || (mb_type == B_Direct_16x16)) {
        predPartWidth = 16;
    }
    else if (mb_type == B_8x8) {
        if (currSubMbType == B_Direct_8x8) {
            predPartWidth = 16;
            if (Slice.direct_spatial_mv_pred_flag == 0) {
                printf("TODO\n"); // TODO, see 'NOTE 1'
            }
        }
        else {
            predPartWidth = SubMbPartWidth(MB->sub_mb_type[mbPartIdx]);
        }
    }
    else if ((mb_type == P_8x8) || (mb_type == P_8x8ref0)) {
        predPartWidth = SubMbPartWidth(MB->sub_mb_type[ mbPartIdx ]);
    }
    else {
        predPartWidth = MbPartWidth(mb_type);
    }
    
    for (i = 0; i < 4; i++) {

        // Step 4: diff (xD, yD): Table 6-2
        switch (i) {
        case 0:
            xD = -1;
            yD = 0;
            break;
        case 1:
            xD = 0;
            yD = -1;
            break;
        case 2:
            xD = predPartWidth;
            yD = -1;
            break;
        case 3:
            xD = -1;
            yD = -1;
            break;
        }

        // Step 5: Determine (xN, yN)
        xN = x + xS + xD;
        yN = y + yS + yD;

        // Step 6: Derivation neighbour's location(based on xN, yN)
        Derivation_process_for_neighbouring_locations(1, xN, yN, &mbAddrN, &xW, &yW);

        // Step 7: Derivation neighbour's index based on the location
        if (-1 == mbAddrN) {
            mbAddrN = -1;
            mbPartIdxN = -1;
            subMbPartIdxN = -1;
        }
        else {
            // Step 7.a
            mbTypeN = Slice.MB[mbAddrN].mb_type;
            if ((mbTypeN == P_8x8) || (mbTypeN == P_8x8ref0) || (mbTypeN == B_8x8)) {
                memcpy(subMbTypeN, Slice.MB[mbAddrN].sub_mb_type, sizeof(subMbTypeN));
            }

            // Step 7.b: Ref to 6.4.13.4
            Derivation_process_for_macroblock_and_sub_macroblock_partition_indices(
                xW, yW,
                mbTypeN,
                subMbTypeN,
                &mbPartIdxN, &subMbPartIdxN);
        }

        switch (i) {
        case 0:
            *mbAddrA = mbAddrN;
            *mbPartIdxA = mbPartIdxN;
            *subMbPartIdxA = subMbPartIdxN;
            break;
        case 1:
            *mbAddrB = mbAddrN;
            *mbPartIdxB = mbPartIdxN;
            *subMbPartIdxB = subMbPartIdxN;
            break;
        case 2:
            *mbAddrC = mbAddrN;
            *mbPartIdxC = mbPartIdxN;
            *subMbPartIdxC = subMbPartIdxN;
            break;
        case 3:
            *mbAddrD = mbAddrN;
            *mbPartIdxD = mbPartIdxN;
            *subMbPartIdxD = subMbPartIdxN;
            break;
        }
    }
}