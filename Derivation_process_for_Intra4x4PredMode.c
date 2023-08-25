#include "h264.h"
#include <stdio.h>

//
// 8.3.1.1 Derivation process for Intra4x4PredMode
//
void Derivation_process_for_Intra4x4PredMode(int luma4x4BlkIdx)
{
    int mbAddrA = -1, luma4x4BlkIdxA = 0, mbAddrB = -1, luma4x4BlkIdxB = 0;
    int dcPredModePredictedFlag = 0;
    int predIntra4x4PredMode;
    int intraMxMPredModeA;
    int intraMxMPredModeB;

    //==========================================================
    // 1: Neighbour
    //==========================================================
    Derivation_process_for_neighbouring_4x4_luma_blocks(luma4x4BlkIdx, &mbAddrA, &luma4x4BlkIdxA, &mbAddrB, &luma4x4BlkIdxB);

    //==========================================================
    // 2: Is DC Mode
    //==========================================================
    if (mbAddrA == -1)
        dcPredModePredictedFlag = 1;
    else if (mbAddrB == -1)
        dcPredModePredictedFlag = 1;
    else if ((mbAddrA > -1) && Slice.MB[mbAddrA].MbPartPredMode[0] > Intra_16x16 && PPS.constrained_intra_pred_flag == 1)
        dcPredModePredictedFlag = 1;
    else if ((mbAddrB > -1) && Slice.MB[mbAddrB].MbPartPredMode[0] > Intra_16x16 && PPS.constrained_intra_pred_flag == 1)
        dcPredModePredictedFlag = 1;

    //==========================================================
    // 3: Get Pred Mode of Neighbour's 4x4 Block 
    //==========================================================
    intraMxMPredModeA = 2;
    intraMxMPredModeB = 2;
    if (mbAddrA != -1) {
        if (dcPredModePredictedFlag == 0) {
            if (Slice.MB[mbAddrA].MbPartPredMode[0] == Intra_4x4)
                intraMxMPredModeA = Slice.MB[mbAddrA].Intra4x4PredMode[luma4x4BlkIdxA];
            else if (Slice.MB[mbAddrA].MbPartPredMode[0] == Intra_8x8)
                intraMxMPredModeA = Slice.MB[mbAddrA].Intra8x8PredMode[luma4x4BlkIdxA >> 2];
        }
    }
    if (mbAddrB != -1) {
        if (dcPredModePredictedFlag == 0) {
            if (Slice.MB[mbAddrB].MbPartPredMode[0] == Intra_4x4)
                intraMxMPredModeB = Slice.MB[mbAddrB].Intra4x4PredMode[luma4x4BlkIdxB];
            else if (Slice.MB[mbAddrB].MbPartPredMode[0] == Intra_8x8)
                intraMxMPredModeB = Slice.MB[mbAddrB].Intra8x8PredMode[luma4x4BlkIdxB >> 2];
        }
    }

    //==========================================================
    // 4: Assign Neighbour Pred Mode to Current 4x4 Block
    // 
    // Read from ES:
    //      prev_intra4x4_pred_mode_flag[16]
    //      rem_intra4x4_pred_mode[16]
    //==========================================================
    predIntra4x4PredMode = Min(intraMxMPredModeA, intraMxMPredModeB);
    if (Slice.MB[Slice.CurrMbAddr].prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]) {
        Slice.MB[Slice.CurrMbAddr].Intra4x4PredMode[luma4x4BlkIdx] = predIntra4x4PredMode;
    }
    else {
        if (Slice.MB[Slice.CurrMbAddr].rem_intra4x4_pred_mode[luma4x4BlkIdx] < predIntra4x4PredMode)
            Slice.MB[Slice.CurrMbAddr].Intra4x4PredMode[luma4x4BlkIdx] = Slice.MB[Slice.CurrMbAddr].rem_intra4x4_pred_mode[luma4x4BlkIdx];
        else
            Slice.MB[Slice.CurrMbAddr].Intra4x4PredMode[luma4x4BlkIdx] = Slice.MB[Slice.CurrMbAddr].rem_intra4x4_pred_mode[luma4x4BlkIdx] + 1;
    }
}
