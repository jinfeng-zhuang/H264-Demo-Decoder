static char *se_type_array[] = {
    "mb_type",
    "mb_skip_flag",
    "sub_mb_type",
    "mvd_lx0",
    "mvd_lx1",
    "ref_idx_lx",
    "mb_qp_delta",
    "intra_chroma_pred_mode",
    "prev_intra_pred_mode_flag",
    "rem_intra_pred_mode",
    "mb_field_decoding_flag",
    "coded_block_pattern",
    "coded_block_flag",
    "significant_coeff_flag",
    "last_significant_coeff_flag",
    "coeff_abs_level_minus1",
    "coeff_sign_flag",
    "transform_size_8x8_flag",
    "end_of_slice_flag"
};

char* se_type_str(int se_type)
{
    return se_type_array[se_type];
}