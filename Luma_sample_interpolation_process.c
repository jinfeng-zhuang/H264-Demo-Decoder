// Standard:
//   8.4 Inter prediction process
//   8.4.2 Decoding process for Inter prediction samples
//   8.4.2.2 Fractional sample interpolation process
//   8.4.2.2.1 Luma sample interpolation process
//
// JM:
//   get_block_luma
//     get_block_00/10/20/30/11/21/31/...
// 
// Pick each pixel's 1/4 sub-pixel or 1/2 sub-pixel to gen a new MB
// 
// 1/4 Pixel:
// (0, 0): copy directly
// (2, 0), (0, 2)
// (3, 3)
// 
// 1/2 Pixel:
// (1, 1), (1, 3), (3, 1), (3, 3)
// (1, 0), (3, 0), (0, 1), (0, 3)
// (2, 1), (2, 3), (1, 2), (3, 2)
// 

#include "h264.h"
#include "assert.h"

static void get_luma_00(
    int xIntL, int yIntL,
    int* refPicLXL[HEIGHT_PAD][WIDTH_PAD],
    int predPartLXL[16][16]
    )
{
    int i;

    for (i = 0; i < 16; i++) {
        memcpy(predPartLXL[i], &refPicLXL[yIntL][xIntL], 16);
    }
}

static void get_luma_20(
    int xIntL, int yIntL,
    int* refPicLXL[HEIGHT_PAD][WIDTH_PAD],
    int predPartLXL[16][16]
)
{
    int i, j;
    unsigned char P[6];
    int result;

    for (j = 0; j < 16; j++) {
        for (i = 0; i < 16; i++) {
            P[0] = refPicLXL[yIntL + j][xIntL - 2 + 0 + i];
            P[1] = refPicLXL[yIntL + j][xIntL - 2 + 1 + i];
            P[2] = refPicLXL[yIntL + j][xIntL - 2 + 2 + i];
            P[3] = refPicLXL[yIntL + j][xIntL - 2 + 3 + i];
            P[4] = refPicLXL[yIntL + j][xIntL - 2 + 4 + i];
            P[5] = refPicLXL[yIntL + j][xIntL - 2 + 5 + i];

            result = (P[0] - 5 * P[1] + 20 * P[2] + 20 * P[3] - 5 * P[4] + P[5]); // (8-241)
            result = (result + 16) / 32; // (8-243)

            predPartLXL[j][i] = result;
        }
    }
}

static void get_luma_31(
    int xIntL, int yIntL,
    int* refPicLXL[HEIGHT_PAD][WIDTH_PAD],
    int predPartLXL[16][16]
)
{
    int i, j;
    unsigned char P[6];
    int result;

    for (j = 0; j < 16; j++) {
        for (i = 0; i < 16; i++) {
            P[0] = refPicLXL[yIntL + j][xIntL - 2 + 0 + i];
            P[1] = refPicLXL[yIntL + j][xIntL - 2 + 1 + i];
            P[2] = refPicLXL[yIntL + j][xIntL - 2 + 2 + i];
            P[3] = refPicLXL[yIntL + j][xIntL - 2 + 3 + i];
            P[4] = refPicLXL[yIntL + j][xIntL - 2 + 4 + i];
            P[5] = refPicLXL[yIntL + j][xIntL - 2 + 5 + i];

            result = (P[0] - 5 * P[1] + 20 * P[2] + 20 * P[3] - 5 * P[4] + P[5]); // (8-241)
            result = (result + 16) / 32; // (8-243)

            predPartLXL[j][i] = result;
        }
    }

    for (j = 0; j < 16; j++) {
        for (i = 0; i < 16; i++) {
            P[0] = refPicLXL[yIntL - 2 + 0 + j][xIntL + i];
            P[1] = refPicLXL[yIntL - 2 + 1 + j][xIntL + i];
            P[2] = refPicLXL[yIntL - 2 + 2 + j][xIntL + i];
            P[3] = refPicLXL[yIntL - 2 + 3 + j][xIntL + i];
            P[4] = refPicLXL[yIntL - 2 + 4 + j][xIntL + i];
            P[5] = refPicLXL[yIntL - 2 + 5 + j][xIntL + i];

            result = (P[0] - 5 * P[1] + 20 * P[2] + 20 * P[3] - 5 * P[4] + P[5]); // (8-241)
            result = (result + 16) / 32; // (8-243)

            predPartLXL[j][i] = (predPartLXL[j][i] + result + 1) / 2; // (8-259)
        }
    }
}

static void (*get_luma_interpolation[4][4])(
    int xIntL, int yIntL,
    int* refPicLXL[HEIGHT_PAD][WIDTH_PAD],
    int predPartLXL[16][16]
    ) =
{
    {get_luma_00, get_luma_00, get_luma_00, get_luma_00},
    {get_luma_00, get_luma_00, get_luma_00, get_luma_31},
    {get_luma_00, get_luma_00, get_luma_00, get_luma_00},
    {get_luma_00, get_luma_00, get_luma_00, get_luma_00},
};

// 8.4.2.2.1 Luma sample interpolation process
// xFracL = [0,3]
// yFracL = [0,3]
void Luma_sample_interpolation_process(
    // INPUT
    int xIntL, int yIntL,                   // luma location in full-sample units
    int xFracL, int yFracL,                 // luma location offset in fractional-sample units
    int *refPicLXL[HEIGHT_PAD][WIDTH_PAD],  // luma sample array of the selected reference picture

    // OUTPUT
    int predPartLXL[16][16]
    )
{
    get_luma_interpolation[xFracL][yFracL](xIntL, yIntL, refPicLXL, predPartLXL);
}

// Refer to 8.4.2.2
void Fractional_sample_interpolation_process(unsigned char *refPicLXL, int predPartLXL[16][16])
{
    // INPUT
    int mbPartIdx, subMbPartIdx;
    int partWidth, partHeight;
    int mvLX[2];

    // OUTPUT: predPartLXL

    // VARIABLES
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];
    int xIntL, yIntL;
    int xFracL, yFracL;
    int xO, yO;

    mbPartIdx = MB->mbPartIdx;
    subMbPartIdx = MB->subMbPartIdx;
    mvLX[0] = MB->mv_l0[mbPartIdx][subMbPartIdx][0];
    mvLX[1] = MB->mv_l0[mbPartIdx][subMbPartIdx][1];

    // 1. Determine block address
    //    Reverse Scan by mbPartIdx & subMbPartIdx
    // TODO: Inverse_macroblock_partition_scanning_process
    Inverse_sub_macroblock_partition_scanning_process(mbPartIdx, subMbPartIdx, &xO, &yO);

    // 2. Determine pred block address
    // TODO: 16 for Luma, not Chroma
    xIntL = (W_PAD + Slice.CurrMbAddr % SPS.PicWidthInMbs) * 16 + xO;
    yIntL = (H_PAD + Slice.CurrMbAddr / SPS.PicWidthInMbs) * 16 + yO;

    // 3. Extend to 1/4 pixel
    xIntL = xIntL * 4 + mvLX[0];
    yIntL = yIntL * 4 + mvLX[1];

    // 4. the 1/4 offset in current pixel
    xFracL = xIntL & 3;
    yFracL = yIntL & 3;

    // 5. The position in original image
    xIntL = xIntL / 4;
    yIntL = yIntL / 4;

    refPicLXL = Luma_Pad;

    // 6. Derivate the pred block
    Luma_sample_interpolation_process(xIntL, yIntL, xFracL, yFracL, refPicLXL, predPartLXL);
}

// Refer to 8.4.2
// 根据 mv 和 refIdx 推导宏块中某个Block的预测值（1/4 像素）
void Decoding_process_for_Inter_prediction_samples(void)
{
    int i, j;
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];

    // INPUT
    int mbPartIdx;
    int subMbPartIdx;
    int partWidth, partHeight;
    int mvL0[2];
    int mvL1[2];
    int refIdxL0;
    int refIdxL1;
    int predFlagL0, predFlagL1;

    mbPartIdx = MB->mbPartIdx;
    subMbPartIdx = MB->subMbPartIdx;
    mvL0[0] = MB->mv_l0[mbPartIdx][subMbPartIdx][0];
    mvL0[1] = MB->mv_l0[mbPartIdx][subMbPartIdx][1];
    refIdxL0 = MB->ref_idx_l0[mbPartIdx];

    // VAR
    unsigned char predPart_Luma[16][16];    // Merge Result
    unsigned char predPartL0_Luma[16][16];  // L0 Result
    unsigned char predPartL1_Luma[16][16];  // L1 Result

    memset(predPart_Luma, 0, sizeof(predPart_Luma));
    memset(predPartL0_Luma, 0, sizeof(predPartL0_Luma));
    memset(predPartL1_Luma, 0, sizeof(predPartL1_Luma));

    // Refer to 8.4.2.2.1
    // Step 1: L0
    // 1. Ref Pic Select
    // 2. Fractional Sample
    // 'Luma_Pad' is the padding luma data of reference picture
    Fractional_sample_interpolation_process(Luma_Pad, predPartL0_Luma);
    // 3. Weighted Sample Predict

    // Refer to 8.4.2.2.1
    // Step 2: L1
    // 1. Ref Pic Select
    // 2. Fractional Sample
    // 'Luma_Pad' is the padding luma data of reference picture
    Fractional_sample_interpolation_process(Luma_Pad, predPartL1_Luma);
    // 3. Weighted Sample Predict

    // 8.4.2.3: Weighted sample prediction process
    // 8.4.2.3.1: Default weighted sample prediction process
    // 8.4.2.3.2: Weighted sample prediction process
    // Ignore

    // 8.4.3: Derivation process for prediction weights
    // Ignore

    // Step 3: Merge L0 & L1
    for (j = 0; j < 16; j++) {
        for (i = 0; i < 16; i++) {
            predPart_Luma[j][i] = predPartL0_Luma[j][i] + predPartL1_Luma[j][i];
        }
    }
}
