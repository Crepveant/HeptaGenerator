#ifndef HEPTACHESS_GAME_H
#define HEPTACHESS_GAME_H

#include "heptachess_board.h"

/** Maximum recorded moves per generated game. */
#define HC_MAX_GAME_STEPS 1024

/** Rule, material, and diplomacy state derived from the board history. */
typedef struct {
	int     alive [MAX_COUNTRIES];
	int     eliminations [MAX_COUNTRIES];
	int     captures [MAX_COUNTRIES];
	int     losses [MAX_COUNTRIES];
	int     material [MAX_COUNTRIES];
	int     turn;
	int     hostility [MAX_COUNTRIES][MAX_COUNTRIES];
	int     last_capture_turn [MAX_COUNTRIES][MAX_COUNTRIES];
	uint8_t detailed_captures [MAX_COUNTRIES][MAX_COUNTRIES];
} HCGameScore;

/** Complete mutable game state used by simulation and strategy search. */
typedef struct {
	HCBoard     board;
	HCGameScore score;
	int8_t      winner;
	int         terminal;
} HCGameState;

#ifdef __cplusplus
extern "C"
{
#endif

/** Initialize a complete game state from the standard starting position. */
void hc_game_init(HCGameState *state);

/** Advance the side to move when the current player has no legal moves. */
void hc_game_skip_turn(HCGameState *state);

/** Apply a legal move and update elimination, transfer, diplomacy, and winner state. */
void hc_game_apply_move(HCGameState *state, HCMove const *move);

/** Refresh terminal state from current board occupancy. */
int  hc_game_check_terminal(HCGameState *state);

#ifdef __cplusplus
}
#endif

#endif
