#include "h264.h"

struct Table_7_14_t {
    int mb_type;
    int NumMbPart;
    int MbPartPredMode0;
    int MbPartPredMode1;
    int MbPartWidth;
    int MbPartHeight;
} Table_7_14[] = {
    {B_Direct_16x16, NA, Direct, NA, 8, 8},
    {B_L0_16x16, 1, Pred_L0, NA, 16, 16},
    {B_L1_16x16, 1, Pred_L1, NA, 16, 16},
    {B_Bi_16x16, 1, BiPred, NA, 16, 16},
    {B_L0_L0_16x8, 2, Pred_L0, Pred_L0, 16, 8},
    {B_L0_L0_8x16, 2, Pred_L0, Pred_L0, 8, 16},
    {B_L1_L1_16x8, 2, Pred_L1, Pred_L1, 16, 8},
    {B_L1_L1_8x16, 2, Pred_L1, Pred_L1, 8, 16},
    {B_L0_L1_16x8, 2, Pred_L0, Pred_L1, 16, 8},
    {B_L0_L1_8x16, 2, Pred_L0, Pred_L1, 8, 16},
    {B_L1_L0_16x8, 2, Pred_L1, Pred_L0, 16, 8},
    {B_L1_L0_8x16, 2, Pred_L1, Pred_L0, 8, 16},
    {B_L0_Bi_16x8, 2, Pred_L0, BiPred, 16, 8},
    {B_L0_Bi_8x16, 2, Pred_L0, BiPred, 8, 16},
    {B_L1_Bi_16x8, 2, Pred_L1, BiPred, 16, 8},
    {B_L1_Bi_8x16, 2, Pred_L1, BiPred, 8, 16},
    {B_Bi_L0_16x8, 2, BiPred, Pred_L0, 16, 8},
    {B_Bi_L0_8x16, 2, BiPred, Pred_L0, 8, 16},
    { B_Bi_L1_16x8, 2, BiPred, Pred_L1, 16, 8 },
    { B_Bi_L1_8x16, 2, BiPred, Pred_L1, 8, 16 },
    { B_Bi_Bi_16x8, 2, BiPred, BiPred, 16, 8 },
    { B_Bi_Bi_8x16, 2, BiPred, BiPred, 8, 16 },
    { B_8x8, 4, NA, NA, 8, 8 },
    { B_Skip, NA, Direct, NA, 8, 8 }
};

struct Table_7_18_t {
    int sub_mb_type;
    int NumSubMbPart;
    int SubMbPredMode;
    int SubMbPartWidth;
    int SubMbPartHeight;
} Table_7_18[] = {
    {B_Direct_8x8, 4, Direct, 4, 4},
    {B_L0_8x8, 1, Pred_L0, 8, 8},
    {B_L1_8x8, 1, Pred_L1, 8, 8},
    {B_Bi_8x8, 1, BiPred, 8, 8},
    {B_L0_8x4, 2, Pred_L0, 8, 4},
    {B_L0_4x8, 2, Pred_L0, 4, 8},
    {B_L1_8x4, 2, Pred_L1, 8, 4},
    {B_L1_4x8, 2, Pred_L1, 4, 8},
    {B_Bi_8x4, 2, BiPred, 8, 4},
    {B_Bi_4x8, 2, BiPred, 4, 8},
    {B_L0_4x4, 4, Pred_L0, 4, 4},
    {B_L1_4x4, 4, Pred_L1, 4, 4},
    {B_Bi_4x4, 4, BiPred, 4, 4}
};

int NumMbPart(int mb_type)
{
    // TODO
    int transform_size_8x8_flag = 0;

    if (Slice.slice_type == SLICE_TYPE_P) {
        switch (mb_type) {
        case P_L0_16x16:
            return 1;
        case P_L0_L0_16x8:
            return 2;
        case P_L0_L0_8x16:
            return 2;
        case P_8x8:
            return 4;
        case P_8x8ref0:
            return 4;
        }
    }
    else if (Slice.slice_type == SLICE_TYPE_B) {
        if (B_Direct_16x16 == mb_type) {
            return NA;
        }
        else if ((B_L0_16x16 == mb_type) || (B_L1_16x16 == mb_type) || (B_Bi_16x16 == mb_type)) {
            return 1;
        }
        else if (B_8x8 == mb_type) {
            return 4;
        }
        else if (B_Skip == mb_type) {
            assert(0);
        }
        else {
            return 2;
        }
    }

    if (mb_type <= I_PCM) {
        if (mb_type == I_NxN) {
            if (transform_size_8x8_flag == 0)
                return 16;
            else
                return 4;
        }
        else if (mb_type == I_PCM) {
            return NA;
        }
        else {
            return 1;
        }
    }

    assert(0);
    return 0;
}

int Intra16x16PredMode(int mb_type)
{
    // Intra 16x16 Prediction Mode, see Table 7-11
    unsigned int i = 1, j = 0;
    int Intra16x16PredMode;

    for (i = 1; i < 5; i++)
    {
        for (j = 0; j < 6; j++)
        {
            if (mb_type == (i + j * 4))
            {
                Intra16x16PredMode = i - 1;
                i = j = 9;
            }
        }
    }

    return Intra16x16PredMode;
}

// Table 7-11 ?Macroblock types for I slices
// Table 7-12 ?Macroblock type with value 0 for SI slices
// Table 7-13 ?Macroblock type values 0 to 4 for P and SP slices
// Table 7-14 ?Macroblock type values 0 to 22 for B slices
int MbPartPredMode(int mb_type, int mbPartIdx)
{
    // TODO
    int transform_size_8x8_flag = 0;

    if (NA == mbPartIdx)
        return NA;

    int slice_type = Slice.slice_type;
    int retcode = 0;

    if (slice_type == 2 || slice_type == 7) // I slice
    {
        assert(mbPartIdx == 0);

        if (mb_type == I_NxN) {
            if (transform_size_8x8_flag)
                retcode = Intra_8x8;
            else
                retcode = Intra_4x4;
        }
        else if (mb_type < I_PCM) {
            retcode = Intra_16x16;
        }
        else {
            retcode = NA;
        }
    }
    else if (slice_type == 4 || slice_type == 9) // SI slice
    {
        assert(0);
    }
    else if (slice_type == SLICE_TYPE_P || slice_type == 5 || slice_type == 3 || slice_type == 8) // P or SP slice
    {
        // I MB in P Slice
        if (mb_type <= I_PCM) {
            if (mb_type == I_NxN) {
                if (transform_size_8x8_flag)
                    retcode = Intra_8x8;
                else
                    retcode = Intra_4x4;
            }
            else if (mb_type < I_PCM) {
                retcode = Intra_16x16;
            }
            else {
                retcode = NA;
            }
        }
        else {
            // ref to Table 7-13
            if (0 == mbPartIdx) {
                if ((mb_type != P_8x8) && (mb_type != P_8x8ref0))
                    retcode = Pred_L0;
                else
                    retcode = NA;
            }
            else {
                if ((P_L0_L0_16x8 == mb_type) || (P_L0_L0_8x16 == mb_type))
                    retcode = Pred_L0;
                else
                    retcode = NA;
            }
        }
    }
    else if (slice_type == SLICE_TYPE_B || slice_type == 6) // B slice
    {
        // I MB in P Slice
        if (mb_type <= I_PCM) {
            if (mb_type == I_NxN) {
                if (transform_size_8x8_flag)
                    retcode = Intra_8x8;
                else
                    retcode = Intra_4x4;
            }
            else if (mb_type < I_PCM) {
                retcode = Intra_16x16;
            }
            else {
                retcode = NA;
            }
        }
        else {
            for (int i = 0; i < ARRAY_SIZE(Table_7_14); i++) {
                if (mb_type == Table_7_14[i].mb_type) {
                    if (mbPartIdx == 0)
                        return Table_7_14[i].MbPartPredMode0;
                    else if (mbPartIdx == 1)
                        return Table_7_14[i].MbPartPredMode1;
                    else
                        return NA;
                }
            }
            assert(0);
        }
    }

    return retcode;
}

int NumSubMbPart(int sub_mb_type)
{
    if (Slice.slice_type == SLICE_TYPE_P) {
        switch (sub_mb_type) {
        case P_L0_8x8:
            return 1;
            break;
        case P_L0_8x4:
            return 2;
            break;
        case P_L0_4x8:
            return 2;
            break;
        case P_L0_4x4:
            return 4;
            break;

        case B_Direct_8x8:
            return 4;
            break;
        }
    }
    else if (Slice.slice_type == SLICE_TYPE_B) {
        for (int i = 0; i < ARRAY_SIZE(Table_7_18); i++) {
            if (sub_mb_type == Table_7_18[i].sub_mb_type) {
                return Table_7_18[i].NumSubMbPart;
            }
        }
        assert(0);
    }

    assert(0);
    return 0;
}

int SubMbPredMode(int sub_mb_type)
{
    if (Slice.slice_type == SLICE_TYPE_P) {
        switch (sub_mb_type) {
        case P_L0_8x8:
            return Pred_L0;
        case P_L0_8x4:
            return Pred_L0;
        case P_L0_4x8:
            return Pred_L0;
        case P_L0_4x4:
            return Pred_L0;
        default:
            return NA;
        }
    }
    else if (Slice.slice_type == SLICE_TYPE_B) {
        for (int i = 0; i < ARRAY_SIZE(Table_7_18); i++) {
            if (sub_mb_type == Table_7_18[i].sub_mb_type) {
                return Table_7_18[i].SubMbPredMode;
            }
        }
        assert(0);
    }

    assert(0);
    return 0;
}

// Table 7-13
int MbPartWidth(int mb_type)
{
    if (Slice.slice_type == SLICE_TYPE_P) {
        if (mb_type == P_L0_16x16)
            return 16;
        else if (mb_type == P_L0_L0_16x8)
            return 16;
        else if (mb_type == P_L0_L0_8x16)
            return 8;
        else if (mb_type == P_8x8)
            return 8;
        else if (mb_type == P_8x8ref0)
            return 8;
        else
            return 16;
    }
    else if (Slice.slice_type == SLICE_TYPE_B) {
        for (int i = 0; i < ARRAY_SIZE(Table_7_14); i++) {
            if (mb_type == Table_7_14[i].mb_type) {
                return Table_7_14[i].MbPartWidth;
            }
        }
        assert(0);
    }

    assert(0);
    return 0;
}

// Table 7-13
int MbPartHeight(int mb_type)
{
    int i;

    if (Slice.slice_type == SLICE_TYPE_P) {
        if (mb_type == P_L0_16x16)
            return 16;
        else if (mb_type == P_L0_L0_16x8)
            return 8;
        else if (mb_type == P_L0_L0_8x16)
            return 16;
        else if (mb_type == P_8x8)
            return 8;
        else if (mb_type == P_8x8ref0)
            return 8;
        else
            return 16;
    }
    else if (Slice.slice_type == SLICE_TYPE_B) {
        for (i = 0; i < ARRAY_SIZE(Table_7_14); i++) {
            if (mb_type == Table_7_14[i].mb_type) {
                return Table_7_14[i].MbPartHeight;
            }
        }
        assert(0);
    }

    assert(0);
    return 0;
}

// Table 7-17
int SubMbPartWidth(int sub_mb_type)
{
    if (Slice.slice_type == SLICE_TYPE_P) {
        if (P_L0_8x8 == sub_mb_type)
            return 8;
        else if (P_L0_8x4 == sub_mb_type)
            return 8;
        else if (P_L0_4x8 == sub_mb_type)
            return 4;
        else if (P_L0_4x4 == sub_mb_type)
            return 4;
        else
            assert(0);
    }
    else if (Slice.slice_type == SLICE_TYPE_B) {
        for (int i = 0; i < ARRAY_SIZE(Table_7_18); i++) {
            if (sub_mb_type == Table_7_18[i].sub_mb_type) {
                return Table_7_18[i].SubMbPartWidth;
            }
        }
        assert(0);
    }

    assert(0);
    return 0;
}


// Table 7-17
int SubMbPartHeight(int sub_mb_type)
{
    if (Slice.slice_type == SLICE_TYPE_P) {
        if (P_L0_8x8 == sub_mb_type)
            return 8;
        else if (P_L0_8x4 == sub_mb_type)
            return 4;
        else if (P_L0_4x8 == sub_mb_type)
            return 8;
        else if (P_L0_4x4 == sub_mb_type)
            return 4;
        else
            assert(0);
    }
    else if (Slice.slice_type == SLICE_TYPE_B) {
        for (int i = 0; i < ARRAY_SIZE(Table_7_18); i++) {
            if (sub_mb_type == Table_7_18[i].sub_mb_type) {
                return Table_7_18[i].SubMbPartHeight;
            }
        }
        assert(0);
    }

    assert(0);
    return 0;
}