#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <stdint.h>
#include "heptachess_board.h"

#define MAX_PLAYERS 8
#define MAX_PIECES 16

#ifdef __cplusplus
extern "C" {
#endif

extern uint64_t zobrist_table[MAX_PLAYERS][MAX_PIECES][SIZE][SIZE];

void init_zobrist();
uint64_t zobrist_hash(const HCBoard* b);

#ifdef __cplusplus
}
#endif

#endif