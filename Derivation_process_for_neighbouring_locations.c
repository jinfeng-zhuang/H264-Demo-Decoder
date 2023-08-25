#include "h264.h"

/*
-----------------------------------------------------------------------------------------------
ITU-T H.264: 6.4.12
Output: Neighbour MB and assosiated Block
-----------------------------------------------------------------------------------------------

Table 6-3 – Specification of mbAddrN
-----------------------------------------------------------------------------------------------
xN              yN              mbAddrN
< 0             < 0             mbAddrD
< 0             0..maxH − 1     mbAddrA
0..maxW − 1     < 0             mbAddrB
0..maxW − 1     0..maxH − 1     CurrMbAddr
> maxW − 1      < 0             mbAddrC
> maxW − 1      0..maxH − 1     not available
                > maxH − 1      not available
*/
void Derivation_process_for_neighbouring_locations(
    const int isLumaBlock,
    const int xN, const int yN,
    int* mbAddrN,
    int* xW, int* yW
)
{
    int maxW;
    int maxH;

    if (isLumaBlock) {
        maxW = 16;
        maxH = 16;
    }
    else {
        maxW = SPS.MbWidthC;
        maxH = SPS.MbHeightC;
    }

    if (yN > -1)
    {
        if (xN < 0)
            *mbAddrN = Slice.MB[Slice.CurrMbAddr].mbAddrA;
        else if (xN < maxW)
            *mbAddrN = Slice.CurrMbAddr;
        else
            return;
    }
    else
    {
        if (xN < 0)
            *mbAddrN = Slice.MB[Slice.CurrMbAddr].mbAddrD;
        else if (xN < maxW)
            *mbAddrN = Slice.MB[Slice.CurrMbAddr].mbAddrB;
        else
            *mbAddrN = Slice.MB[Slice.CurrMbAddr].mbAddrC;
    }

    *xW = (xN + maxW) % maxW; // Formula (6-34)
    *yW = (yN + maxH) % maxH; // Formula (6-35)
}