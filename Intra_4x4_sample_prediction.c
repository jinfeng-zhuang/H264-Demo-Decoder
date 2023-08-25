#include "h264.h"
#include <stdio.h>

static void Intra_4x4_Pred_Vertical(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred);
static void Intra_4x4_Pred_Horizontal(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred);
static void Intra_4x4_DC_prediction_mode(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred);
static void Intra_4x4_Pred_Down_Left(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred);
static void Intra_4x4_Pred_Down_Right(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred);
static void Intra_4x4_Pred_Vertical_Right(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred);
static void Intra_4x4_Pred_Horizontal_Down(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred);
static void Intra_4x4_Pred_Vertical_Left(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred);
static void Intra_4x4_Pred_Horizontal_Up(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred);

/*
---------------------------------------------
    h0/v0    h1  h2  h3  h4  h4  h4  h4  h4
    v1       O   O   O   O
    v2       O   O   O   O
    v3       O   O   O   O
    v4       O   O   O   O
---------------------------------------------
 */
struct intra_pred4x4 {
    int blkIdx;
    int BitDepthY;

    unsigned char ph[8 + 1];
    unsigned char pv[4 + 1];

    bool sample_left;
    bool sample_up_left;
    bool sample_up;
    bool sample_up_right;
};

/*
 * Overview
 *      Copy pixel data from neighbour, then generate current block's prediction.
 * 
 * -----------------------------------------------------------
 * TOC
 * -----------------------------------------------------------
 * 8:       Decoding process
 * 8.3:     Intra prediction process
 * 8.3.1:   Intra_4x4 prediction process for luma samples
 * 8.3.1.2: Intra_4x4 sample prediction
 * 8.3.1.2.1: Intra_4x4_Vertical prediction mode
 * 8.3.1.2.2: Intra_4x4_Horizontal prediction mode
 * 8.3.1.2.3: Intra_4x4_DC prediction mode
 * ...
 * 8.3.1.2.9: Intra_4x4_Horizontal_Up prediction mode
 *
 * Process
 * -----------------------------------------------------------
 * 1. Up-Left data
 * 2. Vertical data
 * 3. Horizontal data
 * 4. Intra_4x4_DC/Intra_4x4_Horizontal/... based on uppper => 4x4 block
 * 5. Copy to MB data
 */

int Intra_4x4_sample_prediction(const int luma4x4BlkIdx)
{
    int xO = -1, yO = -1;
    int mbAddrN;
    int xW, yW;
    int x, y;
    struct intra_pred4x4 pred;

    pred.blkIdx = luma4x4BlkIdx;
    pred.BitDepthY = SPS.BitDepthY;
    pred.sample_left = false;
    pred.sample_up_left = false;
    pred.sample_up = false;
    pred.sample_up_right = false;
    unsigned char pred4x4L[4][4] = { {0} };

    // Map Index to X,Y
    Inverse_4x4_luma_block_scanning_process(luma4x4BlkIdx, &xO, &yO);
    
    //=========================================================
    // Copy Up Left corner samples
    //=========================================================
    x = -1, y = -1;
    {
        // 1
        int xN = xO + x;
        int yN = yO + y;

        // 2
        xW = yW = mbAddrN = -1;
        Derivation_process_for_neighbouring_locations(1, xN, yN, &mbAddrN, &xW, &yW);

        // 3
        if (!(mbAddrN == -1 || (Slice.MB[mbAddrN].MbPartPredMode[0] > Intra_16x16 && PPS.constrained_intra_pred_flag == true)))
        {
            pred.sample_up_left = true;
            pred.pv[0] = pred.ph[0] = Slice.MB[mbAddrN].SprimeL[xW][yW];
        }
    }

    //=========================================================
    // Copy Vertical samples
    //=========================================================
    x = -1, y = 0;
    for (y = 0; y < 4; y++) {
        // 1
        int xN = xO + x;
        int yN = yO + y;

        // 2
        xW = yW = mbAddrN = -1;
        Derivation_process_for_neighbouring_locations(1, xN, yN, &mbAddrN, &xW, &yW);

        // 3
        if (!(mbAddrN == -1 || (Slice.MB[mbAddrN].MbPartPredMode[0] > Intra_16x16 && PPS.constrained_intra_pred_flag == true))) {
            pred.sample_left = true;
            pred.pv[y + 1] = Slice.MB[mbAddrN].SprimeL[xW][yW];
        }
    }

    //=========================================================
    // Copy Horizontal samples
    //=========================================================
    x = 0, y = -1;
    for (x = 0; x < 8; x++) {
        // 1
        int xN = xO + x;
        int yN = yO + y;

        // 2
        xW = yW = mbAddrN = -1;
        Derivation_process_for_neighbouring_locations(1, xN, yN, &mbAddrN, &xW, &yW);

        // 3
        if (!(mbAddrN == -1 || (Slice.MB[mbAddrN].MbPartPredMode[0] > Intra_16x16 && PPS.constrained_intra_pred_flag == true) || (x > 3 && (luma4x4BlkIdx == 3 || luma4x4BlkIdx == 11))))
        {
            if (x < 4)
                pred.sample_up = true;
            else
                pred.sample_up_right = true;

            pred.ph[x + 1] = Slice.MB[mbAddrN].SprimeL[xW][yW];
        }
    }

    //=========================================================
    // Copy Horizontal Extend (mbAddrC) samples
    //=========================================================
    if (pred.sample_up && !pred.sample_up_right) {
        for (x = 4; x < 8; x++)
        {
            pred.ph[x + 1] = pred.ph[3 + 1];
        }

        pred.sample_up_right = true;
    }

    //=========================================================
    // Data & Mode is ready: Calculate Prediction Data
    //=========================================================

    switch (Slice.MB[Slice.CurrMbAddr].Intra4x4PredMode[luma4x4BlkIdx]) {
    case Intra_4x4_PRED_MODE_Vertical:
        Intra_4x4_Pred_Vertical(pred4x4L, &pred);
        break;
    case Intra_4x4_PRED_MODE_Horizontal:
        Intra_4x4_Pred_Horizontal(pred4x4L, &pred);
        break;
    case Intra_4x4_PRED_MODE_DC:
        Intra_4x4_DC_prediction_mode(pred4x4L, &pred);
        break;
    case Intra_4x4_PRED_MODE_Diagonal_Down_Left:
        Intra_4x4_Pred_Down_Left(pred4x4L, &pred);
        break;
    case Intra_4x4_PRED_MODE_Diagonal_Down_Right:
        Intra_4x4_Pred_Down_Right(pred4x4L, &pred);
        break;
    case Intra_4x4_PRED_MODE_Vertical_Right:
        Intra_4x4_Pred_Vertical_Right(pred4x4L, &pred);
        break;
    case Intra_4x4_PRED_MODE_Horizontal_Down:
        Intra_4x4_Pred_Horizontal_Down(pred4x4L, &pred);
        break;
    case Intra_4x4_PRED_MODE_Vertical_Left:
        Intra_4x4_Pred_Vertical_Left(pred4x4L, &pred);
        break;
    case Intra_4x4_PRED_MODE_Horizontal_Up:
        Intra_4x4_Pred_Horizontal_Up(pred4x4L, &pred);
        break;
    default:
        printf("Prediction mode %d should be supported\n", Slice.MB[Slice.CurrMbAddr].Intra4x4PredMode[luma4x4BlkIdx]);
        assert(0);
        break;
    }

    //=========================================================
    // Copy Result to MB
    //=========================================================

    for (x = 0; x < 4; x++) {
        for (y = 0; y < 4; y++) {
            Slice.MB[Slice.CurrMbAddr].predL[xO + x][yO + y] = pred4x4L[x][y];
        }
    }

#ifdef DEBUG
    printf("\nPredition:\n");
    for (x = 0; x < 4; x++) {
        for (y = 0; y < 4; y++) {
            printf("%8d ", pred4x4L[x][y]);
        }
        printf("\n");
    }
#endif

    return 0;
}

//======================================================================
// Helper
//======================================================================

// 8.3.1.2.2 Specification of Intra_4x4_Horizontal prediction mode.
static void Intra_4x4_Pred_Vertical(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred)
{
    int i, j;

    if (pred->sample_up) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                pred4x4L[i][j] = pred->ph[i + 1];
            }
        }
    }
}

// 8.3.1.2.2 Specification of Intra_4x4_Horizontal prediction mode.
static void Intra_4x4_Pred_Horizontal(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred)
{
    int i, j;

    if (pred->sample_left) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                pred4x4L[i][j] = pred->pv[j + 1];
            }
        }
    }
}

// 8.3.1.2.3 Specification of Intra_4x4_DC prediction mode.
static void Intra_4x4_DC_prediction_mode(unsigned char pred4x4L[4][4], struct intra_pred4x4* pred)
{
    int x = 0, y = 0;
    int sumH = pred->ph[0 + 1] + pred->ph[1 + 1] + pred->ph[2 + 1] + pred->ph[3 + 1];
    int sumV = pred->pv[0 + 1] + pred->pv[1 + 1] + pred->pv[2 + 1] + pred->pv[3 + 1];

    if (pred->sample_left && pred->sample_up) {
        for (x = 0; x < 4; x++)
            for (y = 0; y < 4; y++)
                pred4x4L[x][y] = (sumH + sumV + 4) >> 3;
    }
    else if (!pred->sample_up && pred->sample_left) {
        for (x = 0; x < 4; x++)
            for (y = 0; y < 4; y++)
                pred4x4L[x][y] = (sumV + 2) >> 2;
    }
    else if (pred->sample_up && !pred->sample_left) {
        for (x = 0; x < 4; x++)
            for (y = 0; y < 4; y++)
                pred4x4L[x][y] = (sumH + 2) >> 2;
    }
    else if (!pred->sample_left && !pred->sample_up) {
        for (x = 0; x < 4; x++)
            for (y = 0; y < 4; y++)
                pred4x4L[x][y] = (1 << (pred->BitDepthY - 1)); // 1 << 7 = 128
    }
}

static void Intra_4x4_Pred_Down_Left(unsigned char pred4x4L[4][4], struct intra_pred4x4* ip)
{
    int x, y;

    if (ip->sample_up && ip->sample_up_right) {
        for (x = 0; x < 4; x++) {
            for (y = 0; y < 4; y++) {
                if (x == 3 && y == 3)
                    pred4x4L[x][y] = (ip->ph[6 + 1] + 3 * ip->ph[7 + 1] + 2) >> 2;
                else
                    pred4x4L[x][y] = (ip->ph[(x + y) + 1] + 2 * ip->ph[(x + y + 1) + 1] + ip->ph[(x + y + 2) + 1] + 2) >> 2;
            }
        }
    }
}

static void Intra_4x4_Pred_Down_Right(unsigned char pred4x4L[4][4], struct intra_pred4x4* ip)
{
    int x, y;

    if (ip->sample_up && ip->sample_up_left && ip->sample_left) {
        for (x = 0; x < 4; x++) {
            for (y = 0; y < 4; y++) {
                if (x > y)
                    pred4x4L[x][y] = (ip->ph[(x - y - 2) + 1] + 2 * ip->ph[(x - y - 1) + 1] + ip->ph[(x - y) + 1] + 2) >> 2;
                else if (x < y)
                    pred4x4L[x][y] = (ip->pv[(y - x - 2) + 1] + 2 * ip->pv[(y - x - 1) + 1] + ip->pv[(y - x) + 1] + 2) >> 2;
                else
                    pred4x4L[x][y] = (ip->ph[0 + 1] + 2 * ip->pv[0] + ip->pv[0 + 1] + 2) >> 2;
            }
        }
    }
}

static void Intra_4x4_Pred_Vertical_Right(unsigned char pred4x4L[4][4], struct intra_pred4x4* ip)
{
    int x, y;
    int zVR = 0;

    if (ip->sample_up && ip->sample_up_left && ip->sample_left) {
        for (x = 0; x < 4; x++) {
            for (y = 0; y < 4; y++) {
                zVR = 2 * x - y;
                if (zVR > -1) {
                    if (zVR % 2 == 0)
                        pred4x4L[x][y] = (ip->ph[(x - (y >> 1) - 1) + 1] + ip->ph[(x - (y >> 1)) + 1] + 1) >> 1;
                    else
                        pred4x4L[x][y] = (ip->ph[(x - (y >> 1) - 2) + 1] + 2 * ip->ph[(x - (y >> 1) - 1) + 1] + ip->ph[(x - (y >> 1)) + 1] + 2) >> 2;
                }
                else if (zVR == -1) {
                    pred4x4L[x][y] = (ip->pv[0 + 1] + 2 * ip->pv[0] + ip->ph[0 + 1] + 2) >> 2;
                }
                else {
                    pred4x4L[x][y] = (ip->pv[(y - 1) + 1] + 2 * ip->pv[(y - 2) + 1] + ip->pv[(y - 3) + 1] + 2) >> 2;
                }
            }
        }
    }
}

static void Intra_4x4_Pred_Horizontal_Down(unsigned char pred4x4L[4][4], struct intra_pred4x4* ip)
{
    int x, y;
    int zHD = 0;

    if (ip->sample_up && ip->sample_up_left && ip->sample_left) {
        for (x = 0; x < 4; x++) {
            for (y = 0; y < 4; y++) {
                zHD = 2 * y - x;
                if (zHD > -1) {
                    if (zHD % 2 == 0)
                        pred4x4L[x][y] = (ip->pv[(y - (x >> 1) - 1) + 1] + ip->pv[(y - (x >> 1)) + 1] + 1) >> 1;
                    else
                        pred4x4L[x][y] = (ip->pv[(y - (x >> 1) - 2) + 1] + 2 * ip->pv[(y - (x >> 1) - 1) + 1] + ip->pv[(y - (x >> 1)) + 1] + 2) >> 2;
                }
                else if (zHD == -1)
                    pred4x4L[x][y] = (ip->pv[0 + 1] + 2 * ip->pv[0] + ip->ph[0 + 1] + 2) >> 2;
                else
                    pred4x4L[x][y] = (ip->ph[(x - 1) + 1] + 2 * ip->ph[(x - 2) + 1] + ip->ph[(x - 3) + 1] + 2) >> 2;
            }
        }
    }
}

static void Intra_4x4_Pred_Vertical_Left(unsigned char pred4x4L[4][4], struct intra_pred4x4* ip)
{
    int x, y;

    if (ip->sample_up && ip->sample_up_right) {
        for (x = 0; x < 4; x++) {
            for (y = 0; y < 4; y++) {
                if (y % 2 == 0)
                    pred4x4L[x][y] = (ip->ph[(x + (y >> 1)) + 1] + ip->ph[(x + (y >> 1) + 1) + 1] + 1) >> 1;
                else
                    pred4x4L[x][y] = (ip->ph[(x + (y >> 1)) + 1] + 2 * ip->ph[(x + (y >> 1) + 1) + 1] + ip->ph[(x + (y >> 1) + 2) + 1] + 2) >> 2;
            }
        }
    }
}

static void Intra_4x4_Pred_Horizontal_Up(unsigned char pred4x4L[4][4], struct intra_pred4x4* ip)
{
    int x, y;
    int zHU = 0;

    if (ip->sample_left) {
        for (x = 0; x < 4; x++) {
            for (y = 0; y < 4; y++) {
                zHU = x + 2 * y;
                if (zHU < 5 && zHU % 2 == 0)
                    pred4x4L[x][y] = (ip->pv[(y + (x >> 1)) + 1] + ip->pv[(y + (x >> 1) + 1) + 1] + 1) >> 1;
                else if (zHU == 1 || zHU == 3)
                    pred4x4L[x][y] = (ip->pv[(y + (x >> 1)) + 1] + 2 * ip->pv[(y + (x >> 1) + 1) + 1] + ip->pv[(y + (x >> 1) + 2) + 1] + 2) >> 2;
                else if (zHU == 5)
                    pred4x4L[x][y] = (ip->pv[2 + 1] + 3 * ip->pv[3 + 1] + 2) >> 2;
                else
                    pred4x4L[x][y] = ip->pv[3 + 1];
            }
        }
    }
}