/*
 * From 'ITU-T H.264' recommendation:
 * 8.5.12.2 Transformation process for residual 4x4 blocks.
 */
void idct4x4(const int d[4][4], int r[4][4])
{
    int e[4][4];// = {{0}};
    int f[4][4];// = {{0}};
    int g[4][4];// = {{0}};
    int h[4][4];// = {{0}};

    int i = 0, j = 0;

    for (i = 0; i < 4; i++)
    {
        e[i][0] = d[i][0] + d[i][2];
        e[i][1] = d[i][0] - d[i][2];
        e[i][2] = (d[i][1] >> 1) - d[i][3];
        e[i][3] = d[i][1] + (d[i][3] >> 1);
    }

    for (i = 0; i < 4; i++)
    {
        f[i][0] = e[i][0] + e[i][3];
        f[i][1] = e[i][1] + e[i][2];
        f[i][2] = e[i][1] - e[i][2];
        f[i][3] = e[i][0] - e[i][3];
    }

    for (j = 0; j < 4; j++)
    {
        g[0][j] = f[0][j] + f[2][j];
        g[1][j] = f[0][j] - f[2][j];
        g[2][j] = (f[1][j] >> 1) - f[3][j];
        g[3][j] = f[1][j] + (f[3][j] >> 1);
    }

    for (j = 0; j < 4; j++)
    {
        h[0][j] = g[0][j] + g[3][j];
        h[1][j] = g[1][j] + g[2][j];
        h[2][j] = g[1][j] - g[2][j];
        h[3][j] = g[0][j] - g[3][j];
    }

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            r[i][j] = (h[i][j] + 32) >> 6;
}
