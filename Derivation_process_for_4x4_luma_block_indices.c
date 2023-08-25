/*
6.4.13.1 Derivation_process_for_4x4_luma_block_indices
*/

int Derivation_process_for_4x4_luma_block_indices(const int xP, const int yP)
{
    return (int)(8 * (yP / 8) + 4 * (xP / 8) + 2 * ((yP % 8) / 4) + ((xP % 8) / 4));
}