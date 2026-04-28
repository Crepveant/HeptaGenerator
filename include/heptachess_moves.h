#ifndef HEPTACHESS_MOVES_H
#define HEPTACHESS_MOVES_H

#include <stdint.h>

#define SIZE     19
#define BOARD_SQ (SIZE * SIZE)

/** Piece code stored in the low 4 bits of an encoded board cell. */
enum PieceCode
{
	EMPTY      = 0,
	MARSHAL    = 1,
	CHANCELLOR = 2,
	DIPLOMAT   = 3,
	CANNON     = 4,
	CROSSBOW   = 5,
	BOW        = 6,
	SWORD      = 7,
	DAGGER     = 8,
	KNIGHT     = 9,
	PEDESTRIAN = 10,
	EMPEROR    = 11,
};

/** Board move encoded as from_y, from_x, to_y, to_x, and capture flag. */
typedef struct {
	uint8_t fy, fx, ty, tx;
	uint8_t is_capture;
} HCMove;

/** Caller-owned output buffer for generated legal moves. */
typedef struct {
	HCMove *moves;
	int     count;
	int     capacity;
} MoveList;

#ifdef __cplusplus
extern "C"
{
#endif

/** Fill caller-owned output with legal moves for player on board. */
void generate_legal_moves(uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out);

#ifdef __cplusplus
}
#endif

#endif
