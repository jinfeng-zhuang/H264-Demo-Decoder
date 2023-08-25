#include "h264.h"

static void residual_block(enum BlockType_e blkType, int blkIdx, int *coeffLevel, int startIdx, int endIdx, int maxNumCoeff)
{
    int i;
    int coded_block_flag = 0;
    int numCoeff;
    int significant_coeff_flag[64] = { 0 }; // TODO: Max is 16x16?
    int last_significant_coeff_flag[64];
    int coeff_abs_level_minus1[64];
    int coeff_sign_flag[64];
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];

    assert(maxNumCoeff <= 64);

    MB->numDecodAbsLevelEq1 = 0;
    MB->numDecodAbsLevelGt1 = 0;
    MB->levelListIdx = 0;

    if (maxNumCoeff != 64 || 0) {
        coded_block_flag = read_ae_blk(SE_coded_block_flag, blkType, blkIdx);
        switch (blkType) {
        case blk_LUMA_16x16_DC:
            MB->coded_block_flag[0][maxNumCoeff] = coded_block_flag;
            break;
        case blk_CHROMA_DC_Cb:
            MB->coded_block_flag[1][maxNumCoeff] = coded_block_flag;
            break;
        case blk_CHROMA_DC_Cr:
            MB->coded_block_flag[2][maxNumCoeff] = coded_block_flag;
            break;
        case blk_CHROMA_AC_Cb:
            MB->coded_block_flag[1][blkIdx] = coded_block_flag;
            break;
        case blk_CHROMA_AC_Cr:
            MB->coded_block_flag[2][blkIdx] = coded_block_flag;
            break;
        default: // blk_LUMA_4x4 // blk_LUMA_16x16_AC
            MB->coded_block_flag[0][blkIdx] = coded_block_flag;
            break;
        }
    }
    else {
        coded_block_flag = 1;
        MB->coded_block_flag[0][blkIdx] = 1;
    }

    for (i = 0; i < maxNumCoeff; i++) {
        coeffLevel[i] = 0;
    }

    if (coded_block_flag) {
        numCoeff = endIdx + 1;
        i = startIdx;
        MB->levelListIdx = startIdx;

        while (i < numCoeff - 1) {
            significant_coeff_flag[i] = read_ae_blk(SE_significant_coeff_flag, blkType, blkIdx);
            if (significant_coeff_flag[i]) { // Check if the coeff is 0 or not
                last_significant_coeff_flag[i] = read_ae_blk(SE_last_significant_coeff_flag, blkType, blkIdx);
                if (last_significant_coeff_flag[i]) { // Check the number of coeff
                    numCoeff = i + 1;
                }
            }
            i++;
            MB->levelListIdx++;
        }

        // Decode last coefficient
        MB->levelListIdx = numCoeff - 1;
        coeff_abs_level_minus1[numCoeff - 1] = read_ae_blk(SE_coeff_abs_level_minus1, blkType, blkIdx);
        coeff_sign_flag[numCoeff - 1] = read_ae_blk(SE_coeff_sign_flag, blkType, blkIdx);

        // Update stats
        if ((coeff_abs_level_minus1[numCoeff - 1] + 1) == 1)
            MB->numDecodAbsLevelEq1++;
        else if ((coeff_abs_level_minus1[numCoeff - 1] + 1) > 1)
            MB->numDecodAbsLevelGt1++;

        coeffLevel[numCoeff - 1] = (coeff_abs_level_minus1[numCoeff - 1] + 1) * (1 - 2 * coeff_sign_flag[numCoeff - 1]);

        for (i = numCoeff - 2; i >= startIdx; i--) {
            if (significant_coeff_flag[i]) {
                MB->levelListIdx = i;
                coeff_abs_level_minus1[i] = read_ae_blk(SE_coeff_abs_level_minus1, blkType, blkIdx);
                coeff_sign_flag[i] = read_ae_blk(SE_coeff_sign_flag, blkType, blkIdx);

                if ((coeff_abs_level_minus1[i] + 1) == 1)
                    MB->numDecodAbsLevelEq1++;
                else if ((coeff_abs_level_minus1[i] + 1) > 1)
                    MB->numDecodAbsLevelGt1++;

                coeffLevel[i] = (coeff_abs_level_minus1[i] + 1) * (1 - 2 * coeff_sign_flag[i]);
            }
        }
    }

#if 0
    printf("Coeff: ");
    for (i = 0; i < maxNumCoeff; i++) {
        printf("%4d ", coeffLevel[i]);
    }
    printf("\n");
#endif
}

static void residual_luma(
    int *i16x16DClevel, int i16x16AClevel[][15], // for 16x16 block
    int level4x4[][16],                          // for 4x4   block
    int level8x8[][64],                          // for 8x8   block
    int startIdx, int endIdx)
{
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];
    int i;
    int i8x8;
    int i4x4;
    int blkIdx = 0;

    if (startIdx == 0 && MB->MbPartPredMode[0] == Intra_16x16) {
        //log2file("cabac.txt", "blk_LUMA_16x16_DC\n");
        residual_block(blk_LUMA_16x16_DC, 0, i16x16DClevel, 0, 15, 16);
    }

    for (i8x8 = 0; i8x8 < 4; i8x8++) {
        if (!MB->transform_size_8x8_flag || !PPS.entropy_coding_mode_flag) {
            for (i4x4 = 0; i4x4 < 4; i4x4++) {
                blkIdx = i8x8 * 4 + i4x4;
                if (MB->CodedBlockPatternLuma & (1 << i8x8)) {
                    if (MB->MbPartPredMode[0] == Intra_16x16) {
                        //log2file("cabac.txt", "blk_LUMA_16x16_AC: %d %d\n", i8x8, i4x4);
                        residual_block(blk_LUMA_16x16_AC, blkIdx, i16x16AClevel[blkIdx], Max(0, startIdx-1), endIdx-1, 15);
                    }
                    else {
                        //log2file("cabac.txt", "blk_LUMA_4x4: %d %d\n", i8x8, i4x4);
                        residual_block(blk_LUMA_4x4, blkIdx, level4x4[blkIdx], startIdx, endIdx, 16);
                    }
                }
                else if (MB->MbPartPredMode[0] == Intra_16x16) {
                    for (i = 0; i < 15; i++)
                        i16x16AClevel[blkIdx][i] = 0; // if not encoded, it means 0
                }
                else {
                    for (i = 0; i < 16; i++) {
                        level4x4[blkIdx][i] = 0;
                    }
                }

                if (0) {
                }
            }
        }
        else if (MB->CodedBlockPatternLuma & (1 << i8x8)) {
            // 8x8 block, ignore
            assert(0);
        }
        else {
            for (i = 0; i < 64; i++) {
                level8x8[i8x8][i] = 0;
            }
        }
    }
}

void residual(int startIdx, int endIdx)
{
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];
    int iCbCr;
    int i;
    int i8x8;
    int i4x4;
    int blkIdx = 0;

    //===============================================
    // Luma
    //===============================================
    residual_luma(MB->i16x16DClevel, MB->i16x16AClevel, MB->level4x4, MB->level8x8, startIdx, endIdx);

    if (1) { // ChromaArrayType == 1 || 2
        //===============================================
        // Chroma DC: Cb & Cr
        //===============================================
        for (iCbCr = 0; iCbCr < 2; iCbCr++) {
            //log2file("cabac.txt", "chroma DC: %d\n", iCbCr);

            /* chroma DC residual present */
            if ((MB->CodedBlockPatternChroma & 3) && (startIdx == 0)) {
                residual_block(blk_CHROMA_DC_Cb + iCbCr, 0, MB->ChromaDCLevel[iCbCr], 0, 4 * SPS.NumC8x8 - 1, 4 * SPS.NumC8x8);
            }
            else {
                for (i = 0; i < 4 * SPS.NumC8x8; i++) {
                    MB->ChromaDCLevel[iCbCr][i] = 0;
                }
            }
        }

        //===============================================
        // Chroma AC: Cb & Cr
        //===============================================
        for (iCbCr = 0; iCbCr < 2; iCbCr++) {
            for (i8x8 = 0; i8x8 < SPS.NumC8x8; i8x8++) {
                for (i4x4 = 0; i4x4 < 4; i4x4++) {
                    blkIdx = i8x8 * 4 + i4x4;

                    //log2file("cabac.txt", "chroma AC: %d %d %d\n", iCbCr, i8x8, i4x4);

                    if ((Slice.CurrMbAddr == 34) && (1 == iCbCr)) {
                        printf("DEBUG\n");
                    }

                    /* chroma AC residual present */
                    if (MB->CodedBlockPatternChroma & 2) {
                        residual_block(blk_CHROMA_AC_Cb + iCbCr, blkIdx, MB->ChromaACLevel[iCbCr][i8x8 * 4 + i4x4], Max(0, startIdx - 1), endIdx - 1, 15);
                    }
                    else {
                        for (i = 0; i < 15; i++) {
                            MB->ChromaACLevel[iCbCr][i8x8 * 4 + i4x4][i] = 0;
                        }
                    }
                }
            }
        }
    }
    else if (0) { // ChromaArrayType == 3
    }
}