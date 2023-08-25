#include "h264.h"

void mb_pred(enum SyntaxElementType_e seType)
{
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];
    int luma4x4BlkIdx;
    int mvpLX[2];

    if ((MB->MbPartPredMode[0] == Intra_4x4) ||
        (MB->MbPartPredMode[0] == Intra_8x8) ||
        (MB->MbPartPredMode[0] == Intra_16x16)) {
        if (MB->MbPartPredMode[0] == Intra_4x4) {
            for (luma4x4BlkIdx = 0; luma4x4BlkIdx < 16; luma4x4BlkIdx++) {
                MB->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx] = read_ae(SE_prev_intra_pred_mode_flag);
                if (!MB->prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]) {
                    MB->rem_intra4x4_pred_mode[luma4x4BlkIdx] = read_ae(SE_rem_intra_pred_mode);
                }
            }
        }
        else if (MB->MbPartPredMode[0] == Intra_8x8) {
            // ignore
        }

        if (1) { // ChromaArrayType == 1
            MB->intra_chroma_pred_mode = read_ae(SE_intra_chroma_pred_mode);
        }

    }
    else if (MB->MbPartPredMode[0] != Direct) {
        for (MB->mbPartIdx = 0; MB->mbPartIdx < NumMbPart(MB->mb_type); MB->mbPartIdx++) {
            if ((Slice.num_ref_idx_l0_active_minus1 > 0 ||
                Slice.mb_field_decoding_flag != Slice.field_pic_flag) &&
                MbPartPredMode(MB->mb_type, MB->mbPartIdx) != Pred_L1) {
                MB->ref_idx_l0[MB->mbPartIdx] = read_ae(SE_ref_idx_lx);
            }
        }
        for (MB->mbPartIdx = 0; MB->mbPartIdx < NumMbPart(MB->mb_type); MB->mbPartIdx++) {
            if ((Slice.num_ref_idx_l1_active_minus1 > 0 ||
                Slice.mb_field_decoding_flag != Slice.field_pic_flag) &&
                MbPartPredMode(MB->mb_type, MB->mbPartIdx) != Pred_L0) {
                MB->ref_idx_l1[MB->mbPartIdx] = read_ae(SE_ref_idx_lx);
            }
        }

        log2file("cabac.txt", "=== MVD ===\n");

        MB->listSuffixFlag = 0;
        
        for (MB->mbPartIdx = 0; MB->mbPartIdx < NumMbPart(MB->mb_type); MB->mbPartIdx++) {
            if (MbPartPredMode(MB->mb_type, MB->mbPartIdx) != Pred_L1) {
                MB->mvd_l0[MB->mbPartIdx][0][0] = read_ae(SE_mvd_lx0);
                MB->mvd_l0[MB->mbPartIdx][0][1] = read_ae(SE_mvd_lx1);

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

        MB->listSuffixFlag = 1;
        
        for (MB->mbPartIdx = 0; MB->mbPartIdx < NumMbPart(MB->mb_type); MB->mbPartIdx++) {
            if (MbPartPredMode(MB->mb_type, MB->mbPartIdx) != Pred_L0) {
                MB->mvd_l1[MB->mbPartIdx][0][0] = read_ae(SE_mvd_lx0);
                MB->mvd_l1[MB->mbPartIdx][0][1] = read_ae(SE_mvd_lx1);

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