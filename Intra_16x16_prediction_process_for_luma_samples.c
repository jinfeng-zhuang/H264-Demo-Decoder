#include "h264.h"
#include <stdio.h>

/*
---------------------------------------------
    h0/v0    h1  h2  h3  h4  h4  h4  h4  h4
    v1       O   O   O   O
    v2       O   O   O   O
    v3       O   O   O   O
    v4       O   O   O   O
---------------------------------------------
 */
struct intra_pred16x16 {
    int BitDepthY;

    unsigned char phv;
    unsigned char ph[16];
    unsigned char pv[16];

    bool sample_left;
    bool sample_up;
};

static void Intra_16x16_Pred_Vertical(unsigned char pred4x4L[16][16], struct intra_pred16x16* pred);
static void Intra_16x16_Pred_Horizontal(unsigned char pred4x4L[16][16], struct intra_pred16x16* pred);
static void Intra_16x16_Pred_DC(unsigned char pred4x4L[16][16], struct intra_pred16x16* pred);
static void Intra_16x16_Pred_Plane(unsigned char pred4x4L[16][16], struct intra_pred16x16* pred);

/*
 * Overview
 *      Copy pixel data from neighbour, then generate current block's prediction.
 *
 * -----------------------------------------------------------
 * TOC
 * -----------------------------------------------------------
 * 8:       Decoding process
 * 8.3:     Intra prediction process
 * 8.3.3:   Intra_16x16 prediction process for luma samples
 */

void Intra_16x16_prediction_process_for_luma_samples(void)
{
    int mbAddrN;
    int xW, yW;
    int x, y;
    struct intra_pred16x16 pred;

    memset(&pred, 0, sizeof(struct intra_pred16x16));
    pred.BitDepthY = SPS.BitDepthY;
    pred.sample_left = false;
    pred.sample_up = false;

    unsigned char pred16x16L[16][16] = { {0} };

    //=========================================================
    // Copy Up Left corner samples
    //=========================================================
    x = -1, y = -1;
    {
        // 1
        int xN = x;
        int yN = y;

        // 2
        xW = yW = mbAddrN = -1;
        Derivation_process_for_neighbouring_locations(1, xN, yN, &mbAddrN, &xW, &yW);

        // 3
        if (!(mbAddrN == -1 || (Slice.MB[mbAddrN].MbPartPredMode[0] > Intra_16x16 && PPS.constrained_intra_pred_flag == true)))
        {
            pred.phv = Slice.MB[mbAddrN].SprimeL[xW][yW];
        }
    }

    //=========================================================
    // Copy Vertical samples
    //=========================================================
    x = -1, y = 0;
    for (y = 0; y < 16; y++) {
        // 1
        int xN = x;
        int yN = y;

        // 2
        xW = yW = mbAddrN = -1;
        Derivation_process_for_neighbouring_locations(1, xN, yN, &mbAddrN, &xW, &yW);

        // 3
        if (!(mbAddrN == -1 || (Slice.MB[mbAddrN].MbPartPredMode[0] > Intra_16x16 && PPS.constrained_intra_pred_flag == true))) {
            pred.sample_left = true;
            pred.pv[y] = Slice.MB[mbAddrN].SprimeL[xW][yW];
        }
    }

    //=========================================================
    // Copy Horizontal samples
    //=========================================================
    x = 0, y = -1;
    for (x = 0; x < 16; x++) {
        // 1
        int xN = x;
        int yN = y;

        // 2
        xW = yW = mbAddrN = -1;
        Derivation_process_for_neighbouring_locations(1, xN, yN, &mbAddrN, &xW, &yW);

        // 3
        if (!(mbAddrN == -1 || (Slice.MB[mbAddrN].MbPartPredMode[0] > Intra_16x16 && PPS.constrained_intra_pred_flag == true)))
        {
            pred.sample_up = true;
            pred.ph[x] = Slice.MB[mbAddrN].SprimeL[xW][yW];
        }
    }

    //=========================================================
    // Data & Mode is ready: Calculate Prediction Data
    //=========================================================

    switch (Slice.MB[Slice.CurrMbAddr].Intra16x16PredMode) {
    case Intra_16x16_Vertical:
        Intra_16x16_Pred_Vertical(pred16x16L, &pred);
        break;
    case Intra_16x16_Horizontal:
        Intra_16x16_Pred_Horizontal(pred16x16L, &pred);
        break;
    case Intra_16x16_DC:
        Intra_16x16_Pred_DC(pred16x16L, &pred);
        break;
    case Intra_16x16_Plane:
        Intra_16x16_Pred_Plane(pred16x16L, &pred);
        break;
    default:
        printf("Prediction mode %d should be supported\n", Slice.MB[Slice.CurrMbAddr].Intra16x16PredMode);
        assert(0);
        break;
    }

    //=========================================================
    // Copy Result to MB
    //=========================================================

    for (x = 0; x < 16; x++) {
        for (y = 0; y < 16; y++) {
            Slice.MB[Slice.CurrMbAddr].predL[x][y] = pred16x16L[x][y];
        }
    }

#ifdef DEBUG
    printf("\nPredition:\n");
    for (x = 0; x < 16; x++) {
        for (y = 0; y < 16; y++) {
            printf("%8d ", pred16x16L[x][y]);
        }
        printf("\n");
    }
#endif
}

//======================================================================
// Helper
//======================================================================

// 8.3.1.2.2 Specification of Intra_4x4_Horizontal prediction mode.
static void Intra_16x16_Pred_Vertical(unsigned char pred16x16L[16][16], struct intra_pred16x16* pred)
{
    int i, j;

    if (pred->sample_up) {
        for (i = 0; i < 16; i++) {
            for (j = 0; j < 16; j++) {
                pred16x16L[i][j] = pred->ph[i];
            }
        }
    }
}

// 8.3.1.2.2 Specification of Intra_4x4_Horizontal prediction mode.
static void Intra_16x16_Pred_Horizontal(unsigned char pred16x16L[16][16], struct intra_pred16x16* pred)
{
    int i, j;

    if (pred->sample_left) {
        for (i = 0; i < 16; i++) {
            for (j = 0; j < 16; j++) {
                pred16x16L[i][j] = pred->pv[j];
            }
        }
    }
}

// 8.3.1.2.3 Specification of Intra_4x4_DC prediction mode.
static void Intra_16x16_Pred_DC(unsigned char pred16x16L[16][16], struct intra_pred16x16* pred)
{
    int x = 0, y = 0;
    int sumH = 0, sumV = 0;

    for (x = 0; x < 16; x++) {
        sumH += pred->ph[x];
        sumV += pred->pv[x];
    }

    if (pred->sample_left && pred->sample_up) {
        for (x = 0; x < 16; x++)
            for (y = 0; y < 16; y++)
                pred16x16L[x][y] = (sumH + sumV + 16) >> 5;
    }
    else if (!pred->sample_up && pred->sample_left) {
        for (x = 0; x < 16; x++)
            for (y = 0; y < 16; y++)
                pred16x16L[x][y] = (sumV + 8) >> 4;
    }
    else if (pred->sample_up && !pred->sample_left) {
        for (x = 0; x < 16; x++)
            for (y = 0; y < 16; y++)
                pred16x16L[x][y] = (sumH + 8) >> 4;
    }
    else if (!pred->sample_left && !pred->sample_up) {
        for (x = 0; x < 16; x++)
            for (y = 0; y < 16; y++)
                pred16x16L[x][y] = (1 << (pred->BitDepthY - 1)); // 1 << 7 = 128
    }
    else {
        // none
    }
}

static void Intra_16x16_Pred_Plane(unsigned char pred16x16L[16][16], struct intra_pred16x16* ip)
{
    int H = 0, V = 0;
    int a = 0, b = 0, c = 0;
    int i = 0;
    int x = 0, y = 0;

    if (ip->sample_left && ip->sample_up) {
        for (i = 0; i < 8; i++) {
            if (6 - i == -1) {
                H += (i + 1) * (ip->ph[8 + i] - ip->phv);
                V += (i + 1) * (ip->pv[8 + i] - ip->phv);
            }
            else {
                H += (i + 1) * (ip->ph[8 + i] - ip->ph[6 - i]);
                V += (i + 1) * (ip->pv[8 + i] - ip->pv[6 - i]);
            }
        }

        a = 16 * (ip->pv[15] + ip->ph[15]);
        b = (5 * H + 32) >> 6;
        c = (5 * V + 32) >> 6;

        for (x = 0; x < 16; x++)
            for (y = 0; y < 16; y++)
                pred16x16L[x][y] = Clip1Y8((a + b * (x - 7) + c * (y - 7) + 16) >> 5);
    }
}