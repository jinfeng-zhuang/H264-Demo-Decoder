#include "h264.h"

// 9.3.1.2 Initialisation process for the arithmetic decoding engine.
void Initialisation_process_for_the_arithmetic_decoding_engine(void)
{
    Slice.CabacCtx.codIRange = 510;
    Slice.CabacCtx.codIOffset = (int)bitstream_read(9);

    if (Slice.CabacCtx.codIOffset == 510 || Slice.CabacCtx.codIOffset == 511)
        assert(0);
}