#include "h264.h"

/*
* 8.4.1.3.1 Derivation process for median luma motion vector prediction
*/

static void Derivation_process_for_median_luma_motion_vector_prediction(
    // INPUT
    int mbAddrA, int mbPartIdxA, int subMbPartIdxA,
    int mbAddrB, int mbPartIdxB, int subMbPartIdxB,
    int mbAddrC, int mbPartIdxC, int subMbPartIdxC,
    int *mvLXA, int* mvLXB, int* mvLXC,
    int refIdxLXA, int refIdxLXB, int refIdxLXC, // Neighbour's refIdx
    int refIdxLX,   // Current's refIdx

    // OUTPUT
    int * mvpLX
)
{
    // Step 1
    if ((mbAddrB == -1) && (-1 == mbAddrC) && (-1 != mbAddrA)) {
        memcpy(mvLXB, mvLXA, sizeof(int) * 2);
        memcpy(mvLXC, mvLXA, sizeof(int) * 2);
        refIdxLXB = refIdxLXA;
        refIdxLXC = refIdxLXA;
    }
    
    // Step 2
    if ((refIdxLXA == refIdxLX) && (refIdxLXB != refIdxLX) && (refIdxLXC != refIdxLX)) {
        memcpy(mvpLX, mvLXA, sizeof(int) * 2);
    }
    else if ((refIdxLXA != refIdxLX) && (refIdxLXB == refIdxLX) && (refIdxLXC != refIdxLX)) {
        memcpy(mvpLX, mvLXB, sizeof(int) * 2);
    }
    else if ((refIdxLXA != refIdxLX) && (refIdxLXB != refIdxLX) && (refIdxLXC == refIdxLX)) {
        memcpy(mvpLX, mvLXC, sizeof(int) * 2);
    }
    else {
        mvpLX[0] = Median(mvLXA[0], mvLXB[0], mvLXC[0]);
        mvpLX[1] = Median(mvLXA[1], mvLXB[1], mvLXC[1]);
    }
}

/*
* 8.4.1.3.2 Derivation process for motion data of neighbouring partitions
*/

static void Derivation_process_for_motion_data_of_neighbouring_partitions(
    // INPUT
    int mbPartIdx,
    int subMbPartIdx,
    int currSubMbType,
    int listSuffixFlag,

    // OUTPUT
    int *mbAddrA, int* mbPartIdxA, int* subMbPartIdxA,
    int *mbAddrB, int* mbPartIdxB, int* subMbPartIdxB,
    int *mbAddrC, int* mbPartIdxC, int* subMbPartIdxC,
    int *mvLXA, int* mvLXB, int* mvLXC,
    int *refIdxLXA, int* refIdxLXB, int* refIdxLXC
)
{
    int predFlagLX = 0;
    struct MacroBlock_t *MB = &Slice.MB[Slice.CurrMbAddr];
    int mbAddrN;
    int mbPartIdxN;
    int subMbPartIdxN;
    int mvLXN[2];
    int refIdxLXN;
    int i;

    //=======================================================
    // Part A: Derivation of mbAddrN...
    //=======================================================
    
    // Step A.1: additional neighbouring partition
    int mbAddrD = NA, mbPartIdxD = NA, subMbPartIdxD = NA;

    // Step A.2
    Derivation_process_for_neighbouring_partitions(
        mbPartIdx, currSubMbType, subMbPartIdx,
        mbAddrA, mbPartIdxA, subMbPartIdxA,
        mbAddrB, mbPartIdxB, subMbPartIdxB,
        mbAddrC, mbPartIdxC, subMbPartIdxC,
        &mbAddrD, &mbPartIdxD, &subMbPartIdxD
        );

    // Step A.3
    if (*mbAddrC == NA) {
        *mbAddrC = mbAddrD;
        *mbPartIdxC = mbPartIdxD;
        *subMbPartIdxC = subMbPartIdxD;
    }

    //=======================================================
    // Part B: Derivation of mvLXN & refIdxLXN...
    //=======================================================

    // Prepare for Step B.1
    if ((Pred_L0 == MbPartPredMode(MB->mb_type, mbPartIdx)) ||
        (Pred_L1 == MbPartPredMode(MB->mb_type, mbPartIdx)) ||
        (BiPred == MbPartPredMode(MB->mb_type, mbPartIdx)) ||
        (Pred_L0 == SubMbPredMode(MB->sub_mb_type[mbPartIdx])) ||
        (Pred_L1 == SubMbPredMode(MB->sub_mb_type[mbPartIdx])) ||
        (BiPred == SubMbPredMode(MB->sub_mb_type[mbPartIdx]))) {
        predFlagLX = 1;
    }

    for (i = 0; i < 3; i++) {
        // Prepare
        switch (i) {
        case 0:
            mbAddrN = *mbAddrA;
            mbPartIdxN = *mbPartIdxA;
            subMbPartIdxN = *subMbPartIdxA;
            break;
        case 1:
            mbAddrN = *mbAddrB;
            mbPartIdxN = *mbPartIdxB;
            subMbPartIdxN = *subMbPartIdxB;
            break;
        case 2:
            mbAddrN = *mbAddrC;
            mbPartIdxN = *mbPartIdxC;
            subMbPartIdxN = *subMbPartIdxC;
            break;
        }

        // Step B.1
        if ((mbPartIdxN == NA) ||
            (subMbPartIdxN == NA) ||
            ((mbAddrN >= 0) && (Slice.MB[mbAddrN].mb_type <= I_PCM)) ||
            (predFlagLX == 0)) {
            mvLXN[0] = 0;
            mvLXN[1] = 0;
            refIdxLXN = -1;
        }
        else {
            assert(mbAddrN >= 0);

            // Step B.2.1
            if (listSuffixFlag == 0) {
                mvLXN[0] = Slice.MB[mbAddrN].mv_l0[mbPartIdxN][subMbPartIdxN][0];
                mvLXN[1] = Slice.MB[mbAddrN].mv_l0[mbPartIdxN][subMbPartIdxN][1];
                refIdxLXN = Slice.MB[mbAddrN].ref_idx_l0[mbPartIdxN];
            }
            else {
                mvLXN[0] = Slice.MB[mbAddrN].mv_l1[mbPartIdxN][subMbPartIdxN][0];
                mvLXN[1] = Slice.MB[mbAddrN].mv_l1[mbPartIdxN][subMbPartIdxN][1];
                refIdxLXN = Slice.MB[mbAddrN].ref_idx_l1[mbPartIdxN];
            }

            // Step B.2.2
            if (MB->is_field) {
                assert(0);
                if (!Slice.MB[mbAddrN].is_field) {
                    mvLXN[1] = mvLXN[1] / 2;
                    refIdxLXN = refIdxLXN * 2;
                }
            }
            else if (!MB->is_field) {
                if (Slice.MB[mbAddrN].is_field) {
                    mvLXN[1] = mvLXN[1] * 2;
                    refIdxLXN = refIdxLXN / 2;
                }
            }
            else {
                // No changes for mvLXN[1] & refIdxLXN
            }
        }

        switch (i) {
        case 0:
            mvLXA[0] = mvLXN[0];
            mvLXA[1] = mvLXN[1];
            *refIdxLXA = refIdxLXN;
            break;
        case 1:
            mvLXB[0] = mvLXN[0];
            mvLXB[1] = mvLXN[1];
            *refIdxLXB = refIdxLXN;
            break;
        case 2:
            mvLXC[0] = mvLXN[0];
            mvLXC[1] = mvLXN[1];
            *refIdxLXC = refIdxLXN;
            break;
        }
    }
}

/*
* 8.4.1.3 Derivation process for median luma motion vector prediction
*/
void Derivation_process_for_luma_motion_vector_prediction(
    // Input
    int mbPartIdx,
    int subMbPartIdx,
    int refIdxLX,
    int currSubMbType,

    // Output
    int *mvpLX
    )
{
    struct MacroBlock_t *MB = &Slice.MB[Slice.CurrMbAddr];
    int mbAddrA, mbAddrB, mbAddrC;
    int mbPartIdxA, mbPartIdxB, mbPartIdxC;
    int subMbPartIdxA, subMbPartIdxB, subMbPartIdxC;
    int mvLXA[2], mvLXB[2], mvLXC[2];
    int refIdxLXA, refIdxLXB, refIdxLXC;
    
    Derivation_process_for_motion_data_of_neighbouring_partitions(
        mbPartIdx,
        subMbPartIdx,
        currSubMbType,
        MB->listSuffixFlag,
        &mbAddrA, &mbPartIdxA, &subMbPartIdxA,
        &mbAddrB, &mbPartIdxB, &subMbPartIdxB,
        &mbAddrC, &mbPartIdxC, &subMbPartIdxC,
        mvLXA, mvLXB, mvLXC,
        &refIdxLXA, &refIdxLXB, &refIdxLXC
        );

    if ((MbPartWidth(MB->mb_type) == 16) &&
        (MbPartWidth(MB->mb_type) == 8) &&
        (mbPartIdx == 0) &&
        (refIdxLXB == refIdxLX)) {
        mvpLX[0] = mvLXB[0];
        mvpLX[1] = mvLXB[1];
    }
    else if ((MbPartWidth(MB->mb_type) == 16) &&
             (MbPartWidth(MB->mb_type) == 8) &&
             (mbPartIdx == 1) &&
             (refIdxLXA == refIdxLX)) {
             mvpLX[0] = mvLXA[0];
             mvpLX[1] = mvLXA[1];
    }
    else if ((MbPartWidth(MB->mb_type) == 8) &&
             (MbPartWidth(MB->mb_type) == 16) &&
             (mbPartIdx == 0) &&
             (refIdxLXA == refIdxLX)) {
             mvpLX[0] = mvLXA[0];
             mvpLX[1] = mvLXA[1];
    }
    else if ((MbPartWidth(MB->mb_type) == 8) &&
             (MbPartWidth(MB->mb_type) == 16) &&
             (mbPartIdx == 1) &&
             (refIdxLXC == refIdxLX)) {
             mvpLX[0] = mvLXC[0];
             mvpLX[1] = mvLXC[1];
    }
    else {
        // 16x16, 8x8, 4x4
        Derivation_process_for_median_luma_motion_vector_prediction(
            mbAddrA, mbPartIdxA, subMbPartIdxA,
            mbAddrB, mbPartIdxB, subMbPartIdxC,
            mbAddrC, mbPartIdxC, subMbPartIdxC,
            mvLXA, mvLXB, mvLXC,
            refIdxLXA, refIdxLXB, refIdxLXC,
            refIdxLX,
            mvpLX);
    }
}