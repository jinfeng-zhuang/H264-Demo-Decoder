#include "h264.h"

/*
 * 6.4.9 Derivation process for neighbouring macroblock addresses and their availability.

----------------------------------------
|  mbAddr D  |  mbAddr B  |  mbAddr C  |
----------------------------------------
|  mbAddr A  | CurrMbAddr |            |
----------------------------------------
*/
void Derivation_process_for_neighbouring_macroblock_addresses_and_their_availability(void)
{
    struct MacroBlock_t* MB = &Slice.MB[Slice.CurrMbAddr];

    if (Slice.CurrMbAddr % SPS.PicWidthInMbs) {
        MB->mbAddrA = Slice.CurrMbAddr - 1;
    }
    else {
        MB->mbAddrA = -1;
    }

    if (Slice.CurrMbAddr >= SPS.PicWidthInMbs) {
        MB->mbAddrB = Slice.CurrMbAddr - SPS.PicWidthInMbs;
    }
    else {
        MB->mbAddrB = -1;
    }

    if ((Slice.CurrMbAddr >= SPS.PicWidthInMbs) && ((Slice.CurrMbAddr + 1) % SPS.PicWidthInMbs)) {
        MB->mbAddrC = Slice.CurrMbAddr - SPS.PicWidthInMbs + 1;
    }
    else {
        MB->mbAddrC = -1;
    }

    if ((Slice.CurrMbAddr > SPS.PicWidthInMbs) && (Slice.CurrMbAddr % SPS.PicWidthInMbs != 0)) {
        MB->mbAddrD = Slice.CurrMbAddr - SPS.PicWidthInMbs - 1;
    }
    else {
        MB->mbAddrD = -1;
    }
}