
static const int zigzag_4x4_2d[16][2] =
{
    {0,0}, {0,1}, {1,0}, {2,0},
    {1,1}, {0,2}, {0,3}, {1,2},
    {2,1}, {3,0}, {3,1}, {2,2},
    {1,3}, {2,3}, {3,2}, {3,3}
};

// 8.5.6 Inverse scanning process for 4x4 transform coefficients and scaling lists.
// Input : coeff
// Output: coeff matrix
void Inverse_scanning_process_for_4x4_transform(int coeff[16], int matrix[4][4])
{
    int zz;

    for (zz = 0; zz < 16; zz++)
    {
        matrix[zigzag_4x4_2d[zz][0]][zigzag_4x4_2d[zz][1]] = coeff[zz];
    }
}