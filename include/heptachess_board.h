#ifndef HEPTACHESS_BOARD_H
#define HEPTACHESS_BOARD_H

#include <stdint.h>
#include "heptachess_moves.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_COUNTRIES 8  // player 0–7 (0 = empty)

// Encoded piece: high 4 bits = player (1–7), low 4 bits = piece code (1–11)
typedef struct {
    uint8_t grid[SIZE][SIZE];     // 19×19 grid
    uint8_t current_player;       // 1–7
} HCBoard;

// Initialize starting board layout
void hc_board_init(HCBoard* b);

// Apply a move (must be legal), advance player
void hc_apply_move(HCBoard* b, const HCMove* m);

// Transfer ownership of all pieces from `loser` to `winner`
void hc_transfer_country(HCBoard* b, uint8_t loser, uint8_t winner);

// Copy board into encoded 160x19x19 tensor for NN input
void hc_encode_board(const HCBoard* b, uint8_t out[160][19][19]);

// Check terminal condition (1 survivor or domination), return winner if any
int hc_check_terminal(HCBoard* b, int8_t* out_winner);

#ifdef __cplusplus
}
#endif

#endif