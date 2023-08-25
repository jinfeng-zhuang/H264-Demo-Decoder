#ifndef Mathematical_H
#define Mathematical_H

extern int Clip3(int x, int y, int z);
extern int Clip1Y8(int x);

/*
----------------------------------------
5.7 Mathematical functions
----------------------------------------
*/

#define Abs(x) ((x >= 0) ? x : -x)
#define Ceil(x)
#define InverseRasterScan(a,b,c,d,e) ((e == 0) ? ((a%(d/b))*b) : ((a/(d/b))*c))
#define Min(a ,b) ((a < b) ? a : b)
#define Max(a ,b) ((a < b) ? b : a)
#define Median(x, y, z) (x + y + z - Min(x, Min(y, z)) - Max(x, Max(y, z)))

#endif