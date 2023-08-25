#include "h264.h"

// 8.3.1: Intra_4x4 prediction process for luma samples
void Intra_4x4_prediction_process_for_luma_samples(int luma4x4BlkIdx)
{
    // Result: Slice.MB[Slice.CurrMbAddr].Intra4x4PredMode[luma4x4BlkIdx] = Intra4x4PredMode_e;
    Derivation_process_for_Intra4x4PredMode(luma4x4BlkIdx);

    // Result: one 4x4 Prediction Values in MB.predL
    Intra_4x4_sample_prediction(luma4x4BlkIdx);
}