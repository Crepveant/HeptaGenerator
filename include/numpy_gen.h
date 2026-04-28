#ifndef NUMPY_GEN_H
#define NUMPY_GEN_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Write a NumPy .npy file for a contiguous uint8 3D array. */
void write_npy_3d_u8(char const *filepath, int N, int H, int W, uint8_t const *data);

/** Write a NumPy .npy file for a contiguous uint8 4D array. */
void write_npy_4d_u8(char const *filepath, int N, int C, int H, int W, uint8_t const *data);

/** Write a NumPy .npy file for a contiguous int16 2D array. */
void write_npy_2d(char const *filepath, int N, int D, int16_t const *data);

/** Write a NumPy .npy file for a contiguous int8 1D array. */
void write_npy_1d(char const *filepath, int N, int8_t const *data);

#ifdef __cplusplus
}
#endif

#endif
