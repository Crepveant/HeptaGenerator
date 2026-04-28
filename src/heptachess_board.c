#include "heptachess_board.h"
#include "initial_board.h"

#include <string.h>

void hc_board_init(HCBoard *b) {
	memcpy(b->grid, INITIAL_GRID, sizeof(b->grid));
	b->current_player = 1;
}

void hc_apply_move(HCBoard *b, HCMove const *m) {
	uint8_t piece          = b->grid [m->fy][m->fx];
	b->grid [m->ty][m->tx] = piece;
	b->grid [m->fy][m->fx] = 0;
	b->current_player      = (b->current_player % 7) + 1;
}

void hc_transfer_country(HCBoard *b, uint8_t loser, uint8_t winner) {
		for (int y = 0; y < SIZE; ++y) {
				for (int x = 0; x < SIZE; ++x) {
					uint8_t code = b->grid [y][x];
					if ((code >> 4) == loser) b->grid [y][x] = (winner << 4) | (code & 0xf);
				}
		}
}

void hc_encode_board(HCBoard const *b, uint8_t out [160][19][19]) {
	memset(out, 0, sizeof(uint8_t) * 160 * 19 * 19);
	hc_encode_board_features(b, out);
}

void hc_encode_board_features(HCBoard const *b, uint8_t out [160][19][19]) {
		for (int y = 0; y < 19; ++y) {
				for (int x = 0; x < 19; ++x) {
					uint8_t code = b->grid [y][x];
					if (code && code < 128) out [code][y][x] = 1;
				}
		}
	if (b->current_player >= 1 && b->current_player <= 7) out [127 + b->current_player][0][0] = 1;
}

int hc_check_terminal(HCBoard const *b, int8_t *out_winner) {
	int alive [8] = {0};
		for (int y = 0; y < SIZE; ++y) {
				for (int x = 0; x < SIZE; ++x) {
					uint8_t code = b->grid [y][x];
					if (code == 0) continue;
					alive [code >> 4] = 1;
				}
		}
	int count = 0, last = 0;
		for (int i = 1; i <= 7; ++i) {
				if (alive [i]) {
					count++;
					last = i;
				}
		}
		if (count == 1) {
			*out_winner = last;
			return 1;
		}
	return 0;
}
