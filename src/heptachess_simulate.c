#include "heptachess_game.h"
#include "heptachess_moves.h"
#include "heptachess_strategy.h"
#include "numpy_gen.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_MOVES 512

typedef struct {
	uint8_t (*boards) [SIZE][SIZE];
	int16_t (*moves) [5];
	int8_t *players;
} GameTrace;

static uint64_t splitmix64(uint64_t *state) {
	uint64_t z = (*state += UINT64_C(0x9e3779b97f4a7c15));
	z          = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
	z          = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
	return z ^ (z >> 31);
}

static uint64_t seed_from_prefix(char const *prefix) {
	uint64_t hash = UINT64_C(1469598103934665603);
		while (*prefix) {
			hash ^= ( unsigned char ) *prefix++;
			hash *= UINT64_C(1099511628211);
		}
	return splitmix64(&hash);
}

static int trace_alloc(GameTrace *trace) {
	trace->boards  = malloc(sizeof(*trace->boards) * HC_MAX_GAME_STEPS);
	trace->moves   = malloc(sizeof(int16_t) * HC_MAX_GAME_STEPS * 5);
	trace->players = malloc(sizeof(int8_t) * HC_MAX_GAME_STEPS);

	return trace->boards && trace->moves && trace->players;
}

static void trace_free(GameTrace *trace) {
	free(trace->boards);
	free(trace->moves);
	free(trace->players);
}

static void trace_record(GameTrace *trace, int step, HCBoard const *board, HCMove const *move) {
	memcpy(trace->boards [step], board->grid, sizeof(board->grid));
	trace->moves [step][0] = move->fy;
	trace->moves [step][1] = move->fx;
	trace->moves [step][2] = move->ty;
	trace->moves [step][3] = move->tx;
	trace->moves [step][4] = move->is_capture;
	trace->players [step]  = board->current_player;
}

static void write_game_outputs(char const *prefix, int steps, GameTrace const *trace, int8_t const *winner) {
	char path [256];

	snprintf(path, sizeof(path), "%s_boards.npy", prefix);
	write_npy_3d_u8(path, steps, SIZE, SIZE, &trace->boards [0][0][0]);

	snprintf(path, sizeof(path), "%s_moves.npy", prefix);
	write_npy_2d(path, steps, 5, &trace->moves [0][0]);

	snprintf(path, sizeof(path), "%s_players.npy", prefix);
	write_npy_1d(path, steps, &trace->players [0]);

	snprintf(path, sizeof(path), "%s_winner.npy", prefix);
	write_npy_1d(path, 1, winner);
}

int simulate_game(char const *prefix, int search_budget) {
	fflush(stdout);

	HCGameState state;
	hc_game_init(&state);
	uint64_t  rng   = seed_from_prefix(prefix);

	GameTrace trace = {0};
		if (!trace_alloc(&trace)) {
			trace_free(&trace);
			fprintf(stderr, "Memory allocation failed.\n");
			return -1;
		}

	HCMove   move_buffer [MAX_MOVES];
	MoveList move_list = {move_buffer, 0, MAX_MOVES};
	int      step      = 0;

		while (step < HC_MAX_GAME_STEPS) {
			if (hc_game_check_terminal(&state)) break;

			generate_legal_moves(state.board.grid, state.board.current_player, &move_list);
				if (move_list.count == 0) {
					hc_game_skip_turn(&state);
					continue;
				}

			HCMove move = hc_strategy_select(&state, search_budget, &rng);
			trace_record(&trace, step, &state.board, &move);

			if (move.fy == 0 && move.fx == 0 && move.ty == 0 && move.tx == 0 && !move.is_capture)
				fprintf(stderr, "Strategy returned a dummy move at step %d.\n", step);

			hc_game_apply_move(&state, &move);
			++step;
		}

	write_game_outputs(prefix, step, &trace, &state.winner);
	printf("Saved game with %d steps, winner = %d\n", step, state.winner);

	trace_free(&trace);
	return step;
}
