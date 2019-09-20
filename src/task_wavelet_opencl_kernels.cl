static const char* g_focusstack_wavelet_kernel_src = R"---(
// This file contains OpenCL code for complex wavelet transform.
// It's wrapped in a C++ raw string literal to simplify including it in binary.

// Wrap (x + dx) to range 0 to n-1
#define WRAP(x, dx, n) ((dx < 0) ? (((x) + dx >= 0) ? ((x) + dx) : ((x) + dx + n)) \
                                 : (((x) + dx <  n) ? ((x) + dx) : ((x) + dx - n)))
#define LOAD(x, y) *((__global const float2*)(src + mad24((x), (int)sizeof(float2), mad24(y, src_step, src_offset))))
#define STORE(x, y) *((__global float2*)(dst + mad24((x), (int)sizeof(float2), mad24(y, dst_step, dst_offset))))
#define CONJ(x) (float2)((x).s0, -(x).s1)

__kernel void decompose_horizontal(__global const uchar *src, int src_step, int src_offset,
                                   __global uchar *dst, int dst_step, int dst_offset, int rows, int cols,
                                   float16 lopass, float16 hipass)
{
  const int x = get_global_id(0) * 2;
  const int y = get_global_id(1);

  if (x < cols && y < rows)
  {
    float2 lp, hp, v;

    v = LOAD(WRAP(x, -3, cols), y);
    lp = v.s01 * lopass.s00 - v.s10 * CONJ(lopass.s11);
    hp = v.s01 * hipass.s00 - v.s10 * CONJ(hipass.s11);

    v = LOAD(WRAP(x, -2, cols), y);
    lp += v.s01 * lopass.s22 - v.s10 * CONJ(lopass.s33);
    hp += v.s01 * hipass.s22 - v.s10 * CONJ(hipass.s33);

    v = LOAD(WRAP(x, -1, cols), y);
    lp += v.s01 * lopass.s44 - v.s10 * CONJ(lopass.s55);
    hp += v.s01 * hipass.s44 - v.s10 * CONJ(hipass.s55);

    v = LOAD(x, y);
    lp += v.s01 * lopass.s66 - v.s10 * CONJ(lopass.s77);
    hp += v.s01 * hipass.s66 - v.s10 * CONJ(hipass.s77);

    v = LOAD(WRAP(x,  1, cols), y);
    lp += v.s01 * lopass.s88 - v.s10 * CONJ(lopass.s99);
    hp += v.s01 * hipass.s88 - v.s10 * CONJ(hipass.s99);

    v = LOAD(WRAP(x,  2, cols), y);
    lp += v.s01 * lopass.sAA - v.s10 * CONJ(lopass.sBB);
    hp += v.s01 * hipass.sAA - v.s10 * CONJ(hipass.sBB);

    STORE(x / 2, y) = lp;
    STORE(x / 2 + cols / 2, y) = hp;
  }
}

__kernel void decompose_vertical(__global const uchar *src, int src_step, int src_offset,
                                 __global uchar *dst, int dst_step, int dst_offset, int rows, int cols,
                                 float16 lopass, float16 hipass)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1) * 2;

  if (x < cols && y < rows)
  {
    float2 lp, hp, v;

    v = LOAD(x, WRAP(y, -3, rows));
    lp = v.s01 * lopass.s00 - v.s10 * CONJ(lopass.s11);
    hp = v.s01 * hipass.s00 - v.s10 * CONJ(hipass.s11);

    v = LOAD(x, WRAP(y, -2, rows));
    lp += v.s01 * lopass.s22 - v.s10 * CONJ(lopass.s33);
    hp += v.s01 * hipass.s22 - v.s10 * CONJ(hipass.s33);

    v = LOAD(x, WRAP(y, -1, rows));
    lp += v.s01 * lopass.s44 - v.s10 * CONJ(lopass.s55);
    hp += v.s01 * hipass.s44 - v.s10 * CONJ(hipass.s55);

    v = LOAD(x, y);
    lp += v.s01 * lopass.s66 - v.s10 * CONJ(lopass.s77);
    hp += v.s01 * hipass.s66 - v.s10 * CONJ(hipass.s77);

    v = LOAD(x, WRAP(y,  1, rows));
    lp += v.s01 * lopass.s88 - v.s10 * CONJ(lopass.s99);
    hp += v.s01 * hipass.s88 - v.s10 * CONJ(hipass.s99);

    v = LOAD(x, WRAP(y,  2, rows));
    lp += v.s01 * lopass.sAA - v.s10 * CONJ(lopass.sBB);
    hp += v.s01 * hipass.sAA - v.s10 * CONJ(hipass.sBB);

    STORE(x, y / 2) = lp;
    STORE(x, y / 2 + rows / 2) = hp;
  }
}

__kernel void compose_horizontal(__global const uchar *src, int src_step, int src_offset,
                                 __global uchar *dst, int dst_step, int dst_offset, int rows, int cols,
                                 float16 lopass, float16 hipass)
{
  const int x = get_global_id(0) * 2;
  const int y = get_global_id(1);
  const int halflen = cols / 2;

  if (x < cols && y < rows)
  {
    float2 p0, p1, lp, hp;

    lp = LOAD(WRAP(x / 2, 1, halflen), y);
    hp = LOAD(WRAP(x / 2, 1, halflen) + halflen, y);
    p0  = lp.s01 * lopass.s00 + lp.s10 * CONJ(lopass.s11);
    p0 += hp.s01 * hipass.s00 + hp.s10 * CONJ(hipass.s11);
    p1  = lp.s01 * lopass.s22 + lp.s10 * CONJ(lopass.s33);
    p1 += hp.s01 * hipass.s22 + hp.s10 * CONJ(hipass.s33);

    lp = LOAD(x / 2, y);
    hp = LOAD(x / 2 + halflen, y);
    p0 += lp.s01 * lopass.s44 + lp.s10 * CONJ(lopass.s55);
    p0 += hp.s01 * hipass.s44 + hp.s10 * CONJ(hipass.s55);
    p1 += lp.s01 * lopass.s66 + lp.s10 * CONJ(lopass.s77);
    p1 += hp.s01 * hipass.s66 + hp.s10 * CONJ(hipass.s77);

    lp = LOAD(WRAP(x / 2, -1, halflen), y);
    hp = LOAD(WRAP(x / 2, -1, halflen) + halflen, y);
    p0 += lp.s01 * lopass.s88 + lp.s10 * CONJ(lopass.s99);
    p0 += hp.s01 * hipass.s88 + hp.s10 * CONJ(hipass.s99);
    p1 += lp.s01 * lopass.sAA + lp.s10 * CONJ(lopass.sBB);
    p1 += hp.s01 * hipass.sAA + hp.s10 * CONJ(hipass.sBB);

    STORE(WRAP(x, -1, cols), y) = p0;
    STORE(x, y) = p1;
  }
}

__kernel void compose_vertical(__global const uchar *src, int src_step, int src_offset,
                               __global uchar *dst, int dst_step, int dst_offset, int rows, int cols,
                               float16 lopass, float16 hipass)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1) * 2;
  const int halflen = rows / 2;

  if (x < cols && y < rows)
  {
    float2 p0, p1, lp, hp;

    lp = LOAD(x, WRAP(y / 2, 1, halflen));
    hp = LOAD(x, WRAP(y / 2, 1, halflen) + halflen);
    p0  = lp.s01 * lopass.s00 + lp.s10 * CONJ(lopass.s11);
    p0 += hp.s01 * hipass.s00 + hp.s10 * CONJ(hipass.s11);
    p1  = lp.s01 * lopass.s22 + lp.s10 * CONJ(lopass.s33);
    p1 += hp.s01 * hipass.s22 + hp.s10 * CONJ(hipass.s33);

    lp = LOAD(x, y / 2);
    hp = LOAD(x, y / 2 + halflen);
    p0 += lp.s01 * lopass.s44 + lp.s10 * CONJ(lopass.s55);
    p0 += hp.s01 * hipass.s44 + hp.s10 * CONJ(hipass.s55);
    p1 += lp.s01 * lopass.s66 + lp.s10 * CONJ(lopass.s77);
    p1 += hp.s01 * hipass.s66 + hp.s10 * CONJ(hipass.s77);

    lp = LOAD(x, WRAP(y / 2, -1, halflen));
    hp = LOAD(x, WRAP(y / 2, -1, halflen) + halflen);
    p0 += lp.s01 * lopass.s88 + lp.s10 * CONJ(lopass.s99);
    p0 += hp.s01 * hipass.s88 + hp.s10 * CONJ(hipass.s99);
    p1 += lp.s01 * lopass.sAA + lp.s10 * CONJ(lopass.sBB);
    p1 += hp.s01 * hipass.sAA + hp.s10 * CONJ(hipass.sBB);

    STORE(x, WRAP(y, -1, rows)) = p0;
    STORE(x, y) = p1;
  }
}

)---";






