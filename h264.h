#ifndef H264_H
#define H264_H

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "Mathematical.h"

#define DUMP_MVD
#define DEBUG_CABAC

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef unsigned char bool;
#define false 0
#define true 1

#define NA  (-1)

#define H_PAD   (12)
#define W_PAD   (32)
#define WIDTH (1920)
#define HEIGHT (1088)
#define WIDTH_PAD     (1920 + W_PAD*2)
#define HEIGHT_PAD    (1088 + H_PAD*2)
#define SLICE_SIZE    (WIDTH*HEIGHT)
#define SLICE_MAX_MB (SLICE_SIZE/16/16)

#define FAILURE 0
#define SUCCESS 1

enum NALU_e {
    NALU_NON_IDR = 1,
    NALU_PART_A = 2,
    NALU_PART_B = 3,
    NALU_PART_C = 4,
    NALU_IDR = 5,
    NALU_SPS = 7,
    NALU_PPS = 8,
    NALU_SEI = 6,
};

#define REF_PIC_MAX   (3)

enum SliceType_e {
    SLICE_TYPE_P = 0,
    SLICE_TYPE_B,
    SLICE_TYPE_I,
    SLICE_TYPE_SP,
    SLICE_TYPE_SI,
};

enum SyntaxElementType_e
{
    SE_mb_type = 0,
    SE_mb_skip_flag,
    SE_sub_mb_type,
    SE_mvd_lx0,
    SE_mvd_lx1, // 4
    SE_ref_idx_lx,
    SE_mb_qp_delta,
    SE_intra_chroma_pred_mode,
    SE_prev_intra_pred_mode_flag, // 8
    SE_rem_intra_pred_mode,
    SE_mb_field_decoding_flag,
    SE_coded_block_pattern,
    SE_coded_block_flag, // 12
    SE_significant_coeff_flag,
    SE_last_significant_coeff_flag,
    SE_coeff_abs_level_minus1,
    SE_coeff_sign_flag, // 16
    SE_transform_size_8x8_flag,
    SE_end_of_slice_flag
};

enum BlockType_e
{
    blk_LUMA_8x8      = 0,
    blk_LUMA_4x4      = 1,
    blk_LUMA_16x16_DC = 2,
    blk_LUMA_16x16_AC = 3,
    blk_CHROMA_DC_Cb  = 4,
    blk_CHROMA_DC_Cr  = 5,
    blk_CHROMA_AC_Cb  = 6,
    blk_CHROMA_AC_Cr  = 7,

    blk_UNKNOWN       = 999
};

enum mb_type_I
{
    I_NxN             = 0,
    I_16x16_0_0_0     = 1,
    I_16x16_1_0_0     = 2,
    I_16x16_2_0_0     = 3,
    I_16x16_3_0_0     = 4,
    I_16x16_0_1_0     = 5,
    I_16x16_1_1_0     = 6,
    I_16x16_2_1_0     = 7,
    I_16x16_3_1_0     = 8,
    I_16x16_0_2_0     = 9,
    I_16x16_1_2_0     = 10,
    I_16x16_2_2_0     = 11,
    I_16x16_3_2_0     = 12,
    I_16x16_0_0_1     = 13,
    I_16x16_1_0_1     = 14,
    I_16x16_2_0_1     = 15,
    I_16x16_3_0_1     = 16,
    I_16x16_0_1_1     = 17,
    I_16x16_1_1_1     = 18,
    I_16x16_2_1_1     = 19,
    I_16x16_3_1_1     = 20,
    I_16x16_0_2_1     = 21,
    I_16x16_1_2_1     = 22,
    I_16x16_2_2_1     = 23,
    I_16x16_3_2_1     = 24,
    I_PCM             = 25
};

enum mb_type_P
{
    P_L0_16x16 = 26,     // Refer to list 0
    P_L0_L0_16x8,
    P_L0_L0_8x16,
    P_8x8,
    P_8x8ref0,
    P_Skip // can't be get by read_ae(), derived by SE.mb_skip_flag
};

enum sub_mb_type_P
{
    P_L0_8x8 = 32,
    P_L0_8x4,
    P_L0_4x8,
    P_L0_4x4
};

enum mb_type_B
{
    B_Direct_16x16 = 36,    // no ref list
    B_L0_16x16,             // ref to list 0
    B_L1_16x16,             // ref to list 1
    B_Bi_16x16,             // ref to list 0 & list 1
    B_L0_L0_16x8,
    B_L0_L0_8x16,
    B_L1_L1_16x8,
    B_L1_L1_8x16,
    B_L0_L1_16x8,
    B_L0_L1_8x16,
    B_L1_L0_16x8,
    B_L1_L0_8x16,
    B_L0_Bi_16x8,
    B_L0_Bi_8x16,
    B_L1_Bi_16x8,
    B_L1_Bi_8x16,
    B_Bi_L0_16x8,
    B_Bi_L0_8x16,
    B_Bi_L1_16x8,
    B_Bi_L1_8x16,
    B_Bi_Bi_16x8,
    B_Bi_Bi_8x16,
    B_8x8,
    B_Skip  // 23, Attention: B_Skip and P_Skip set by 'mb_skip_flag'
};

enum sub_mb_type_B
{
    B_Direct_8x8 = 60,
    B_L0_8x8,
    B_L1_8x8,
    B_Bi_8x8,
    B_L0_8x4,
    B_L0_4x8,
    B_L1_8x4,
    B_L1_4x8,
    B_Bi_8x4,
    B_Bi_4x8,
    B_L0_4x4,
    B_L1_4x4,
    B_Bi_4x4  // 12
};

// Also means plane in JM
enum {
    Component_Y,
    Component_Cb,
    Component_Cr
};

enum MbPartPredMode_Luma_e {
    NoPrediction = 0,
    Intra_4x4 = 1, // I frame
    Intra_8x8 = 2,
    Intra_16x16 = 3,
    Direct = 4, // P and B frame
    Pred_L0 = 5,
    Pred_L1 = 6,
    BiPred = 7
};

// Table 8-2: Specification of Intra4x4PredMode[luma4x4BlkIdx] and associated names
enum Intra4x4PredMode_e {
    Intra_4x4_PRED_MODE_Vertical,
    Intra_4x4_PRED_MODE_Horizontal,
    Intra_4x4_PRED_MODE_DC,
    Intra_4x4_PRED_MODE_Diagonal_Down_Left,
    Intra_4x4_PRED_MODE_Diagonal_Down_Right,
    Intra_4x4_PRED_MODE_Vertical_Right,
    Intra_4x4_PRED_MODE_Horizontal_Down,
    Intra_4x4_PRED_MODE_Vertical_Left,
    Intra_4x4_PRED_MODE_Horizontal_Up
};

// Table 8-3
enum Intra16x16PredMode_e {
    Intra_16x16_Vertical,
    Intra_16x16_Horizontal,
    Intra_16x16_DC,
    Intra_16x16_Plane
};

struct SPS_t {
    int profile_idc;
    int MbWidthC;
    int MbHeightC;
    int BitDepthY;
    int SubWidthC;
    int SubHeightC;
    int NumC8x8;

    int chroma_format_idc;
    int separate_colour_plane_flag;
    int ChromaArrayType;

    int frame_mbs_only_flag;

    int pic_order_cnt_type;

    int qpprime_y_zero_transform_bypass_flag;

    int QpBdOffsetY; /* QpBdOffsetY = 6 * bit_depth_luma_minus8 (Foluma 7-4) */

    int ScalingMatrix4x4[6][4][4];
    int LevelScale4x4[3][6][4][4]; // [Y/Cb/Cr][Qstep6][4x4]

    int log2_max_frame_num_minus4;
    int log2_max_pic_order_cnt_lsb_minus4;

    int direct_8x8_inference_flag; // for B_Skip, B_Direct luma MV derivation

    int pic_width_in_mbs_minus1;
    int PicWidthInMbs;

    int width;
    int height;
};

struct PPS_t {
    int pic_parameter_set_id;
    int entropy_coding_mode_flag;
    int pic_init_qp_minus26;
    int pic_init_qs_minus26;
    int chroma_qp_index_offset;
    int constrained_intra_pred_flag;
    int transform_8x8_mode_flag;
    int weighted_pred_flag;
    int weighted_bipred_idc;
    int deblocking_filter_control_present_flag;
};

struct MacroBlock_t {
    int mb_skip_flag;

    int mb_type;
    int sub_mb_type[4];
    int noSubMbPartSizeLessThan8x8Flag;

    int is_field;

    int mbPartIdx;
    int subMbPartIdx;

    //==============================================
    // Neighbour
    //==============================================
    int mbAddrA;
    int mbAddrB;
    int mbAddrC;
    int mbAddrD;

    //==============================================
    // Prediction
    //==============================================
    
    /* 
     * MbPartPredMode Macro valid for Both I/P/B, see MbPartPredMode_Luma_e
     * But Part = Partition, only valid for P/B:
     * 16x16, 16x8, 8x16, 8x8
     * I slice use this macro to specify 4x4 or 8x8.
     */
    int MbPartPredMode[4];
    int NumMbPart;

    // 4x4 mode
    int prev_intra4x4_pred_mode_flag[16];
    int rem_intra4x4_pred_mode[16];
    int Intra4x4PredMode[16];
    int intra_chroma_pred_mode;

    // 8x8 mode
    int Intra8x8PredMode[4];

    // 16x16 mode
    int Intra16x16PredMode;

    int ref_idx_l0[4];
    int ref_idx_l1[4];
    int mvd_l0[4][4][2]; // Part <= 4, SubPart <=4, X/Y offset = 2
    int mvd_l1[4][4][2]; // Part <= 4, SubPart <=4, X/Y offset = 2

    int mv_l0[4][4][2];
    int mv_l1[4][4][2];

    int listSuffixFlag;

    // Prediction Result
    int predL[16][16]; // Predition Values of Luma
    int predCb[8][8]; // Predition Values of Cb
    int predCr[8][8]; // Predition Values of Cr

    //==============================================
    // Residual
    //==============================================
    int coded_block_pattern;
    int CodedBlockPatternLuma;
    int CodedBlockPatternChroma;
    int levelListIdx;   // The index of coeff list
    int numDecodAbsLevelEq1;
    int numDecodAbsLevelGt1;
    int coded_block_flag[3][17]; // [YCbCr][blkIdx]
    int transform_size_8x8_flag;
    int i16x16DClevel[16];
    int i16x16AClevel[16][15];
    int level4x4[16][16];
    int level8x8[4][64];
    int ChromaACLevel[2][4][15]; // Cb & Cr, Only support 4:2:0
    int ChromaDCLevel[2][16];

    int residual[16][16];

    int TransformBypassModeFlag;

    //==============================================
    // Dequant
    //==============================================
    int mb_qp_delta;
    int QPY;
    int QPC;
    int qP; // The final

    //==============================================
    // Constructed YCbCr = Residual + Prediction
    //==============================================
    int SprimeL[16][16];
    int SprimeCb[8][8];
    int SprimeCr[8][8];
};

struct binarization_t
{
    int SyntaxElementValue;

    int maxBinIdxCtx;
    int ctxIdxOffset;         //!< The context index offset, specifies the lower value of the range of ctxIdx.
    bool bypassFlag;          //!< Indicate if the DecodeBypass() process must be used.

    unsigned char** bintable; //!< The binarization table we are gonna use to run decoded bins against.
    int bintable_x;           //!< Width of our binarization table.
    int bintable_y;           //!< Height of our binarization table.
};

struct CabacCtx_t
{
    unsigned char pStateIdx[460]; //!< Corresponds to a probability state index
    unsigned char valMPS[460];    //!< Corresponds to the value of the most probable symbol

    // The status of the arithmetic decoding engine is represented by the variables codIRange and codIOffset.
    unsigned short codIRange;
    unsigned short codIOffset;
};

struct ref_pic_list_modification_t {
    int ref_pic_list_modification_flag_l0;
    int ref_pic_list_modification_flag_l1;
    int modification_of_pic_nums_idc[2][REF_PIC_MAX];
    int abs_diff_pic_num_minus1[2][REF_PIC_MAX];
    int long_term_pic_num[2][REF_PIC_MAX];
};

struct pred_weight_table_t {
    int luma_log2_weight_denom;
    int chroma_log2_weight_denom;
    int luma_weight_l0_flag[REF_PIC_MAX];
    int luma_weight_l0[REF_PIC_MAX];
    int luma_offset_l0[REF_PIC_MAX];
    int chroma_weight_l0_flag[REF_PIC_MAX];
    int chroma_weight_l0[REF_PIC_MAX][2];
    int chroma_offset_l0[REF_PIC_MAX][2];

    int luma_weight_l1_flag[REF_PIC_MAX];
    int luma_weight_l1[REF_PIC_MAX];
    int luma_offset_l1[REF_PIC_MAX];
    int chroma_weight_l1_flag[REF_PIC_MAX];
    int chroma_weight_l1[REF_PIC_MAX][2];
    int chroma_offset_l1[REF_PIC_MAX][2];
};

struct dec_ref_pic_marking_t {
    // if( IdrPicFlag ) {
    int no_output_of_prior_pics_flag;   // u(1)
    int long_term_reference_flag;       // u(1)
    // else {
    int adaptive_ref_pic_marking_mode_flag;
    // if( adaptive_ref_pic_marking_mode_flag ) {
    // do {
    int memory_management_control_operation[REF_PIC_MAX];

    int difference_of_pic_nums_minus1[REF_PIC_MAX];
    int long_term_pic_num[REF_PIC_MAX];
    int long_term_frame_idx[REF_PIC_MAX];
    int max_long_term_frame_idx_plus1[REF_PIC_MAX];
    // } while( memory_management_control_operation != 0 )
    // }
    // }
};

struct Slice_t {
    struct CabacCtx_t CabacCtx;

    //===============================================
    // Slice Raw Data
    //===============================================
    unsigned char buffer[32];
    int buffer_length;

    //===============================================
    // Slice Header
    //===============================================

    int first_mb_in_slice;              // ue(v)
    enum SliceType_e slice_type;        // ue(v)
    int pic_parameter_set_id;           // ue(v)
    int frame_num;                      // u(v)

    int idr_pic_id;                     // ue(v)
    int pic_order_cnt_lsb;              // u(v)

    int direct_spatial_mv_pred_flag;

    int field_pic_flag;

    // if( slice_type = = P | | slice_type = = SP | | slice_type = = B ) {
    int num_ref_idx_active_override_flag;   // if below exist
    int num_ref_idx_l0_active_minus1;       // the max ref index
    int num_ref_idx_l1_active_minus1;       // the max ref index
    // }

    // if( nal_unit_type = = 20 | | nal_unit_type = = 21 )
    // else {
    struct ref_pic_list_modification_t ref_pic_list_modification;
    // }
    
    // if((weighted_pred_flag && (slice_type == P || SP )) || (weighted_bipred_idc ... B...) ) {
     struct pred_weight_table_t pred_weight_table;
    // }

    struct dec_ref_pic_marking_t dec_ref_pic_marking;

    // if( entropy_coding_mode_flag && slice_type != I && slice_type != SI )
    int cabac_init_idc;                 // ue(v)

    int slice_qp_delta;                 // se(v)

    // if( deblocking_filter_control_present_flag ) {
    int disable_deblocking_filter_idc;  // ue(v)
    // if( disable_deblocking_filter_idc != 1 ) {
    int slice_alpha_c0_offset_div2;     // se(v)
    int slice_beta_offset_div2;         // se(v)
    // }
    // }
    
    //===============================================
    // Slice Macros
    //===============================================
    int IdrPicFlag;
    int MbaffFrameFlag;

    //===============================================
    // Slice Data
    //===============================================

    int cabac_alignment_one_bit;
    int mb_field_decoding_flag;
    int end_of_slice_flag;

    int moreDataFlag;
    int prevMbSkipped;

    //===============================================
    // MB Layer
    //===============================================

    int CurrMbAddr;                 // The index of MB[N]
    struct MacroBlock_t MB[SLICE_MAX_MB];    // Up to 4K Resolution

    /* Dequant */
    int SliceQPY;                   /* Init @ first MB in slice */
    int QPYprev;                    /* Inherite in Slice */
	// unsigned char Luma[3840*2160];
};

struct bitstream_t {
    unsigned char* buf;
    unsigned int buflen;
    int bitpos;
};

struct H264_t {
    struct bitstream_t bs;
    int normAdjust4x4[6][4][4];
};

extern struct H264_t H264;
extern struct SPS_t SPS;
extern struct PPS_t PPS;
extern struct Slice_t Slice;
extern int nalu_type;
extern unsigned char Luma[SLICE_SIZE];
extern unsigned char Luma_Pad[HEIGHT_PAD][WIDTH_PAD];
extern int nal_ref_idc;

//============================================================================

extern int nalu_read(unsigned char* raw, unsigned char* nalu);
extern void Derivation_process_for_scaling_functions(void);
extern void Transformation_process_for_residual_4x4_blocks(const int d[4][4], int r[4][4]);
extern void Scaling_process_for_residual_4x4_blocks(int qp, int YCbCr, const int coeff[4][4], int dequant[4][4]);
extern void Inverse_scanning_process_for_4x4_transform(int coeff[16], int matrix[4][4]);
extern void transform_decoding_process_for_4x4_luma_residual_blocks(int luma4x4BlkIdx);
extern void Intra_4x4_prediction_process_for_luma_samples(int luma4x4BlkIdx);
extern int Intra_4x4_sample_prediction(const int luma4x4BlkIdx);
extern void Derivation_process_for_Intra4x4PredMode(int luma4x4BlkIdx);
extern int Derivation_process_for_4x4_luma_block_indices(const int xP, const int yP);
extern void Derivation_process_for_neighbouring_locations(
    const int isLumaBlock,
    const int xN, const int yN,
    int* mbAddrN,
    int* xW, int* yW
);
extern void Derivation_process_for_neighbouring_4x4_luma_blocks (
    const int luma4x4BlkIdx,
    int* mbAddrA, int* luma4x4BlkIdxA,
    int* mbAddrB, int* luma4x4BlkIdxB
);
extern void Derivation_process_for_neighbouring_8x8_luma_blocks(
    const int luma8x8BlkIdx,
    int* mbAddrA, int* luma8x8BlkIdxA,
    int* mbAddrB, int* luma8x8BlkIdxB);
extern void Inverse_4x4_luma_block_scanning_process(const int luma4x4BlkIdx, int* x, int* y);
extern int Derivation_process_for_ctxIdx(enum SyntaxElementType_e seType, enum BlockType_e blkType, const int blkIdx, const uint8_t decodedSE[32], int binIdx, const int maxBinIdxCtx, const int ctxIdxOffset);
extern int Derivation_process_for_8x8_luma_block_indices(const int xP, const int yP);
extern void Inverse_macroblock_partition_scanning_process(
    int mb_type, int mbPartIdx,
    int *x, int *y);
extern void Derivation_process_for_macroblock_and_sub_macroblock_partition_indices(
    int xP, int yP,
    int mbType,
    int subMbType[4],
    int *mbPartIdx, int *subMbPartIdx);

extern void Derivation_process_for_neighbouring_partitions(
    int mbPartIdx,                                      // Input
    int currSubMbType,                                  // Input
    int subMbPartIdx,                                   // Input
    int *mbAddrA, int *mbPartIdxA, int *subMbPartIdxA,  // Output
    int *mbAddrB, int *mbPartIdxB, int *subMbPartIdxB,  // Output
    int *mbAddrC, int *mbPartIdxC, int *subMbPartIdxC,  // Output
    int *mbAddrD, int *mbPartIdxD, int *subMbPartIdxD   // Output
    );

extern void Derivation_process_for_neighbouring_4x4_chroma_blocks(
    const int luma4x4BlkIdx,
    int* mbAddrA, int* luma4x4BlkIdxA,
    int* mbAddrB, int* luma4x4BlkIdxB
    );

extern void Inverse_sub_macroblock_partition_scanning_process(int mbPartIdx, int subMbPartIdx, int* x, int* y);

extern void Derivation_process_for_luma_motion_vector_prediction(
    // Input
    int mbPartIdx,
    int subMbPartIdx,
    int refIdxLX,
    int currSubMbType,

    // Output
    int *mvpLX
    );

extern void Derivation_process_for_neighbouring_macroblock_addresses_and_their_availability(void);

extern void transform_decoding_process_for_luma_samples_of_Intra_16x16_macroblock_prediction_mode(void);

extern void Intra_16x16_prediction_process_for_luma_samples(void);

extern void Scaling_and_transformation_process_for_DC_transform_coefficients_for_Intra_16x16_macroblock_type(
    int c[4][4], int dcY[4][4]);

extern void SPS_parse(unsigned char* buf);
extern void PPS_parse(unsigned char* buf);
extern void slice_parse(unsigned char* buf, int len);

extern void mb_pred(enum SyntaxElementType_e seType);

extern void bitstream_init(unsigned char* buf, unsigned int len);
extern unsigned int bitstream_read(int count);
extern int bitstream_is_align(void);
extern void bitstream_status(void);

extern void Initialisation_process_for_context_variables(void);
extern void Initialisation_process_for_the_arithmetic_decoding_engine(void);

extern int read_ae(enum SyntaxElementType_e seType);
extern int read_ae_blk(enum SyntaxElementType_e seType, enum BlockType_e blkType, const int blkIdx);
extern int read_se(void);
extern unsigned int read_ue(void);
extern unsigned int read_u(int n);

extern int cabac_process(
    enum SyntaxElementType_e seType,
    enum BlockType_e blkType,
    const int blkIdx,
    struct binarization_t* bin);

extern unsigned char DecodeBypass(void);

extern int binarization_get(enum SyntaxElementType_e seType,
    enum BlockType_e blkType,
    struct binarization_t* prefix,
    struct binarization_t* suffix);
extern void residual(int startIdx, int endIdx);

// Table 7-13 ~ Table 7-18
extern int NumMbPart(int mb_type);
extern int NumSubMbPart(int sub_mb_type);
extern int MbPartPredMode(int mb_type, int mbPartIdx);
extern int SubMbPredMode(int sub_mb_type);
extern int MbPartWidth(int mb_type);
extern int MbPartHeight(int mb_type);
extern int SubMbPartWidth(int sub_mb_type);
extern int SubMbPartHeight(int sub_mb_type);

extern const unsigned char binarization_eg0[1023][19];
extern const unsigned char binarization_u[32][32];
extern const unsigned char binarization_tu2[3][2];
extern const unsigned char binarization_tu3[4][3];
extern const unsigned char binarization_tu14[15][14];
extern const unsigned char binarization_fl1[2][1];
extern const unsigned char binarization_fl3[4][2];
extern const unsigned char binarization_fl7[8][3];
extern const unsigned char binarization_fl15[16][4];
extern const unsigned char binarization_mbtype_I[26][7];

extern void log2file(const char* filename, char* fmt, ...);

#endif
