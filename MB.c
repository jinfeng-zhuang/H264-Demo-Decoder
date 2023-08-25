#include "h264.h"

// For I_4x4, Result in:
//   Intra4x4PredMode[4x4]
//   predL[16][16]
//   SprimeL[16][16]
static void MacroBlock_Parse_Fake(void)
{
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];

    bitstream_status();

    if (8159 == Slice.CurrMbAddr) {
        printf("DEBUG\n");
    }

    /*
     * enum mb_type_I
     * enum mb_type_P
     * enum mb_type_B
     */
    MB->mb_type = read_ae(SE_mb_type);
    if (Slice.slice_type == SLICE_TYPE_P) {
        if (MB->mb_type <= 4) {
            MB->mb_type += P_L0_16x16;

            assert(P_8x8ref0 != MB->mb_type); // no refIdx in SE, default is 0
        }
        else {
            // It's a I MB in P Slice, the mb_type derived by suffix
            // Refer to Table 9-37: (5 - 30), 25 mb_types, match I MB type count
            MB->mb_type -= 5;
        }
    }
    else if (Slice.slice_type == SLICE_TYPE_B) {
        MB->mb_type += B_Direct_16x16;

        if (B_Skip == MB->mb_type) {
            MB->mb_type -= B_Skip;
        }
    }

    MB->sub_mb_type[0] = NA;
    MB->sub_mb_type[1] = NA;
    MB->sub_mb_type[2] = NA;
    MB->sub_mb_type[3] = NA;

    assert(PPS.transform_8x8_mode_flag == 0);

    // TODO: will be get below, pre-set for MbPartPredMode
    MB->transform_size_8x8_flag = 0;

    MB->MbPartPredMode[0] = MbPartPredMode(MB->mb_type, 0);
    MB->NumMbPart = NumMbPart(MB->mb_type); // TODO

    if (Slice.CurrMbAddr == 266) {
        printf("DEBUG\n");
    }

    if (I_PCM == MB->mb_type) {
        assert(0);
    }
    else {
        MB->noSubMbPartSizeLessThan8x8Flag = 1;
        if ((MB->mb_type != I_NxN) && (MB->MbPartPredMode[0] != Intra_16x16) && (MB->NumMbPart == 4)) {
            sub_mb_pred();
            for (MB->mbPartIdx = 0; MB->mbPartIdx < 4; MB->mbPartIdx++) {
                if (MB->sub_mb_type[MB->mbPartIdx] != B_Direct_8x8) {
                    if (NumSubMbPart(MB->sub_mb_type[MB->mbPartIdx]) > 1) {
                        MB->noSubMbPartSizeLessThan8x8Flag = 0;
                    }
                }
                else if (!SPS.direct_8x8_inference_flag) {
                    MB->noSubMbPartSizeLessThan8x8Flag = 0;
                }
            }
        }
        else {
            if (PPS.transform_8x8_mode_flag && MB->mb_type == I_NxN) {
                MB->transform_size_8x8_flag = read_ae(SE_transform_size_8x8_flag);
            }
            mb_pred(MB->mb_type);
        }

        if (MB->MbPartPredMode[0] != Intra_16x16) {
            MB->coded_block_pattern = read_ae(SE_coded_block_pattern);

            MB->CodedBlockPatternLuma = MB->coded_block_pattern % 16;
            MB->CodedBlockPatternChroma = MB->coded_block_pattern / 16;

            if (MB->CodedBlockPatternLuma > 0 &&
                PPS.transform_8x8_mode_flag &&
                MB->mb_type != I_NxN &&
                MB->noSubMbPartSizeLessThan8x8Flag &&
                (MB->mb_type != B_Direct_16x16 || SPS.direct_8x8_inference_flag)) {
                MB->transform_size_8x8_flag = read_ae(SE_transform_size_8x8_flag);
            }
        }
        else {
            if (MB->mb_type > I_16x16_3_0_0)
            {
                if (MB->mb_type > I_16x16_3_2_0)
                {
                    MB->CodedBlockPatternLuma = 15;

                    if (MB->mb_type > I_16x16_3_1_1)
                        MB->CodedBlockPatternChroma = 2;
                    else if (MB->mb_type > I_16x16_3_0_1)
                        MB->CodedBlockPatternChroma = 1;
                }
                else
                {
                    if (MB->mb_type > I_16x16_3_1_0)
                        MB->CodedBlockPatternChroma = 2;
                    else if (MB->mb_type > I_16x16_3_0_0)
                        MB->CodedBlockPatternChroma = 1;
                }
            }
        }

        if (MB->CodedBlockPatternLuma > 0 ||
            MB->CodedBlockPatternChroma > 0 ||
            MB->MbPartPredMode[0] == Intra_16x16) {
            MB->mb_qp_delta = read_ae(SE_mb_qp_delta);

            residual(0, 15);
        }
    }

    /* (7-37) */
    MB->QPY = ((Slice.QPYprev + MB->mb_qp_delta + 52 + 2 * SPS.QpBdOffsetY) % (52 + SPS.QpBdOffsetY)) - SPS.QpBdOffsetY;
    MB->QPC = 0;

    MB->qP = MB->QPY + SPS.QpBdOffsetY; /* (7-38) */

    /*
     * where QPY,PREV is the luma quantisation parameter, QPY,
     * of the previous macroblock in decoding order in the current slice.
     */
    Slice.QPYprev = MB->QPY;
}

void macroblock_parse(void)
{
    int luma4x4BlkIdx;
    int i, j;
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];

    if (Slice.CurrMbAddr == 3) {
        printf("Breakpoint\n");
    }

    // Parse Pred, CBP, Residual: Copy from H264Visa & JM debug data
    MacroBlock_Parse_Fake();

    if (Intra_4x4 == Slice.MB[Slice.CurrMbAddr].MbPartPredMode[0]) {
        //===================================================
        // 16 4x4 Block Luma
        //===================================================
        for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++) {
            // Implement 8.3.1
            // 8.3.1.1: Derivation process for Intra4x4PredMode
            // 8.3.1.2: Intra_4x4 sample prediction
            Intra_4x4_prediction_process_for_luma_samples(luma4x4BlkIdx);

            // Implement 8.5.1, which depend on 8.5.12, 8.5.6, etc.
            // 8.5.1: transform decoding process for 4x4 luma residual blocks
            transform_decoding_process_for_4x4_luma_residual_blocks(luma4x4BlkIdx);
        }
    }
    else if (Intra_16x16 == Slice.MB[Slice.CurrMbAddr].MbPartPredMode[0]) {
        Intra_16x16_prediction_process_for_luma_samples();

        // Residual Rebuild: 8.5.2
        transform_decoding_process_for_luma_samples_of_Intra_16x16_macroblock_prediction_mode();
    }
    else if (SLICE_TYPE_P == Slice.slice_type) {
        if (P_Skip == Slice.MB[Slice.CurrMbAddr].mb_type) {
            // 1. Predition = Reference Value
            // 2. Residual = 0
            // 3. Reconstruction = Predition + 0
        }
        else {
            // The first MB is 8x8 type
            // Step 1: 1/4 Predition => MB.predL[16][16]

            if (P_L0_16x16 == Slice.MB[Slice.CurrMbAddr].mb_type) {

            }
            else if (P_L0_L0_16x8 == Slice.MB[Slice.CurrMbAddr].mb_type) {

            }
            else if (P_L0_L0_8x16 == Slice.MB[Slice.CurrMbAddr].mb_type) {

            }
            else if (P_8x8 == Slice.MB[Slice.CurrMbAddr].mb_type) {
                for (MB->mbPartIdx = 0; MB->mbPartIdx < 4; MB->mbPartIdx++) {
                    for (MB->subMbPartIdx = 0; MB->subMbPartIdx < NumSubMbPart(MB->sub_mb_type[MB->mbPartIdx]); MB->subMbPartIdx++) {
                        Decoding_process_for_Inter_prediction_samples();
                    }
                }
            }

            // Step 2: Residual
            // 4x4
            for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++) {
                transform_decoding_process_for_4x4_luma_residual_blocks(luma4x4BlkIdx);
            }
        }
    }

#if 0
    // Dump MB residual
    printf("Residual:\n");
    for (j = 0; j < 16; j++) {
        for (i = 0; i < 16; i++) {
            printf("%3d ", Slice.MB[Slice.CurrMbAddr].residual[j][i]);
        }
        printf("\n");
    }
#endif

#ifdef DEBUG
    printf("\nLuma:\n");
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            printf("%4d ", MB->SprimeL[j][i]);
        }
        printf("\n");
    }
#endif

    int xO = 0, yO = 0;
    xO = (Slice.CurrMbAddr % SPS.PicWidthInMbs) * 16;
    yO = (Slice.CurrMbAddr / SPS.PicWidthInMbs) * 16;
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 16; j++) {
            Luma[(xO + i) + (yO + j) * SPS.width] = MB->SprimeL[j][i];
        }
    }

    //===================================================
    // Chroma: Ignore
    //===================================================

    // Pred
    // DC: 2x2 Dequant-IDCT
    // AC: 4 * 4x4 - 1 = 15
    // 1 DC + 15 AC: zigzagscan > transform > blockscan > copy > pred + residual > copy
}