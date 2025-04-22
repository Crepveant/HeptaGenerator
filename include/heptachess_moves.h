#ifndef HEPTACHESS_MOVES_H
#define HEPTACHESS_MOVES_H

#include <stdint.h>

#define SIZE 19
#define BOARD_SQ (SIZE * SIZE)

// Piece code enum (low 4 bits)
enum PieceCode {
    EMPTY = 0,
    MARSHAL = 1,
    CHANCELLOR = 2,
    DIPLOMAT = 3,
    CANNON = 4,
    CROSSBOW = 5,
    BOW = 6,
    SWORD = 7,
    DAGGER = 8,
    KNIGHT = 9,
    PEDESTRIAN = 10,
    EMPEROR = 11
};

// A move = from_y, from_x, to_y, to_x, is_capture
typedef struct {
    uint8_t fy, fx, ty, tx;
    uint8_t is_capture;
} HCMove;

// Output buffer (caller allocates)
typedef struct {
    HCMove* moves;
    int count;
    int capacity;
} MoveList;

#ifdef __cplusplus
extern "C" {
#endif

// Main entry point: fills moves for given board/player
void generate_legal_moves(uint8_t board[SIZE][SIZE], uint8_t player, MoveList* out);

#ifdef __cplusplus
}
#endif

#endif
