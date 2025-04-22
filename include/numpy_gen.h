#ifndef NUMPY_GEN_H
#define NUMPY_GEN_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// write .npy file for uint8 4D array (e.g. states)
void write_npy_4d(const char* filepath, int N, int C, int H, int W, const uint8_t* data);

// write .npy file for int16 2D array (e.g. moves)
void write_npy_2d(const char* filepath, int N, int D, const int16_t* data);

// write .npy file for int8 1D array (e.g. players/winner)
void write_npy_1d(const char* filepath, int N, const int8_t* data);

#ifdef __cplusplus
}
#endif

#endif
