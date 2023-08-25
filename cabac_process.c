#include "h264.h"

//! Table 9-44 | Specification of rangeTabLPS depending on pStateIdx and qCodIRangeIdx
const uint16_t rangeTabLPS[64][4] =
{
    {128, 176, 208, 240}, // 0
    {128, 167, 197, 227},
    {128, 158, 187, 216},
    {123, 150, 178, 205},
    {116, 142, 169, 195},
    {111, 135, 160, 185},
    {105, 128, 152, 175},
    {100, 122, 144, 166},
    { 95, 116, 137, 158}, // 8
    { 90, 110, 130, 150},
    { 85, 104, 123, 142},
    { 81,  99, 117, 135},
    { 77,  94, 111, 128},
    { 73,  89, 105, 122},
    { 69,  85, 100, 116},
    { 66,  80,  95, 110},
    { 62,  76,  90, 104}, // 16
    {59, 72, 86, 99},
    {56, 69, 81, 94},
    {53, 65, 77, 89},
    {51, 62, 73, 85},
    {48, 59, 69, 80},
    {46, 56, 66, 76},
    {43, 53, 63, 72},
    {41, 50, 59, 69}, // 24
    {39, 48, 56, 65},
    {37, 45, 54, 62},
    {35, 43, 51, 59},
    {33, 41, 48, 56},
    {32, 39, 46, 53},
    {30, 37, 43, 50},
    {29, 35, 41, 48},
    {27, 33, 39, 45}, // 32
    {26, 31, 37, 43},
    {24, 30, 35, 41},
    {23, 28, 33, 39},
    {22, 27, 32, 37},
    {21, 26, 30, 35},
    {20, 24, 29, 33},
    {19, 23, 27, 31},
    {18, 22, 26, 30}, // 40
    {17, 21, 25, 28},
    {16, 20, 23, 27},
    {15, 19, 22, 25},
    {14, 18, 21, 24},
    {14, 17, 20, 23},
    {13, 16, 19, 22},
    {12, 15, 18, 21},
    {12, 14, 17, 20}, // 48
    {11, 14, 16, 19},
    {11, 13, 15, 18},
    {10, 12, 15, 17},
    {10, 12, 14, 16},
    { 9, 11, 13, 15},
    { 9, 11, 12, 14},
    { 8, 10, 12, 14},
    { 8,  9, 11, 13}, // 56
    { 7,  9, 11, 12},
    { 7,  9, 10, 12},
    { 7,  8, 10, 11},
    { 6,  8,  9, 11},
    { 6,  7,  9, 10},
    { 6,  7,  8,  9},
    { 2,  2,  2,  2}, // 63
};

//! Table 9-45 | State transition table for Least Probable Symbol
const uint8_t transIdxLPS[64] =
{ 0,  0,  1,  2,  2,  4,  4,  5,  6,  7,  8,  9,  9, 11, 11, 12,
 13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21, 21, 22, 22, 23, 24,
 24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 30, 30, 31, 32, 32, 33,
 33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 63 };

//! Table 9-45 | State transition table for Most Probable Symbol
const uint8_t transIdxMPS[64] =
{ 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 62, 63 };

static void RenormD(struct CabacCtx_t* cc)
{
    while (cc->codIRange < 256) {
        cc->codIRange <<= 1;
        cc->codIOffset <<= 1;
        cc->codIOffset |= bitstream_read(1);
    }
}

unsigned char DecodeBypass(void)
{
    uint8_t binVal = 0;

#ifdef DEBUG_CABAC
    log2file("cabac.txt", "[Bypass] range = %-4d, offset = %-6d\n",
        Slice.CabacCtx.codIRange,
        Slice.CabacCtx.codIOffset);
#endif

    Slice.CabacCtx.codIOffset <<= 1;
    Slice.CabacCtx.codIOffset |= bitstream_read(1);

    if (Slice.CabacCtx.codIOffset >= Slice.CabacCtx.codIRange) {
        binVal = 1;
        Slice.CabacCtx.codIOffset -= Slice.CabacCtx.codIRange;
    }

    return binVal;
}

// 9.3.3.2.1 Arithmetic decoding process for a binary decision.
static unsigned char DecodeDecision(const int ctxIdx)
{
    uint8_t binVal = 0;
    uint16_t codIRangeIdx;
    uint16_t codIRangeLPS;

#ifdef DEBUG_CABAC
    log2file("cabac.txt", "ctxId = %-3d, state = %-2d, MPS = %d, range = %-4d, offset = %-6d\n",
        0, //ctxIdx,
        Slice.CabacCtx.pStateIdx[ctxIdx],
        Slice.CabacCtx.valMPS[ctxIdx],
        Slice.CabacCtx.codIRange,
        Slice.CabacCtx.codIOffset);
#endif

    codIRangeIdx = (Slice.CabacCtx.codIRange >> 6) & 3;
    codIRangeLPS = rangeTabLPS[Slice.CabacCtx.pStateIdx[ctxIdx]][codIRangeIdx];

    Slice.CabacCtx.codIRange -= codIRangeLPS;

    if (Slice.CabacCtx.codIOffset < Slice.CabacCtx.codIRange) {
        binVal = Slice.CabacCtx.valMPS[ctxIdx];

        // 9.3.3.2.1.1 State transition process
        Slice.CabacCtx.pStateIdx[ctxIdx] = transIdxMPS[Slice.CabacCtx.pStateIdx[ctxIdx]];
    }
    else {
        binVal = 1 - Slice.CabacCtx.valMPS[ctxIdx];

        Slice.CabacCtx.codIOffset -= Slice.CabacCtx.codIRange;
        Slice.CabacCtx.codIRange = codIRangeLPS;

        // 9.3.3.2.1.1 State transition process
        if (Slice.CabacCtx.pStateIdx[ctxIdx] == 0) {
            Slice.CabacCtx.valMPS[ctxIdx] = 1 - Slice.CabacCtx.valMPS[ctxIdx];
        }

        Slice.CabacCtx.pStateIdx[ctxIdx] = transIdxLPS[Slice.CabacCtx.pStateIdx[ctxIdx]];
    }

    RenormD(&Slice.CabacCtx);

    return binVal;
}

// 9.3.3.2.1 Arithmetic decoding process for a binary decision.
static unsigned char DecodeTerminate(const int ctxIdx)
{
    uint8_t binVal = 1;

#ifdef DEBUG_CABAC
    log2file("cabac.txt", "[Terminate] range = %-4d, offset = %-6d\n",
        Slice.CabacCtx.codIRange,
        Slice.CabacCtx.codIOffset);
#endif

    // codIRange never goes under 25x, no need to check for integer underflow
    Slice.CabacCtx.codIRange -= 2;

    if (Slice.CabacCtx.codIOffset < Slice.CabacCtx.codIRange)
    {
        binVal = 0;

        RenormD(&Slice.CabacCtx);
    }

    return binVal;
}

int cabac_process(
    enum SyntaxElementType_e seType,
    enum BlockType_e blkType,
    const int blkIdx,
    struct binarization_t* bin)
{
    int retcode = -1;
    int binIdx = -1;
    int ctxIdx = -1;
    int match = 999;
    uint8_t decodedSE[32];
    int i;

    while (match > 1)
    {
        // next bin
        binIdx++;

        if (bin->bypassFlag == true) {
            decodedSE[binIdx] = DecodeBypass();
        }
        else {
            // 1. Given binIdx, maxBinIdxCtx and ctxIdxOffset, ctxIdx is derived.
            //    9.3.3.1 Derivation process for ctxIdx.
            // 
            //    Syntax Element
            //    --------------------------------------------------
            //                   [binIdx]             [maxBinIdxCtx]
            //    --------------------------------------------------
            //    ctxIdxOffset
            //    => ctxIdx
            ctxIdx = Derivation_process_for_ctxIdx(seType, blkType, blkIdx, decodedSE, binIdx, bin->maxBinIdxCtx, bin->ctxIdxOffset);

            // 2. Given ctxIdx, the value of the bin from the bitstream is decoded.
            assert(ctxIdx != -1);
            assert(ctxIdx <= 459); // please extend cabac_context_init_I table

            if (ctxIdx == 276) {
                decodedSE[binIdx] = DecodeTerminate(ctxIdx);
            }
            else {
                // normal: DecodeDecision
                decodedSE[binIdx] = DecodeDecision(ctxIdx);
            }
        }

        // 3. Compare bin string with binarization
        i = 0;
        match = 0;
        while (i < bin->bintable_y) {
            if (0 == memcmp(decodedSE, (((uint8_t*)(bin->bintable)) + (int)(i * (bin->bintable_x))), binIdx + 1)) {
                bin->SyntaxElementValue = i;
                match++;
            }

            i++;
        }

        // 4. Check result of compare, only one can be matched
        if (match == 1)
            retcode = 0;
    }

    return retcode;
}