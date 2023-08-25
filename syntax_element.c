#include "h264.h"

static unsigned int exp_golomb_value(struct bitstream_t* bs)
{
    int bit;
    int leadingZeroBits = -1;
    unsigned int value = 0; // default 0

    for (bit = 0; !bit; leadingZeroBits++) {
        bit = bitstream_read(1);
    }

    if (leadingZeroBits > 0)
        value = (unsigned int)pow(2.0, leadingZeroBits) - 1 + bitstream_read(leadingZeroBits);

    return value;
}

//=============================================================================
// API
//=============================================================================

extern char* se_type_str(int se_type);

int read_ae(enum SyntaxElementType_e seType)
{
    int value;
    struct binarization_t prefix;
    struct binarization_t suffix;

#ifdef DEBUG_CABAC
    log2file("cabac.txt", "=== %s ===\n", se_type_str(seType));
    bitstream_status();

    if ((Slice.CurrMbAddr == 155) && (seType == SE_mvd_lx0)) {
        printf("DEBUG\n");
    }
#endif

    prefix.SyntaxElementValue = -1;
    prefix.maxBinIdxCtx = -1;
    prefix.ctxIdxOffset = -1;
    prefix.bintable = NULL;
    prefix.bintable_x = -1;
    prefix.bintable_y = -1;
    prefix.bypassFlag = false;

    suffix.SyntaxElementValue = -1;
    suffix.maxBinIdxCtx = -1;
    suffix.ctxIdxOffset = -1;
    suffix.bintable = NULL;
    suffix.bintable_x = -1;
    suffix.bintable_y = -1;
    suffix.bypassFlag = false;

    binarization_get(seType, blk_UNKNOWN, &prefix, &suffix);

    // result write to prefix
    cabac_process(seType, blk_UNKNOWN, blk_UNKNOWN, &prefix);

    // patch for SE_mvd_lx
    if ((SE_mvd_lx0 == seType) || (SE_mvd_lx1 == seType)) {
        if (prefix.SyntaxElementValue == 0) {
            suffix.maxBinIdxCtx = -1; // no suffix
        }
        else {
            if (prefix.SyntaxElementValue >= 9) { // Refer to Clause (9-6), uCoff = 9
                suffix.maxBinIdxCtx = 0;
            }
        }
    }
    else if (SE_mb_type == seType) {
        // '4', '5' refer to binarization_mbtype_P
        // Refer to JM: interpret_mb_mode_P()
        if ((Slice.slice_type == SLICE_TYPE_P) && (prefix.SyntaxElementValue == 4)) {
            prefix.SyntaxElementValue = 5;
        }

        if ((Slice.slice_type == SLICE_TYPE_P) && (prefix.SyntaxElementValue != 5)) {
            suffix.maxBinIdxCtx = -1;
        }
        else if ((Slice.slice_type == SLICE_TYPE_B) && (prefix.SyntaxElementValue != 23)) {
            suffix.maxBinIdxCtx = -1;
        }
    }

    if (suffix.maxBinIdxCtx != -1) {
        cabac_process(seType, blk_UNKNOWN, blk_UNKNOWN, &suffix);

        if (SE_coded_block_pattern == seType) {
            // ChromaArrayType << 4 | CodedBlockPatternLuma
            // CodedBlockPatternLuma <= 1111b
            // ChromaArrayType <= 3
            value = prefix.SyntaxElementValue + 16 * suffix.SyntaxElementValue;
        }
        else {
            value = prefix.SyntaxElementValue + suffix.SyntaxElementValue;
        }
    }
    else {
        if (SE_mb_qp_delta == seType) {
            value = pow(-1.0, prefix.SyntaxElementValue + 1) * ceil(prefix.SyntaxElementValue / 2.0);
        }
        else {
            value = prefix.SyntaxElementValue;
        }
    }

    // Patch, TODO optimize
    if ((SE_mvd_lx0 == seType) || (SE_mvd_lx1 == seType)) {
        if (prefix.SyntaxElementValue != 0) {
            if (DecodeBypass()) {
                value = -value; // Refer to Clause (9-6)
            }
        }
    }

    return value;
}

int read_ae_blk(enum SyntaxElementType_e seType, enum BlockType_e blkType, const int blkIdx)
{
    // Initialization
    ////////////////////////////////////////////////////////////////////////////

    int SyntaxElementValue = 0;

    struct binarization_t prefix;
    prefix.SyntaxElementValue = -1;
    prefix.maxBinIdxCtx = -1;
    prefix.ctxIdxOffset = -1;
    prefix.bintable = NULL;
    prefix.bintable_x = -1;
    prefix.bintable_y = -1;
    prefix.bypassFlag = false;

    struct binarization_t suffix;
    suffix.SyntaxElementValue = -1;
    suffix.maxBinIdxCtx = -1;
    suffix.ctxIdxOffset = -1;
    suffix.bintable = NULL;
    suffix.bintable_x = -1;
    suffix.bintable_y = -1;
    suffix.bypassFlag = false;

#ifdef DEBUG_CABAC
    //log2file("cabac.txt", "=== %s ===\n", se_type_str(seType));
    //bitstream_status();

    if ((Slice.CurrMbAddr == 735) && (SE_coded_block_flag == seType)) {
        printf("DEBUG\n");
    }
#endif

    // Binarization process
    ////////////////////////////////////////////////////////////////////////////

    binarization_get(seType, blkType, &prefix, &suffix);

    // Arithmetic decoding process
    ////////////////////////////////////////////////////////////////////////////

    // Prefix
    cabac_process(seType, blkType, blkIdx, &prefix);

    // Suffix?
    if (seType == SE_coeff_abs_level_minus1 && prefix.SyntaxElementValue >= 14) {
        cabac_process(seType, blkType, blkIdx, &suffix);

        SyntaxElementValue = prefix.SyntaxElementValue + suffix.SyntaxElementValue;
    }
    else {
        SyntaxElementValue = prefix.SyntaxElementValue;
    }

    return SyntaxElementValue;
}


unsigned int read_ue(void)
{
    return exp_golomb_value(&H264.bs);
}

int read_se(void)
{
    unsigned int value;

    value = exp_golomb_value(&H264.bs);

    return (int)(pow(-1.0, value + 1) * ceil(value / 2.0));
}

unsigned int read_u(int n)
{
    return bitstream_read(n);
}