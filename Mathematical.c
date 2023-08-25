
// z should be in range [x, y], otherwise, it be x or y
int Clip3(int x, int y, int z)
{
    if (z < x)
        return x;
    else if (z > y)
        return y;
    else
        return z;
}

int Clip1Y8(int x)
{
    return Clip3(0, (1 << 8) - 1, x);
}