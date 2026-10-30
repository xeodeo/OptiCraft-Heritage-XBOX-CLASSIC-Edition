#include "stdafx.h"
#include "ImprovedNoise_SPU.h"

#include <spu_intrinsics.h>

// OptiCraft: the terrain this computes must be bit-for-bit the PPU's
// (NoiseGeneratorPerlin::getCanSpawnHere). The PPU build (gcc 13, default
// -ffp-contract=fast) fuses every "a * b + c" whose product has no other use
// into one fmadd: 44 of them in getCanSpawnHere, which are exactly the spots
// marked FMA below (10 in the 2D path, 17 per 3D path). Sony's SPU gcc 4.1
// never fuses on its own, so those spots use dfma explicitly and every other
// operation stays a plain IEEE dfa/dfs/dfm, in the PPU's order. Nothing else
// in 4J's math differs from ours. The console's verify mode compares the two.
static inline double fmaddd(double a, double b, double c)
{
    return spu_extract(spu_madd(spu_promote(a, 0), spu_promote(b, 0), spu_promote(c, 0)), 0);
}

// x * x * x * (x * (x * 6 - 15) + 10), with the PPU's two fused steps.
static inline double fade(double x)
{
    return x * x * x * fmaddd(x, fmaddd(x, 6.0, -15.0), 10.0);   // FMA x2
}


double ImprovedNoise_SPU::lerp(double t, double a, double b)
{
   return fmaddd(t, b - a, a);   // FMA: a + t * (b - a)
}

double ImprovedNoise_SPU::grad2(int hash, double x, double z)
{
    int h = hash & 15; // CONVERT LO 4 BITS OF HASH CODE

    double u = (1-((h&8)>>3))*x, // INTO 12 GRADIENT DIRECTIONS.
    v = h < 4 ? 0 : h == 12 || h == 14 ? x : z;

    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double ImprovedNoise_SPU::grad(int hash, double x, double y, double z)
{
    int h = hash & 15; // CONVERT LO 4 BITS OF HASH CODE

    double u = h < 8 ? x : y, // INTO 12 GRADIENT DIRECTIONS.
    v = h < 4 ? y : h == 12 || h == 14 ? x : z;

    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}


void ImprovedNoise_SPU::add(double* buffer, double _x, double _y, double _z, int xSize, int ySize, int zSize, double xs, double ys, double zs, double invPow)
{
    if (ySize==1)
	{
        int A = 0, AA = 0, B = 0, BA = 0;
        double vv0 = 0, vv2 = 0;
        int pp = 0;
        double scale = invPow;
        for (int xx = 0; xx < xSize; xx++)
		{
            double x = fmaddd((double)xx, xs, _x) + xo;   // FMA
            int xf = (int) x;
            if (x < xf) xf--;
            int X = xf & 255;
            x -= xf;
            double u = fade(x);

            for (int zz = 0; zz < zSize; zz++)
			{
                double z = fmaddd((double)zz, zs, _z) + zo;   // FMA
                int zf = (int) z;
                if (z < zf) zf--;
                int Z = zf & 255;
                z -= zf;
                double w = fade(z);

                A = p[X] + 0;
                AA = p[A] + Z;
                B = p[X + 1] + 0;
                BA = p[B] + Z;
                vv0 = lerp(u, grad2(p[AA], x, z), grad(p[BA], x - 1, 0, z));
                vv2 = lerp(u, grad(p[AA + 1], x, 0, z - 1), grad(p[BA + 1], x - 1, 0, z - 1));

                double val = lerp(w, vv0, vv2);

                buffer[pp] = fmaddd(val, scale, buffer[pp]);   // FMA: += val * scale
                pp++;
            }
        }
        return;
    }
    int pp = 0;
    double scale = invPow;
    int yOld = -1;
    int A = 0, AA = 0, AB = 0, B = 0, BA = 0, BB = 0;
    double vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++)
	{
        double x = fmaddd((double)xx, xs, _x) + xo;   // FMA
        int xf = (int) x;
        if (x < xf) xf--;
        int X = xf & 255;
        x -= xf;
        double u = fade(x);


        for (int zz = 0; zz < zSize; zz++)
		{
            double z = fmaddd((double)zz, zs, _z) + zo;   // FMA
            int zf = (int) z;
            if (z < zf) zf--;
            int Z = zf & 255;
            z -= zf;
            double w = fade(z);


            for (int yy = 0; yy < ySize; yy++)
			{
                double y = fmaddd((double)yy, ys, _y) + yo;   // FMA
                int yf = (int) y;
                if (y < yf) yf--;
                int Y = yf & 255;
                y -= yf;
                double v = fade(y);

                if (yy == 0 || Y != yOld)
				{
                    yOld = Y;
                    A = p[X] + Y;
                    AA = p[A] + Z;
                    AB = p[A + 1] + Z;
                    B = p[X + 1] + Y;
                    BA = p[B] + Z;
                    BB = p[B + 1] + Z;
                    vv0 = lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z));
                    vv1 = lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z));
                    vv2 = lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1));
                    vv3 = lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1));
                }


                double v0 = lerp(v, vv0, vv1);
                double v1 = lerp(v, vv2, vv3);
                double val = lerp(w, v0, v1);

                buffer[pp] = fmaddd(val, scale, buffer[pp]);   // FMA: += val * scale
                pp++;
            }
        }
	}
}
