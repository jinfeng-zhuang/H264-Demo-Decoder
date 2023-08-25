#include "h264.h"

struct PPS_t PPS;

void PPS_parse(unsigned char* buf)
{
    PPS.entropy_coding_mode_flag = 1;
    PPS.pic_init_qp_minus26 = -3;
    PPS.transform_8x8_mode_flag = 0;
    PPS.weighted_pred_flag = 1;
    PPS.weighted_bipred_idc = 2;
    PPS.deblocking_filter_control_present_flag = 1;
}
