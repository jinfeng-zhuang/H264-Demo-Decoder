#include "h264.h"

static int deriv_ctxIdxInc(const uint8_t decodedSE[32], const int binIdx, const int ctxIdxOffset);

// Table 9-39
// Assignment of ctxIdxInc to binIdx for all ctxIdxOffset values
// except those related to the syntax elements
// coded_block_flag, significant_coeff_flag, last_significant_coeff_flag, and coeff_abs_level_minus1
const int ctxIdxOffset_table[22] =
{ 0, 3, 11, 14, 17, 21, 24, 27, 32, 36, 40, 47, 54, 60, 64, 68, 69, 70, 73, 77, 276, 399 };

const int ctxIdxBlkTypeOffset_values[4][8] =
{
    {0,  8, 0,  4, 12, 12, 16, 16}, // coded_block_flag
    {0, 29, 0, 15, 44, 44, 47, 47}, // significant_coeff_flag
    {0, 29, 0, 15, 44, 44, 47, 47}, // last_significant_coeff_flag
    {0, 20, 0, 10, 30, 30, 39, 39}, // coeff_abs_level_minus1
};

//! Table 9-43 | Mapping of scanning position to ctxIdxInc for 8x8 blocks
const int ctxIdxInc_8x8blk[64][3] =
{
    { 0,  0, 0}, // 0
    { 1,  1, 1},
    { 2,  1, 1},
    { 3,  2, 1},
    { 4,  2, 1},
    { 5,  3, 1},
    { 5,  3, 1},
    { 4,  4, 1},
    { 4,  5, 1}, // 8
    { 3,  6, 1},
    { 3,  7, 1},
    { 4,  7, 1},
    { 4,  7, 1},
    { 4,  8, 1},
    { 5,  4, 1},
    { 5,  5, 1}, // 16
    { 4,  6, 2},
    { 4,  9, 2},
    { 4, 10, 2},
    { 4, 10, 2},
    { 3,  8, 2},
    { 3, 11, 2},
    { 6, 12, 2},
    { 7, 11, 2}, // 24
    { 7,  9, 2},
    { 7,  9, 2},
    { 8, 10, 2},
    { 9, 10, 2},
    {10,  8, 2},
    { 9, 11, 2},
    { 8, 12, 2},
    { 7, 11, 2}, // 32
    { 7,  9, 3},
    { 6,  9, 3},
    {11, 10, 3},
    {12, 10, 3},
    {13,  8, 3},
    {11, 11, 3},
    { 6, 12, 3},
    { 7, 11, 3}, // 40
    { 8,  9, 4},
    { 9,  9, 4},
    {14, 10, 4},
    {10, 10, 4},
    { 9,  8, 4},
    { 8, 13, 4},
    { 6, 13, 4},
    {11,  9, 4}, // 48
    {12,  9, 5},
    {13, 10, 5},
    {11, 10, 5},
    { 6,  8, 5},
    { 9, 13, 6},
    {14, 13, 6},
    {10,  9, 6},
    { 9,  9, 6}, // 56
    {11, 10, 7},
    {12, 10, 7},
    {13, 14, 7},
    {11, 14, 7},
    {14, 14, 8},
    {10, 14, 8},
    {12, 14, 8}, // 63
};

// 9.3.3.1.1.9
static int deriv_ctxIdxInc_coded_block_flag(const int blkType, const int blkIdx)
{
    int iYCbCr = 0;
    int mbAddrA = -1;
    int mbAddrB = -1;

    // Compute transBlockN
    ////////////////////////////////////////////////////////////////////////////
    int transBlockA = -1;
    int transBlockB = -1;

    if (blkType == blk_LUMA_16x16_DC)
    {
        mbAddrA = Slice.MB[Slice.CurrMbAddr].mbAddrA;
        mbAddrB = Slice.MB[Slice.CurrMbAddr].mbAddrB;

        // A
        if ((mbAddrA > -1) &&
            Slice.MB[mbAddrA].MbPartPredMode[0] == Intra_16x16)
        {
            transBlockA = 16;
        }

        // B
        if ((mbAddrB > -1) &&
            Slice.MB[mbAddrB].MbPartPredMode[0] == Intra_16x16)
        {
            transBlockB = 16;
        }
    }
    else if (blkType == blk_LUMA_4x4 || blkType == blk_LUMA_16x16_AC)
    {
        int luma4x4BlkIdxA = -1;
        int luma4x4BlkIdxB = -1;
        Derivation_process_for_neighbouring_4x4_luma_blocks(blkIdx, &mbAddrA, &luma4x4BlkIdxA, &mbAddrB, &luma4x4BlkIdxB);

        // A
        if (mbAddrA > -1) {
            if ((Slice.MB[mbAddrA].mb_type != P_Skip &&
                Slice.MB[mbAddrA].mb_type != B_Skip) &&
                (((Slice.MB[mbAddrA].CodedBlockPatternLuma >> (luma4x4BlkIdxA >> 2)) & 1) != 0))
            {
                if (Slice.MB[mbAddrA].mb_type != I_PCM &&
                    Slice.MB[mbAddrA].transform_size_8x8_flag == false)
                {
                    transBlockA = luma4x4BlkIdxA;
                }
                else if (Slice.MB[mbAddrA].transform_size_8x8_flag == true)
                {
                    transBlockA = luma4x4BlkIdxA >> 2;
                }
            }
        }

        // B
        if (mbAddrB > -1)
        {
            if ((Slice.MB[mbAddrB].mb_type != P_Skip &&
                Slice.MB[mbAddrB].mb_type != B_Skip) &&
                (((Slice.MB[mbAddrB].CodedBlockPatternLuma >> (luma4x4BlkIdxB >> 2)) & 1) != 0))
            {
                if (Slice.MB[mbAddrB].mb_type != I_PCM &&
                    Slice.MB[mbAddrB].transform_size_8x8_flag == false)
                {
                    transBlockB = luma4x4BlkIdxB;
                }
                else if (Slice.MB[mbAddrB].transform_size_8x8_flag == true)
                {
                    transBlockB = luma4x4BlkIdxB >> 2;
                }
            }
        }
    }
    else if (blkType == blk_CHROMA_DC_Cb || blkType == blk_CHROMA_DC_Cr)
    {
        if (blkType == blk_CHROMA_DC_Cb)
            iYCbCr = 1;
        else if (blkType == blk_CHROMA_DC_Cr)
            iYCbCr = 2;

        mbAddrA = Slice.MB[Slice.CurrMbAddr].mbAddrA;
        mbAddrB = Slice.MB[Slice.CurrMbAddr].mbAddrB;

        // A
        if (mbAddrA > -1)
        {
            if ((Slice.MB[mbAddrA].mb_type != P_Skip &&
                Slice.MB[mbAddrA].mb_type != B_Skip &&
                Slice.MB[mbAddrA].mb_type != I_PCM) &&
                Slice.MB[mbAddrA].CodedBlockPatternChroma != 0)
            {
                transBlockA = 4;
            }
        }

        // B
        if (mbAddrB > -1)
        {
            if ((Slice.MB[mbAddrB].mb_type != P_Skip &&
                Slice.MB[mbAddrB].mb_type != B_Skip &&
                Slice.MB[mbAddrB].mb_type != I_PCM) &&
                Slice.MB[mbAddrB].CodedBlockPatternChroma != 0)
            {
                transBlockB = 4;
            }
        }
    }
    else if (blkType == blk_CHROMA_AC_Cb || blkType == blk_CHROMA_AC_Cr)
    {
        if (blkType == blk_CHROMA_AC_Cb)
            iYCbCr = 1;
        else if (blkType == blk_CHROMA_AC_Cr)
            iYCbCr = 2;

        int chroma4x4BlkIdxA = -1;
        int chroma4x4BlkIdxB = -1;
        Derivation_process_for_neighbouring_4x4_chroma_blocks(blkIdx, &mbAddrA, &chroma4x4BlkIdxA, &mbAddrB, &chroma4x4BlkIdxB);

        // A
        if (mbAddrA > -1)
        {
            if ((Slice.MB[mbAddrA].mb_type != P_Skip &&
                Slice.MB[mbAddrA].mb_type != B_Skip &&
                Slice.MB[mbAddrA].mb_type != I_PCM) &&
                Slice.MB[mbAddrA].CodedBlockPatternChroma == 2)
            {
                transBlockA = chroma4x4BlkIdxA;
            }
        }

        // B
        if (mbAddrB > -1)
        {
            if ((Slice.MB[mbAddrB].mb_type != P_Skip &&
                Slice.MB[mbAddrB].mb_type != B_Skip &&
                Slice.MB[mbAddrB].mb_type != I_PCM) &&
                Slice.MB[mbAddrB].CodedBlockPatternChroma == 2)
            {
                transBlockB = chroma4x4BlkIdxB;
            }
        }
    }
    else if (blkType == blk_LUMA_8x8)
    {
        int luma8x8BlkIdxA = -1;
        int luma8x8BlkIdxB = -1;
        Derivation_process_for_neighbouring_8x8_luma_blocks(blkIdx, &mbAddrA, &luma8x8BlkIdxA, &mbAddrB, &luma8x8BlkIdxB);

        // A
        if (mbAddrA > -1)
        {
            if ((Slice.MB[mbAddrA].mb_type != P_Skip &&
                Slice.MB[mbAddrA].mb_type != B_Skip &&
                Slice.MB[mbAddrA].mb_type != I_PCM) &&
                (((Slice.MB[mbAddrA].CodedBlockPatternLuma >> luma8x8BlkIdxA) & 1) != 0) &&
                (Slice.MB[mbAddrA].transform_size_8x8_flag == true))
            {
                transBlockA = luma8x8BlkIdxA;
            }
        }

        // B
        if (mbAddrB > -1)
        {
            if ((Slice.MB[mbAddrB].mb_type != P_Skip &&
                Slice.MB[mbAddrB].mb_type != B_Skip &&
                Slice.MB[mbAddrB].mb_type != I_PCM) &&
                (((Slice.MB[mbAddrB].CodedBlockPatternLuma >> luma8x8BlkIdxB) & 1) != 0) &&
                (Slice.MB[mbAddrB].transform_size_8x8_flag == true))
            {
                transBlockB = luma8x8BlkIdxB;
            }
        }
    }
    else //if (blkType > 7)
    {
        return -1;
    }


    // Compute condTermFlagN
    ////////////////////////////////////////////////////////////////////////////
    bool condTermFlagA = false;
    bool condTermFlagB = false;

    // A
    if (mbAddrA > -1) {
        if (transBlockA == -1 && Slice.MB[mbAddrA].mb_type != I_PCM) {
            condTermFlagA = false;
        }
        else if (Slice.MB[mbAddrA].mb_type == I_PCM)
            condTermFlagA = true;
        else
            condTermFlagA = Slice.MB[mbAddrA].coded_block_flag[iYCbCr][transBlockA];
    }
    else {
        // mbAddrA is not available
        if (Slice.MB[Slice.CurrMbAddr].mb_type > I_PCM)
            condTermFlagA = false;
        else
            condTermFlagA = true;
    }

    // B
    if (mbAddrB > -1) {
        if (transBlockB == -1 && Slice.MB[mbAddrB].mb_type != I_PCM) {
            condTermFlagB = false;
        }
        else if (Slice.MB[mbAddrB].mb_type == I_PCM)
            condTermFlagB = true;
        else
            condTermFlagB = Slice.MB[mbAddrB].coded_block_flag[iYCbCr][transBlockB];
    }
    else
    {
        // mbAddrB is not available
        if (Slice.MB[Slice.CurrMbAddr].mb_type > I_PCM)
            condTermFlagB = false;
        else
            condTermFlagB = true;
    }

    return (condTermFlagA + condTermFlagB * 2);
}

//
// 9.3.3.1.3 Assignment process of ctxIdxInc for syntax elements significant_coeff_flag...
// 
// Let the variable levelListIdx be set equal to the index of the list of transform coefficient levels
// as specified in clause 7.4.5.3.
// (Note: list, not matrix)
//
// Let numDecodAbsLevelEq1 denote the accumulated number of decoded transform coefficient levels
// with absolute value equal to 1
//
// Let numDecodAbsLevelGt1 denote the accumulated number of decoded transform coefficient levels
// with absolute value greater than 1.
//
static int assign_ctxIdxInc_se(const enum SyntaxElementType_e seType, const enum BlockType_e blkType, const int binIdx)
{
    int ctxIdxInc = -1;

    if (seType == SE_significant_coeff_flag || seType == SE_last_significant_coeff_flag) {
        int levelListIdx = Slice.MB[Slice.CurrMbAddr].levelListIdx;

        if (blkType == blk_CHROMA_DC_Cb || blkType == blk_CHROMA_DC_Cr) {
            ctxIdxInc = Min(levelListIdx / SPS.NumC8x8, 2);
        }
        else if (blkType == blk_LUMA_8x8) {
            if (Slice.mb_field_decoding_flag == false)
                ctxIdxInc = ctxIdxInc_8x8blk[levelListIdx][(seType == SE_significant_coeff_flag) ? 0 : 2];
            else
                ctxIdxInc = ctxIdxInc_8x8blk[levelListIdx][(seType == SE_significant_coeff_flag) ? 1 : 2];
        }
        else {
            ctxIdxInc = levelListIdx;
        }
    }
    else //if (seType == SE_coeff_abs_level_minus1)
    {
        int Eq1 = Slice.MB[Slice.CurrMbAddr].numDecodAbsLevelEq1;
        int Gt1 = Slice.MB[Slice.CurrMbAddr].numDecodAbsLevelGt1;

        if (binIdx == 0) {
            ctxIdxInc = ((Gt1 != 0) ? 0 : Min(4, 1 + Eq1));
        }
        else {
            if (blkType == blk_CHROMA_DC_Cb || blkType == blk_CHROMA_DC_Cr)
                ctxIdxInc = 5 + Min(3, Gt1);
            else
                ctxIdxInc = 5 + Min(4, Gt1);
        }
    }

    return ctxIdxInc;
}

int Derivation_process_for_ctxIdx(enum SyntaxElementType_e seType, enum BlockType_e blkType, const int blkIdx, const uint8_t decodedSE[32], int binIdx, const int maxBinIdxCtx, const int ctxIdxOffset)
{
    int i = -1;
    int ctxIdxOffset_intable_flag = false;
    int ctxIdx = -1;
    int ctxIdxInc;

    binIdx = (binIdx > maxBinIdxCtx) ? maxBinIdxCtx : binIdx;

    // Check if ctx in Table 9-39 (Almost all Ctx)
    while (i < 21) {
        i++;
        if (ctxIdxOffset == ctxIdxOffset_table[i]) {
            ctxIdxOffset_intable_flag = true;
            i = 22;
        }
    }

    if (ctxIdxOffset_intable_flag) {
        // ctxIdx = ctxIdxOffset + ctxIdxInc;
        ctxIdxInc = deriv_ctxIdxInc(decodedSE, binIdx, ctxIdxOffset);

        if (ctxIdxInc == -1)
            ctxIdx = -1;
        else
            ctxIdx = ctxIdxOffset + ctxIdxInc;
    }
    else {
        // Table 9-40
        // ctxIdx = ctxIdxOffset + ctxIdxBlockCatOffset(ctxBlockCat) + ctxIdxInc(ctxBlockCat)
        // 9.3.3.1.1.9: ctxIdxInc(ctxBlockCat) for coded_block_flag
        // 9.3.3.1.3  : ctxIdxInc(ctxBlockCat) for
        //              significant_coeff_flag
        //              last_significant_coeff_flag
        //              coeff_abs_level_minus1
        if (seType == SE_coded_block_flag) {
            ctxIdxInc = deriv_ctxIdxInc_coded_block_flag(blkType, blkIdx);
            if (ctxIdxInc == -1) {
                ctxIdx = -1;
            }
            else {
                ctxIdx = ctxIdxOffset + ctxIdxBlkTypeOffset_values[0][blkType] + ctxIdxInc;
            }
        }
        else if (seType == SE_significant_coeff_flag ||
                seType == SE_last_significant_coeff_flag ||
                seType == SE_coeff_abs_level_minus1) {
            ctxIdx = ctxIdxOffset + ctxIdxBlkTypeOffset_values[seType - 12][blkType] + assign_ctxIdxInc_se(seType, blkType, binIdx);
        }
        else {
            assert(0);
        }
    }

    return ctxIdx;
}

//================================================================================
// Standard: 9.3.3.1.1
//================================================================================

// 9.3.3.1.1.1 Derivation process of ctxIdxInc for the syntax element mb_skip_flag.
static int deriv_ctxIdxInc_mb_skip_flag(void)
{
    int mbAddrA = Slice.MB[Slice.CurrMbAddr].mbAddrA;
    int mbAddrB = Slice.MB[Slice.CurrMbAddr].mbAddrB;

    int condTermFlagA = 1;
    int condTermFlagB = 1;

    if ((mbAddrA == -1) || (Slice.MB[mbAddrA].mb_skip_flag)) {
        condTermFlagA = 0;
    }
    else {
        condTermFlagA = 1;
    }

    if ((mbAddrB == -1) || (Slice.MB[mbAddrB].mb_skip_flag)) {
        condTermFlagB = 0;
    }
    else {
        condTermFlagB = 1;
    }

    return (condTermFlagA + condTermFlagB);
}

// 9.3.3.1.1.3 Derivation process of ctxIdxInc for the syntax element mb_type.
static int deriv_ctxIdxInc_mbtype(int ctxIdxOffset)
{
    int mbAddrA = Slice.MB[Slice.CurrMbAddr].mbAddrA;
    int mbAddrB = Slice.MB[Slice.CurrMbAddr].mbAddrB;

    int condTermFlagA = 1;
    int condTermFlagB = 1;

    if (mbAddrA > -1) {
        if (ctxIdxOffset == 3 && Slice.MB[mbAddrA].mb_type == I_NxN)
            condTermFlagA = 0;
        if ((ctxIdxOffset == 27) && ((Slice.MB[mbAddrA].mb_type == B_Skip) || (Slice.MB[mbAddrA].mb_type == B_Direct_16x16))) {
            condTermFlagA = 0;
        }
    }
    else {
        condTermFlagA = 0;
    }

    if (mbAddrB > -1) {
        if (ctxIdxOffset == 3 && Slice.MB[mbAddrB].mb_type == I_NxN)
            condTermFlagB = 0;
        if ((ctxIdxOffset == 27) && ((Slice.MB[mbAddrB].mb_type == B_Skip) || (Slice.MB[mbAddrB].mb_type == B_Direct_16x16))) {
            condTermFlagB = 0;
        }
    }
    else {
        condTermFlagB = 0;
    }

    return (condTermFlagA + condTermFlagB);
}

// 9.3.3.1.1.4 Derivation process of ctxIdxInc for the syntax element coded_block_pattern.
static int deriv_ctxIdxInc_cbp_luma(const uint8_t decodedSE[32], const int binIdx)
{
    int mbAddrA = -1;
    int mbAddrB = -1;

    int condTermFlagA = 1;
    int condTermFlagB = 1;

    int ctxIdxInc = 0;

    int luma8x8BlkIdxA = -1;
    int luma8x8BlkIdxB = -1;

    Derivation_process_for_neighbouring_8x8_luma_blocks(binIdx, &mbAddrA, &luma8x8BlkIdxA, &mbAddrB, &luma8x8BlkIdxB);

    if (mbAddrA != -1) {
        if (Slice.MB[mbAddrA].mb_type == I_PCM) {
            condTermFlagA = 0;
        }
        else {
            if ((mbAddrA != (int)(Slice.CurrMbAddr)) &&
                (Slice.MB[mbAddrA].mb_type != P_Skip && Slice.MB[mbAddrA].mb_type != B_Skip) &&
                (((Slice.MB[mbAddrA].CodedBlockPatternLuma >> luma8x8BlkIdxA) & 1) != 0)) {
                condTermFlagA = 0;
            }
            else if ((mbAddrA == (int)(Slice.CurrMbAddr)) && (decodedSE[luma8x8BlkIdxA] != 0)) {
                condTermFlagA = 0;
            }
        }
    }
    else {
        condTermFlagA = 0;
    }

    if (mbAddrB != -1) {
        if (Slice.MB[mbAddrB].mb_type == I_PCM) {
            condTermFlagB = 0;
        }
        else {
            if ((mbAddrB != (int)(Slice.CurrMbAddr)) &&
                (Slice.MB[mbAddrB].mb_type != P_Skip && Slice.MB[mbAddrB].mb_type != B_Skip) &&
                (((Slice.MB[mbAddrB].CodedBlockPatternLuma >> luma8x8BlkIdxB) & 1) != 0)) {
                condTermFlagB = 0;
            }
            else if ((mbAddrB == (int)(Slice.CurrMbAddr)) && (decodedSE[luma8x8BlkIdxB] != 0)) {
                condTermFlagB = 0;
            }
        }
    }
    else {
        condTermFlagB = 0;
    }

    ctxIdxInc = (condTermFlagA + condTermFlagB * 2);

    return ctxIdxInc;
}

// 9.3.3.1.1.4 Derivation process of ctxIdxInc for the syntax element coded_block_pattern.
static int deriv_ctxIdxInc_cbp_chroma(const uint8_t decodedSE[32], const int binIdx)
{
    int condTermFlagA = 1;
    int condTermFlagB = 1;

    int ctxIdxInc = 0;

    int mbAddrA = Slice.MB[Slice.CurrMbAddr].mbAddrA;
    int mbAddrB = Slice.MB[Slice.CurrMbAddr].mbAddrB;

    if (mbAddrA > -1) {
        if (Slice.MB[mbAddrA].mb_type == P_Skip || Slice.MB[mbAddrA].mb_type == B_Skip)
            condTermFlagA = 0;
        else if (binIdx == 0 && Slice.MB[mbAddrA].CodedBlockPatternChroma == 0)
            condTermFlagA = 0;
        else if (binIdx == 1 && Slice.MB[mbAddrA].CodedBlockPatternChroma != 2)
            condTermFlagA = 0;
    }
    else {
        condTermFlagA = 0;
    }

    if (mbAddrB > -1) {
        if (Slice.MB[mbAddrB].mb_type == P_Skip || Slice.MB[mbAddrB].mb_type == B_Skip)
            condTermFlagB = 0;
        else if (binIdx == 0 && Slice.MB[mbAddrB].CodedBlockPatternChroma == 0)
            condTermFlagB = 0;
        else if (binIdx == 1 && Slice.MB[mbAddrB].CodedBlockPatternChroma != 2)
            condTermFlagB = 0;
    }
    else {
        condTermFlagB = 0;
    }

    ctxIdxInc = condTermFlagA + condTermFlagB * 2 + ((binIdx == 1) ? 4 : 0);

    return ctxIdxInc;
}

// 9.3.3.1.1.5 Derivation process of ctxIdxInc for the syntax element mb_qp_delta.
static int deriv_ctxIdxInc_mb_qp_delta(void)
{
    int ctxIdxInc = 1;
    int prevMbAddr = Slice.CurrMbAddr - 1;

    if (prevMbAddr > -1) {
        if (Slice.MB[prevMbAddr].mb_type == I_PCM ||
            Slice.MB[prevMbAddr].mb_type == P_Skip ||
            Slice.MB[prevMbAddr].mb_type == B_Skip)
            ctxIdxInc = 0;
        else if ((Slice.MB[prevMbAddr].MbPartPredMode[0] != Intra_16x16) &&
            (Slice.MB[prevMbAddr].CodedBlockPatternLuma == 0 && Slice.MB[prevMbAddr].CodedBlockPatternChroma == 0))
            ctxIdxInc = 0;
        else if (Slice.MB[prevMbAddr].mb_qp_delta == 0)
            ctxIdxInc = 0;
    }
    else {
        ctxIdxInc = 0;
    }

    return ctxIdxInc;
}

/*
 * 9.3.3.1.1.7 Derivation process of ctxIdxInc for the syntax elements mvd_l0 and mvd_l1
 *  Input:  mbPartIdx, subMbPartIdx, and ctxIdxOffset
 *  Output: ctxIdxInc 
 *
 * Example:
 *  mbPartIdx = 1
 *  subMbPartIdx = 2
 *  ctxIdxOffset = 40
 ┌──────────┬─────┬─────┐
 │          │     │     │
 │          │     │     │
 ├──────────┼─────┼─────┤
 │          │  *  │     │
 │          │     │     │
 ├────┬─────┼─────┼─────┤
 │    │     │     │     │
 │    │     │     │     │
 │    │     │     │     │
 │    │     │     │     │
 └────┴─────┴─────┴─────┘
 */
static int deriv_ctxIdxInc_mvd(int mbPartIdx, int subMbPartIdx, int ctxIdxOffset)
{
    int is_mvd_l0;
    int i;
    int mbPartIdxN;
    int mbSubPartIdxN;
    int mbAddrN;
    int mbAddrA, mbPartIdxA, subMbPartIdxA;
    int mbAddrB, mbPartIdxB, subMbPartIdxB;
    int mbAddrC, mbPartIdxC, subMbPartIdxC;
    int mbAddrD, mbPartIdxD, subMbPartIdxD;
    int mvd_lX[4][4][2] = { 0 };
    int Pred_LX;
    int compIdx;
    int predModeEqualFlagN;
    int absMvdCompN;
    int absMvdComp[2];
    int mb_type;
    int sub_mb_type[4];
    struct MacroBlock_t *MB = &Slice.MB[Slice.CurrMbAddr];

    is_mvd_l0 = (MB->listSuffixFlag == 0) ? 1 : 0;

    // Step 1: => mvd_lX & Pred_LX, use neighbour's mvd for ctxIdxInc deriv
    //
    // Move to below already
    //

    // Step 2: => Neighbour
    Derivation_process_for_neighbouring_partitions(
        MB->mbPartIdx,
        MB->sub_mb_type[MB->subMbPartIdx],
        MB->subMbPartIdx,
        &mbAddrA, &mbPartIdxA, &subMbPartIdxA,
        &mbAddrB, &mbPartIdxB, &subMbPartIdxB,
        &mbAddrC, &mbPartIdxC, &subMbPartIdxC,
        &mbAddrD, &mbPartIdxD, &subMbPartIdxD
        );

    // Step 3: => compIdx
    if (ctxIdxOffset == 40) {
        compIdx = 0; // X Offset
    }
    else { // 47
        compIdx = 1; // Y Offset
    }

    for (i = 0; i < 2; i++) {

        // Step Hidden: Init the temp variables
        if (0 == i) {
            mbAddrN = mbAddrA;
            mbPartIdxN = mbPartIdxA;
            mbSubPartIdxN = subMbPartIdxA;
            mb_type = Slice.MB[mbAddrN].mb_type;
            memcpy(sub_mb_type, Slice.MB[mbAddrN].sub_mb_type, sizeof(sub_mb_type));
        }
        else {
            mbAddrN = mbAddrB;
            mbPartIdxN = mbPartIdxB;
            mbSubPartIdxN = subMbPartIdxB;
            if (mbAddrN >= 0) {
                mb_type = Slice.MB[mbAddrN].mb_type;
                memcpy(sub_mb_type, Slice.MB[mbAddrN].sub_mb_type, sizeof(sub_mb_type));
            }
            else {
                mb_type = NA;
                sub_mb_type[0] = NA;
                sub_mb_type[1] = NA;
                sub_mb_type[2] = NA;
                sub_mb_type[3] = NA;
            }
        }

        if (is_mvd_l0) {
            if (mbAddrN >= 0)
                memcpy(mvd_lX, Slice.MB[mbAddrN].mvd_l0, sizeof(mvd_lX));
            Pred_LX = Pred_L0;
        }
        else {
            if (mbAddrN >= 0)
                memcpy(mvd_lX, Slice.MB[mbAddrN].mvd_l1, sizeof(mvd_lX));
            Pred_LX = Pred_L1;
        }

        // Step 4: => predModeEqualFlagN

        if ((mb_type == B_Direct_16x16) || (mb_type == B_Skip)) {
            predModeEqualFlagN = 0;
        }
        else if ((mb_type == P_8x8) || (mb_type == B_8x8)) {
            if ((SubMbPredMode(sub_mb_type[mbPartIdxN]) != Pred_LX) && (SubMbPredMode(sub_mb_type[mbPartIdxN]) != BiPred)) {
                predModeEqualFlagN = 0;
            }
            else {
                predModeEqualFlagN = 1;
            }
        }
        else {
            if ((MbPartPredMode(mb_type, mbPartIdxN) != Pred_LX) && (MbPartPredMode(mb_type, mbPartIdxN) != BiPred)) {
                predModeEqualFlagN = 0;
            }
            else {
                predModeEqualFlagN = 1;
            }
        }

        // Step 5: => absMvdCompN

        if ((mbAddrN == -1) ||
            ((mb_type == P_Skip) || (mb_type == B_Skip)) ||
            ((Slice.MB[mbAddrN].MbPartPredMode[0] >= Intra_4x4) && (Slice.MB[mbAddrN].MbPartPredMode[0] <= Intra_16x16)) ||
            (predModeEqualFlagN == 0)) {
            absMvdCompN = 0;
        }
        else {
            if (0) { // MbaffFrameFlag = 1
            }
            else if (0) { // MbaffFrameFlag = 1
            }
            else {
                absMvdCompN = abs(mvd_lX[mbPartIdxN][mbSubPartIdxN][compIdx]);
            }
        }

        absMvdComp[i] = absMvdCompN;
    }

    // Step 6: => ctxIdxInc

    if (absMvdComp[0] + absMvdComp[1] < 3)
        return 0;
    else if (absMvdComp[0] + absMvdComp[1] > 32)
        return 2;
    else
        return 1;
}

// 9.3.3.1.1.8 Derivation process of ctxIdxInc for the syntax element intra_chroma_pred_mode.
static int deriv_ctxIdxInc_intra_chroma_pred_mode(void)
{
    int mbAddrA = Slice.MB[Slice.CurrMbAddr].mbAddrA;
    int mbAddrB = Slice.MB[Slice.CurrMbAddr].mbAddrB;

    int condTermFlagA = 1;
    int condTermFlagB = 1;

    if (mbAddrA > -1) {
        if (Slice.MB[mbAddrA].MbPartPredMode[0] > 3)
            condTermFlagA = 0;
        else if (Slice.MB[mbAddrA].mb_type == I_PCM)
            condTermFlagA = 0;
        else if (Slice.MB[mbAddrA].intra_chroma_pred_mode == 0)
            condTermFlagA = 0;
    }
    else {
        condTermFlagA = 0;
    }

    if (mbAddrB > -1) {
        if (Slice.MB[mbAddrB].MbPartPredMode[0] > 3)
            condTermFlagB = 0;
        else if (Slice.MB[mbAddrB].mb_type == I_PCM)
            condTermFlagB = 0;
        else if (Slice.MB[mbAddrB].intra_chroma_pred_mode == 0)
            condTermFlagB = 0;
    }
    else {
        condTermFlagB = 0;
    }

    return (condTermFlagA + condTermFlagB);
}

// 9.3.3.1.1.10 Derivation process of ctxIdxInc for the syntax element transform_size_8x8_flag.
static int deriv_ctxIdxInc_transform_size_8x8_flag(void)
{
    int mbAddrA = Slice.MB[Slice.CurrMbAddr].mbAddrA;
    int mbAddrB = Slice.MB[Slice.CurrMbAddr].mbAddrB;

    int condTermFlagA = 1;
    int condTermFlagB = 1;

    if (mbAddrA > -1) {
        if (Slice.MB[mbAddrA].transform_size_8x8_flag == false)
            condTermFlagA = 0;
    }
    else {
        condTermFlagA = 0;
    }

    if (mbAddrB > -1) {
        if (Slice.MB[mbAddrB].transform_size_8x8_flag == false)
            condTermFlagB = 0;
    }
    else {
        condTermFlagB = 0;
    }

    return (condTermFlagA + condTermFlagB);
}

//================================================================================
// Standard: 9.3.3.1.2
//================================================================================

// 9.3.3.1.2 Assignment process of ctxIdxInc using prior decoded bin values.
static int assign_ctxIdxInc_using_prior_values(const uint8_t decodedSE[32], const int ctxIdxOffset, const int binIdx)
{
    int ctxIdxInc = -1;

    if (ctxIdxOffset == 3) {
        if (binIdx == 4)
            ctxIdxInc = (decodedSE[3] != 0) ? 5 : 6;
        else if (binIdx == 5)
            ctxIdxInc = (decodedSE[3] != 0) ? 6 : 7;
    }
    else if (ctxIdxOffset == 14 && binIdx == 2)
        ctxIdxInc = (decodedSE[1] != 1) ? 2 : 3;
    else if (ctxIdxOffset == 17 && binIdx == 4)
        ctxIdxInc = (decodedSE[3] != 0) ? 2 : 3;
    else if (ctxIdxOffset == 27 && binIdx == 2)
        ctxIdxInc = (decodedSE[1] != 0) ? 4 : 5;
    else if (ctxIdxOffset == 32 && binIdx == 4)
        ctxIdxInc = (decodedSE[3] != 0) ? 2 : 3;
    else if (ctxIdxOffset == 36 && binIdx == 2)
        ctxIdxInc = (decodedSE[1] != 0) ? 2 : 3;
    else
        assert(0);

    return ctxIdxInc;
}

//================================================================================
// Standard: 9.3.3.1
//
// Attention: The ctxIncDerive Refer to Table 9-39
//================================================================================

static int deriv_ctxIdxInc(const uint8_t decodedSE[32], const int binIdx, const int ctxIdxOffset)
{
    int ctxIdxInc = -1;
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];

    switch (ctxIdxOffset) {
    case 0:
        assert(binIdx == 0);
        ctxIdxInc = deriv_ctxIdxInc_mbtype(ctxIdxOffset);
        break;
    case 3:
        if (binIdx == 0) {
            ctxIdxInc = deriv_ctxIdxInc_mbtype(ctxIdxOffset);
        }
        else if (binIdx == 1) {
            ctxIdxInc = 273;
        }
        else if (binIdx < 4) {
            ctxIdxInc = binIdx + 1;
        }
        else if (binIdx < 6) {
            ctxIdxInc = assign_ctxIdxInc_using_prior_values(decodedSE, ctxIdxOffset, binIdx);
        }
        else {
            ctxIdxInc = binIdx + 1;
        }
        break;
    case 17: // Table 9-39
    case 32:
        if (binIdx == 0) {
            ctxIdxInc = 0;
        }
        else if (binIdx == 1) {
            ctxIdxInc = 276 - 17;
        }
        else if (binIdx == 2) {
            ctxIdxInc = 1;
        }
        else if (binIdx == 3) {
            ctxIdxInc = 2;
        }
        else if (binIdx == 4) {
            ctxIdxInc = assign_ctxIdxInc_using_prior_values(decodedSE, ctxIdxOffset, binIdx);
        }
        else {
            ctxIdxInc = 3;
        }
        break;
    case 14: // Table 9-39
        if (0 == binIdx) {
            ctxIdxInc = 0;
        }
        else if (1 == binIdx) {
            ctxIdxInc = 1;
        }
        else if (2 == binIdx) {
            ctxIdxInc = assign_ctxIdxInc_using_prior_values(decodedSE, ctxIdxOffset, binIdx);
        }
        else{
            assert(0);
        }
        break;
    case 36: // Table 9-39
        if (0 == binIdx) {
            ctxIdxInc = 0;
        }
        else if (1 == binIdx) {
            ctxIdxInc = 1;
        }
        else if (2 == binIdx) {
            ctxIdxInc = assign_ctxIdxInc_using_prior_values(decodedSE, ctxIdxOffset, binIdx);
        }
        else if ((binIdx == 3) || (binIdx == 4) || (binIdx == 5)) {
            ctxIdxInc = 3;
        }
        else {
            assert(0);
        }
        break;
    case 27: // Table 9-39
        if (0 == binIdx) {
            ctxIdxInc = deriv_ctxIdxInc_mbtype(ctxIdxOffset);
        }
        else if (1 == binIdx) {
            ctxIdxInc = 3;
        }
        else if (2 == binIdx) {
            ctxIdxInc = assign_ctxIdxInc_using_prior_values(decodedSE, ctxIdxOffset, binIdx);
        }
        else {
            ctxIdxInc = 5;
        }
        break;
    case 11:
    case 24:
        assert(0 == binIdx);
        // 9.3.3.1.1.1: Derivation process of ctxIdxInc for the syntax element mb_skip_flag
        ctxIdxInc = deriv_ctxIdxInc_mb_skip_flag();
        break;
    case 21:
        ctxIdxInc = binIdx;
        break;
    case 40:
        if (binIdx == 0) {
            ctxIdxInc = deriv_ctxIdxInc_mvd(MB->mbPartIdx, MB->subMbPartIdx,ctxIdxOffset);
        }
        else if (binIdx < 5) {
            ctxIdxInc = binIdx + 2;
        }
        else {
            ctxIdxInc = 6;
        }
        break;
    case 47:
        if (binIdx == 0) {
            ctxIdxInc = deriv_ctxIdxInc_mvd(MB->mbPartIdx, MB->subMbPartIdx, ctxIdxOffset);
        }
        else if (binIdx < 5) {
            ctxIdxInc = binIdx + 2;
        }
        else {
            ctxIdxInc = 6;
        }
        break;
    case 60:
        if (binIdx == 0) {
            ctxIdxInc = deriv_ctxIdxInc_mb_qp_delta();
        }
        else if (binIdx == 1) {
            ctxIdxInc = 2;
        }
        else {
            ctxIdxInc = 3;
        }
        break;
    case 64:
        if (binIdx == 0)
            ctxIdxInc = deriv_ctxIdxInc_intra_chroma_pred_mode();
        else if (binIdx < 3)
            ctxIdxInc = 3;
        else
            assert(0);
        break;
    case 68:
        if (binIdx == 0)
            ctxIdxInc = 0;
        else
            assert(0);
        break;
    case 69:
        if (binIdx == 0 || binIdx == 1 || binIdx == 2)
            ctxIdxInc = 0;
        else
            assert(0);
        break;

    case 73:
        if (binIdx == 0 || binIdx == 1 || binIdx == 2 || binIdx == 3)
            ctxIdxInc = deriv_ctxIdxInc_cbp_luma(decodedSE, binIdx);
        else
            assert(0);
        break;

    case 77:
        if (binIdx == 0 || binIdx == 1)
            ctxIdxInc = deriv_ctxIdxInc_cbp_chroma(decodedSE, binIdx);
        else
            assert(0);
        break;

    case 276:
        if (binIdx == 0)
            ctxIdxInc = 0;
        else
            assert(0);
        break;

    case 399:
        if (binIdx == 0)
            ctxIdxInc = deriv_ctxIdxInc_transform_size_8x8_flag();
        else
            assert(0);
        break;

    default:
        assert(0);
        break;
    }

    return ctxIdxInc;
}