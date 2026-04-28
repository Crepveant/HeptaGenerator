#include "numpy_gen.h"

static void write_header(FILE *f, char const *dtype, char const *shape_str) {
	char dict [512];
	snprintf(dict, sizeof(dict), "{'descr': '%s', 'fortran_order': False, 'shape': (%s), }", dtype, shape_str);

	size_t dict_len           = strlen(dict);
	size_t pad                = 16 - ((10 + dict_len) % 16);
	dict [dict_len + pad - 1] = '\n';
	dict [dict_len + pad]     = '\0';
	for (size_t i = dict_len; i < dict_len + pad - 1; ++i) dict [i] = ' ';

	fputc(( char ) 0x93, f);
	fwrite("NUMPY", 1, 5, f);
	fputc(0x01, f);
	fputc(0x00, f);
	uint16_t header_len = ( uint16_t ) (strlen(dict));
	fwrite(&header_len, sizeof(uint16_t), 1, f);
	fwrite(dict, 1, strlen(dict), f);
}

void write_npy_3d_u8(char const *filepath, int N, int H, int W, uint8_t const *data) {
	FILE *f = fopen(filepath, "wb");
	if (!f) return;
	char shape [64];
	snprintf(shape, sizeof(shape), "%d, %d, %d", N, H, W);
	write_header(f, "|u1", shape);
	fwrite(data, sizeof(uint8_t), N * H * W, f);
	fclose(f);
}

void write_npy_4d_u8(char const *filepath, int N, int C, int H, int W, uint8_t const *data) {
	FILE *f = fopen(filepath, "wb");
	if (!f) return;
	char shape [64];
	snprintf(shape, sizeof(shape), "%d, %d, %d, %d", N, C, H, W);
	write_header(f, "|u1", shape);
	fwrite(data, sizeof(uint8_t), N * C * H * W, f);
	fclose(f);
}

void write_npy_2d(char const *filepath, int N, int D, int16_t const *data) {
	FILE *f = fopen(filepath, "wb");
	if (!f) return;
	char shape [64];
	snprintf(shape, sizeof(shape), "%d, %d", N, D);
	write_header(f, "<i2", shape);
	fwrite(data, sizeof(int16_t), N * D, f);
	fclose(f);
}

void write_npy_1d(char const *filepath, int N, int8_t const *data) {
	FILE *f = fopen(filepath, "wb");
	if (!f) return;
	char shape [64];
	snprintf(shape, sizeof(shape), "%d,", N);
	write_header(f, "<i1", shape);
	fwrite(data, sizeof(int8_t), N, f);
	fclose(f);
}
