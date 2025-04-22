#include "numpy_gen.h"

static void write_header(FILE* f, const char* dtype, const char* shape_str) {
    // Build header dictionary
    char dict[512];
    snprintf(dict, sizeof(dict),
        "{'descr': '%s', 'fortran_order': False, 'shape': (%s), }",
        dtype, shape_str);

    // Pad to 16-byte alignment
    size_t dict_len = strlen(dict);
    size_t pad = 16 - ((10 + dict_len) % 16);
    dict[dict_len + pad - 1] = '\n';
    dict[dict_len + pad] = '\0';
    for (size_t i = dict_len; i < dict_len + pad - 1; ++i) dict[i] = ' ';

    // Write preamble
    fputc((char)0x93, f);              // magic string
    fwrite("NUMPY", 1, 5, f);         // "NUMPY"
    fputc(0x01, f); fputc(0x00, f);    // version 1.0
    uint16_t header_len = (uint16_t)(strlen(dict));
    fwrite(&header_len, sizeof(uint16_t), 1, f);
    fwrite(dict, 1, strlen(dict), f);
}

void write_npy_4d(const char* filepath, int N, int C, int H, int W, const uint8_t* data) {
    FILE* f = fopen(filepath, "wb");
    if (!f) return;
    char shape[64]; snprintf(shape, sizeof(shape), "%d, %d, %d, %d", N, C, H, W);
    write_header(f, "|u1", shape);
    fwrite(data, sizeof(uint8_t), N * C * H * W, f);
    fclose(f);
}

void write_npy_2d(const char* filepath, int N, int D, const int16_t* data) {
    FILE* f = fopen(filepath, "wb");
    if (!f) return;
    char shape[64]; snprintf(shape, sizeof(shape), "%d, %d", N, D);
    write_header(f, "<i2", shape);
    fwrite(data, sizeof(int16_t), N * D, f);
    fclose(f);
}

void write_npy_1d(const char* filepath, int N, const int8_t* data) {
    FILE* f = fopen(filepath, "wb");
    if (!f) return;
    char shape[64]; snprintf(shape, sizeof(shape), "%d,", N);
    write_header(f, "<i1", shape);
    fwrite(data, sizeof(int8_t), N, f);
    fclose(f);
}