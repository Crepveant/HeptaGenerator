#include "zobrist.h"
#include <stdlib.h>

uint64_t zobrist_table[MAX_PLAYERS][MAX_PIECES][SIZE][SIZE];

void init_zobrist() {
    for (int p = 0; p < MAX_PLAYERS; ++p) {
        for (int t = 0; t < MAX_PIECES; ++t) {
            for (int y = 0; y < SIZE; ++y) {
                for (int x = 0; x < SIZE; ++x) {
                    uint64_t hi = ((uint64_t)rand()) << 32;
                    uint64_t lo = (uint64_t)rand();
                    zobrist_table[p][t][y][x] = hi | lo;
                }
            }
        }
    }
}

uint64_t zobrist_hash(const HCBoard* b) {
    uint64_t h = 0;
    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            uint8_t code = b->grid[y][x];
            if (!code) continue;
            int player = code >> 4;
            int piece = code & 15;
            h ^= zobrist_table[player][piece][y][x];
        }
    }
    return h;
}