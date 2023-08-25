#include "h264.h"

extern void macroblock_parse(void);

struct Slice_t Slice;
struct Slice_t SliceRef[REF_PIC_MAX];
struct H264_t H264;

void sub_mb_pred(void)
{
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];
    int mvpLX[2];

    // Step 1: => sub_mb_type[4]

    for (MB->mbPartIdx = 0; MB->mbPartIdx < 4; MB->mbPartIdx++) {
        MB->sub_mb_type[MB->mbPartIdx] = read_ae(SE_sub_mb_type);

        if (Slice.slice_type == SLICE_TYPE_P) {
            MB->sub_mb_type[MB->mbPartIdx] += P_L0_8x8;
        }
        else if (Slice.slice_type == SLICE_TYPE_B) {
            MB->sub_mb_type[MB->mbPartIdx] += B_Direct_8x8;
        }
    }

    // Step 2: => ref_idx_l0[4];

    for (MB->mbPartIdx = 0; MB->mbPartIdx < 4; MB->mbPartIdx++) {
        if (((Slice.num_ref_idx_l0_active_minus1 > 0) || (Slice.mb_field_decoding_flag != Slice.field_pic_flag)) &&
            (MB->mb_type != P_8x8ref0) &&
            (MB->sub_mb_type[MB->mbPartIdx] != B_Direct_8x8) &&
            (SubMbPredMode(MB->sub_mb_type[MB->mbPartIdx]) != Pred_L1)) {
            MB->ref_idx_l0[MB->mbPartIdx] = read_ae(SE_ref_idx_lx);
        }
    }

    // Step 3: => ref_idx_l1[4];

    for (MB->mbPartIdx = 0; MB->mbPartIdx < 4; MB->mbPartIdx++) {
        if (((Slice.num_ref_idx_l1_active_minus1 > 0) || (Slice.mb_field_decoding_flag != Slice.field_pic_flag)) &&
            (MB->mb_type != P_8x8ref0) &&
            (MB->sub_mb_type[MB->mbPartIdx] != B_Direct_8x8) &&
            (SubMbPredMode(MB->sub_mb_type[MB->mbPartIdx]) != Pred_L0)) {
            MB->ref_idx_l1[MB->mbPartIdx] = read_ae(SE_ref_idx_lx);
        }
    }

    // Step 4: => mvd_l0[4][4][2]

    log2file("cabac.txt", "=== MVD ===\n");

    MB->listSuffixFlag = 0;

    if (Slice.CurrMbAddr == 155) {
        printf("DEBUG\n");
    }

    for (MB->mbPartIdx = 0; MB->mbPartIdx < 4; MB->mbPartIdx++) {
        if ((MB->sub_mb_type[MB->mbPartIdx] != B_Direct_8x8) && (SubMbPredMode(MB->sub_mb_type[MB->mbPartIdx]) != Pred_L1)) {
            for (MB->subMbPartIdx = 0; MB->subMbPartIdx < NumSubMbPart(MB->sub_mb_type[MB->mbPartIdx]); MB->subMbPartIdx++) {
                MB->mvd_l0[MB->mbPartIdx][MB->subMbPartIdx][0] = read_ae(SE_mvd_lx0);
                MB->mvd_l0[MB->mbPartIdx][MB->subMbPartIdx][1] = read_ae(SE_mvd_lx1);

                Derivation_process_for_luma_motion_vector_prediction(
                    MB->mbPartIdx,
                    MB->subMbPartIdx,
                    MB->ref_idx_l0[MB->mbPartIdx],
                    MB->sub_mb_type[MB->mbPartIdx],
                    mvpLX);

                MB->mv_l0[MB->mbPartIdx][MB->subMbPartIdx][0] = MB->mvd_l0[MB->mbPartIdx][0][0] + mvpLX[0];
                MB->mv_l0[MB->mbPartIdx][MB->subMbPartIdx][1] = MB->mvd_l0[MB->mbPartIdx][0][1] + mvpLX[1];
            }
        }
    }

    // Step 5: => mvd_l1[4][4][2]

    MB->listSuffixFlag = 1;

    for (MB->mbPartIdx = 0; MB->mbPartIdx < 4; MB->mbPartIdx++) {
        if ((MB->sub_mb_type[MB->mbPartIdx] != B_Direct_8x8) && (SubMbPredMode(MB->sub_mb_type[MB->mbPartIdx]) != Pred_L0)) {
            for (MB->subMbPartIdx = 0; MB->subMbPartIdx < NumSubMbPart(MB->sub_mb_type[MB->mbPartIdx]); MB->subMbPartIdx++) {
                MB->mvd_l1[MB->mbPartIdx][MB->subMbPartIdx][0] = read_ae(SE_mvd_lx0);
                MB->mvd_l1[MB->mbPartIdx][MB->subMbPartIdx][1] = read_ae(SE_mvd_lx1);

                Derivation_process_for_luma_motion_vector_prediction(
                    MB->mbPartIdx,
                    MB->subMbPartIdx,
                    MB->ref_idx_l1[MB->mbPartIdx],
                    MB->sub_mb_type[MB->mbPartIdx],
                    mvpLX);

                MB->mv_l1[MB->mbPartIdx][MB->subMbPartIdx][0] = MB->mvd_l1[MB->mbPartIdx][0][0] + mvpLX[0];
                MB->mv_l1[MB->mbPartIdx][MB->subMbPartIdx][1] = MB->mvd_l1[MB->mbPartIdx][0][1] + mvpLX[1];
            }
        }
    }
}

static void ref_pic_list_mvc_modification(void)
{
    assert(0);
}

static void ref_pic_list_modification(void)
{
    int i;
    struct ref_pic_list_modification_t* rplm = &Slice.ref_pic_list_modification;

    memset(rplm, 0xFF, sizeof(struct ref_pic_list_modification_t));

    i = -1;
    if (((Slice.slice_type % 5) != 2) && ((Slice.slice_type % 5) != 4)) {
        rplm->ref_pic_list_modification_flag_l0 = read_u(1);
        if (rplm->ref_pic_list_modification_flag_l0) {
            do {
                i++;
                rplm->modification_of_pic_nums_idc[0][i] = read_ue();
                if ((rplm->modification_of_pic_nums_idc[0][i] == 0) ||
                    (rplm->modification_of_pic_nums_idc[0][i] == 1)) {
                    rplm->abs_diff_pic_num_minus1[0][i] = read_ue();
                }
                else if (rplm->modification_of_pic_nums_idc[0][i] == 2) {
                    rplm->long_term_pic_num[0][i] = read_ue();
                }
            } while (rplm->modification_of_pic_nums_idc[0][i] != 3);
        }
    }

    i = -1;
    if (Slice.slice_type % 5 == 1) {
        rplm->ref_pic_list_modification_flag_l1 = read_u(1);
        if (rplm->ref_pic_list_modification_flag_l1) {
            do {
                i++;
                rplm->modification_of_pic_nums_idc[1][i] = read_ue();
                if ((rplm->modification_of_pic_nums_idc[1][i] == 0) ||
                    (rplm->modification_of_pic_nums_idc[1][i] == 1)) {
                    rplm->abs_diff_pic_num_minus1[1][i] = read_ue();
                }
                else if (rplm->modification_of_pic_nums_idc[1][i] == 2) {
                    rplm->long_term_pic_num[1][i] = read_ue();
                }
            } while (rplm->modification_of_pic_nums_idc[1][i] != 3);
        }
    }
}

static void pred_weight_table(void)
{
    int i, j;
    struct pred_weight_table_t* pwt = &Slice.pred_weight_table;

    pwt->luma_log2_weight_denom = read_ue();

    if (SPS.ChromaArrayType != 0) {
        pwt->chroma_log2_weight_denom = read_ue();
    }

    for (i = 0; i <= Slice.num_ref_idx_l0_active_minus1; i++) {
        pwt->luma_weight_l0_flag[i] = read_u(1);
        if (pwt->luma_weight_l0_flag[i]) {
            pwt->luma_weight_l0[i] = read_se();
            pwt->luma_offset_l0[i] = read_se();
        }
        if (SPS.ChromaArrayType != 0) {
            pwt->chroma_weight_l0_flag[i] = read_u(1);
            if (pwt->chroma_weight_l0_flag[i]) {
                for (j = 0; j < 2; j++) {
                    pwt->chroma_weight_l0[i][j] = read_se();
                    pwt->chroma_offset_l0[i][j] = read_se();
                }
            }
        }
    }

    if (Slice.slice_type % 5 == 1) {
        for (i = 0; i <= Slice.num_ref_idx_l1_active_minus1; i++) {
            pwt->luma_weight_l1_flag[i] = read_u(1);
            if (pwt->luma_weight_l1_flag[i]) {
                pwt->luma_weight_l1[i] = read_se();
                pwt->luma_offset_l1[i] = read_se();
            }
            if (SPS.ChromaArrayType != 0) {
                pwt->chroma_weight_l1_flag[i] = read_u(1);
                if (pwt->chroma_weight_l1_flag[i]) {
                    for (j = 0; j < 2; j++) {
                        pwt->chroma_weight_l1[i][j] = read_se();
                        pwt->chroma_offset_l1[i][j] = read_se();
                    }
                }
            }
        }
    }
}

static void dec_ref_pic_marking(void)
{
    int i = -1;
    struct dec_ref_pic_marking_t* drpm = &Slice.dec_ref_pic_marking;

    memset(drpm, 0xFF, sizeof(struct dec_ref_pic_marking_t));

    if (Slice.IdrPicFlag) {
        drpm->no_output_of_prior_pics_flag = read_u(1);
        drpm->long_term_reference_flag = read_u(1);
    }
    else {
        drpm->adaptive_ref_pic_marking_mode_flag = read_u(1);
        if (drpm->adaptive_ref_pic_marking_mode_flag) {
            do {
                i++;
                drpm->memory_management_control_operation[i] = read_ue();
                if ((drpm->memory_management_control_operation[i] == 1) ||
                    (drpm->memory_management_control_operation[i] == 3)) {
                    drpm->difference_of_pic_nums_minus1[i] = read_ue();
                }
                if (drpm->memory_management_control_operation[i] == 2) {
                    drpm->long_term_pic_num[i] = read_ue();
                }
                if ((drpm->memory_management_control_operation[i] == 3) ||
                    (drpm->memory_management_control_operation[i] == 6)) {
                    drpm->long_term_frame_idx[i] = read_ue();
                }
                if (drpm->memory_management_control_operation[i] == 4) {
                    drpm->max_long_term_frame_idx_plus1[i] = read_ue();
                }
            } while (drpm->memory_management_control_operation[i] != 0);
        }
    }
}

void Slice_Header_Parse(void)
{
    Slice.first_mb_in_slice = read_ue();
    Slice.slice_type = read_ue() % 5;
    Slice.pic_parameter_set_id = read_ue();
    Slice.frame_num = read_u(SPS.log2_max_frame_num_minus4 + 4);

    if (Slice.IdrPicFlag) {
        Slice.idr_pic_id = read_ue();
    }

    if (SPS.pic_order_cnt_type == 0) {
        Slice.pic_order_cnt_lsb = read_u(SPS.log2_max_pic_order_cnt_lsb_minus4 + 4);
    }

    // if( pic_order_cnt_type = = 1 && !delta_pic_order_always_zero_flag ) {

    // if( redundant_pic_cnt_present_flag )

    if (Slice.slice_type == SLICE_TYPE_B) {
        Slice.direct_spatial_mv_pred_flag = read_u(1);
    }

    if ((SLICE_TYPE_P == Slice.slice_type) ||
        (SLICE_TYPE_SP == Slice.slice_type) ||
        (SLICE_TYPE_B == Slice.slice_type)) {
        Slice.num_ref_idx_active_override_flag = read_u(1);
        if (Slice.num_ref_idx_active_override_flag) {
            Slice.num_ref_idx_l0_active_minus1 = read_ue();
            if (Slice.slice_type == SLICE_TYPE_B) {
                Slice.num_ref_idx_l1_active_minus1 = read_ue();
            }
        }
    }

    if ((nalu_type == 20) || (nalu_type == 21)) {
        ref_pic_list_mvc_modification();
    }
    else {
        ref_pic_list_modification();
    }

    if ((PPS.weighted_pred_flag && ((Slice.slice_type == SLICE_TYPE_P) || (Slice.slice_type == SLICE_TYPE_SP))) ||
        ((PPS.weighted_bipred_idc == 1) && (Slice.slice_type == SLICE_TYPE_P))) {
        pred_weight_table();
    }
    
    if (nal_ref_idc) {
        dec_ref_pic_marking();
    }

    if (PPS.entropy_coding_mode_flag && (Slice.slice_type != SLICE_TYPE_I) && (Slice.slice_type != SLICE_TYPE_SI)) {
        Slice.cabac_init_idc = read_ue();
        assert(0 == Slice.cabac_init_idc);
    }

    Slice.slice_qp_delta = read_se();

    if (PPS.deblocking_filter_control_present_flag) {
        Slice.disable_deblocking_filter_idc = read_ue();
        if (Slice.disable_deblocking_filter_idc != -1) {
            Slice.slice_alpha_c0_offset_div2 = read_se();
            Slice.slice_beta_offset_div2 = read_se();
        }
    }
}

static unsigned int NextMbAddress(unsigned int addr)
{
    return (addr + 1);
}

// The same name of JM
static void pad_buf(void)
{
    int i, j;

    // Fill the center area
    for (j = 0; j < HEIGHT; j++) {
        memcpy(&Luma_Pad[H_PAD + j][W_PAD], Luma[j], WIDTH);
    }

    // Fill the padding of first & last line, to prepare fill the top area
    memset(&Luma_Pad[H_PAD][0], Luma_Pad[H_PAD][W_PAD], W_PAD);
    memset(&Luma_Pad[H_PAD][W_PAD + WIDTH], Luma_Pad[H_PAD][W_PAD + WIDTH - 1], W_PAD);

    // Fill the top area
    for (j = 0; j < H_PAD; j++) {
        memcpy(Luma_Pad[j], Luma_Pad[H_PAD], WIDTH + 2 * W_PAD);
    }

    // Fill the bottom area
    for (j = H_PAD + HEIGHT; j < H_PAD + HEIGHT + H_PAD; j++) {
        memcpy(Luma_Pad[j], Luma_Pad[H_PAD + HEIGHT], WIDTH + 2 * W_PAD);
    }

    // Fill the left & right area
    for (j = H_PAD; j < H_PAD + HEIGHT; j++) {
        memset(&Luma_Pad[j][0], Luma_Pad[j][W_PAD], W_PAD);
        memset(&Luma_Pad[j][W_PAD + WIDTH], Luma_Pad[j][W_PAD + WIDTH - 1], W_PAD);
    }
}

void Slice_Data_Parse(void)
{
    int i;
    int value;

    if (PPS.entropy_coding_mode_flag) {
        while (!bitstream_is_align()) {
            value = bitstream_read(1);
            assert(value == 1);
        }
    }

    // TODO: if skip, not 0
    Slice.CurrMbAddr = 0;

    Slice.moreDataFlag = 1;
    Slice.prevMbSkipped = 0;

    // Dequant Related
    Slice.SliceQPY = 26 + PPS.pic_init_qp_minus26 + Slice.slice_qp_delta; /* (7-30) */

    /* 'For the first macroblock in the slice QPY,PREV is initially set equal to SliceQPY' */
    Slice.QPYprev = Slice.SliceQPY;

    // CABAC exist in Slice Data
    Initialisation_process_for_context_variables();
    Initialisation_process_for_the_arithmetic_decoding_engine();

    Slice.mb_field_decoding_flag = 0;

    for (i = 0; i < SLICE_MAX_MB; i++) {
        Slice.MB[i].mbAddrA = -1;
        Slice.MB[i].mbAddrB = -1;
        Slice.MB[i].mbAddrC = -1;
        Slice.MB[i].mbAddrD = -1;
    }

    memset(Luma, 0xFF, sizeof(Luma));

    do {
        log2file("cabac.txt", "== MB [%d] ==\n", Slice.CurrMbAddr);

        Derivation_process_for_neighbouring_macroblock_addresses_and_their_availability();

        if (34 == Slice.CurrMbAddr) {
            printf("DEBUG\n");
        }

        if ((Slice.slice_type != SLICE_TYPE_I) && (Slice.slice_type != SLICE_TYPE_SI)) {
            if (! PPS.entropy_coding_mode_flag) {
                assert(0);
            }
            else {
                Slice.MB[Slice.CurrMbAddr].mb_skip_flag = read_ae(SE_mb_skip_flag);
                
                // TODO
                if (Slice.slice_type == SLICE_TYPE_B) {
                    Slice.MB[Slice.CurrMbAddr].mb_type = B_Skip;
                }
                else if (Slice.slice_type == SLICE_TYPE_P) {
                    Slice.MB[Slice.CurrMbAddr].mb_type = P_Skip;
                }

                Slice.moreDataFlag = !Slice.MB[Slice.CurrMbAddr].mb_skip_flag;
            }
        }

        if (Slice.moreDataFlag) {
#ifdef DEBUG
            printf("======================================================\n");
#endif
            macroblock_parse();
        }

        if (! PPS.entropy_coding_mode_flag) {
        }
        else {
            if (Slice.slice_type != SLICE_TYPE_I && Slice.slice_type != SLICE_TYPE_SI) {
                Slice.prevMbSkipped = Slice.MB[Slice.CurrMbAddr].mb_skip_flag;
            }

            if (Slice.MbaffFrameFlag && (Slice.CurrMbAddr % 2 == 0)) {

            }
            else {
                Slice.end_of_slice_flag = read_ae(SE_end_of_slice_flag);
                Slice.moreDataFlag = !Slice.end_of_slice_flag;
            }
        }

        Slice.CurrMbAddr = NextMbAddress(Slice.CurrMbAddr);
    } while (Slice.moreDataFlag);

    pad_buf();

#if 1
    static FILE* fp = NULL;
    if (NULL == fp) {
        fp = fopen("dump.yuv", "wb");
    }
    if (fp) {
        fwrite(Luma, 1, sizeof(Luma), fp);
        fflush(fp);
    }
#endif
}

void slice_parse(unsigned char* buf, int len)
{
    static int count = 0;

    count++;

    bitstream_init(buf, len);

    memset(&Slice, 0, sizeof(struct Slice_t));

    if (nalu_type == 5) {
        Slice.IdrPicFlag = 1;
    }

    // SliceHeader Parse
    Slice_Header_Parse();

    if (Slice.slice_type != SLICE_TYPE_P) {
        return;
    }

    // Slice Data Parse
    Slice_Data_Parse();

    printf("Stop Here\n");
    while (1) {
        Sleep(1000);
    }
}