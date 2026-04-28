#include "heptachess_game.h"

#include <string.h>

#define HOSTILITY_CAP 20000

static int piece_value(int piece) {
	static int const values [16] = {
	  [EMPTY]      = 0,
	  [MARSHAL]    = 1200,
	  [CHANCELLOR] = 650,
	  [DIPLOMAT]   = 650,
	  [CANNON]     = 520,
	  [CROSSBOW]   = 420,
	  [BOW]        = 360,
	  [SWORD]      = 220,
	  [DAGGER]     = 220,
	  [KNIGHT]     = 480,
	  [PEDESTRIAN] = 80,
	  [EMPEROR]    = 0,
	};

	return piece >= 0 && piece < 16 ? values [piece] : 0;
}

static void score_init(HCGameScore *score) {
	memset(score, 0, sizeof(*score));
	for (int player = 1; player < MAX_COUNTRIES; ++player) score->alive [player] = 1;
}

static void material_init(HCGameState *state) {
	uint8_t const *cells = &state->board.grid [0][0];
		for (int index = 0; index < BOARD_SQ; ++index) {
			uint8_t code   = cells [index];
			int     player = code >> 4;
			if (player > 0 && player < MAX_COUNTRIES) state->score.material [player] += piece_value(code & 15);
		}
}

static void decay_hostility(HCGameScore *score) {
	int *items = &score->hostility [0][0];
		for (int index = 0; index < MAX_COUNTRIES * MAX_COUNTRIES; ++index) {
			int value = items [index];
			if (value > 0) items [index] -= value / 64 + 1;
		}
}

static void add_hostility(HCGameScore *score, int victim, int attacker, int piece_type) {
	int value = piece_value(piece_type);
	if (piece_type == MARSHAL) value += 3000;

	score->hostility [victim][attacker] += value;
	if (score->hostility [victim][attacker] > HOSTILITY_CAP) score->hostility [victim][attacker] = HOSTILITY_CAP;
}

static int is_live_player(HCGameState const *state, int player) {
	return player > 0 && player < MAX_COUNTRIES && state->score.alive [player];
}

static int fallback_transfer_winner(HCGameState const *state, int fallback, int victim) {
	if (is_live_player(state, fallback) && fallback != victim) return fallback;
	for (int player = 1; player < MAX_COUNTRIES; ++player)
		if (is_live_player(state, player) && player != victim) return player;
	return 0;
}

static int choose_transfer_winner(HCGameState const *state, int victim, int fallback) {
	int max_captures = 0;
	int latest_turn  = -1;
	int chosen       = fallback_transfer_winner(state, fallback, victim);

		for (int player = 1; player < MAX_COUNTRIES; ++player) {
			if (!is_live_player(state, player) || player == victim) continue;
			int captures = state->score.detailed_captures [victim][player];
			int turn     = state->score.last_capture_turn [victim][player];
			if (captures < max_captures) continue;
			if (captures == max_captures && turn <= latest_turn) continue;
			max_captures = captures;
			latest_turn  = turn;
			chosen       = player;
		}

	return chosen;
}

static uint8_t next_alive_player(HCGameState const *state, uint8_t current) {
		for (int offset = 1; offset < MAX_COUNTRIES; ++offset) {
			uint8_t player = ( uint8_t ) (((current + offset - 1) % 7) + 1);
			if (state->score.alive [player]) return player;
		}
	return current;
}

static void set_winner(HCGameState *state, int winner) {
	state->winner   = ( int8_t ) winner;
	state->terminal = winner != 0;
}

static void apply_capture_result(HCGameState *state, int attacker, int victim, int piece_type) {
	if (!victim || !state->score.alive [victim]) return;

	add_hostility(&state->score, victim, attacker, piece_type);

	++state->score.captures [attacker];
	++state->score.detailed_captures [victim][attacker];
	state->score.last_capture_turn [victim][attacker] = state->score.turn;
	++state->score.losses [victim];

	if (piece_type != MARSHAL && state->score.losses [victim] < 10) return;

	int transfer_winner = attacker;
	if (piece_type != MARSHAL) transfer_winner = choose_transfer_winner(state, victim, attacker);
	if (!is_live_player(state, transfer_winner) || transfer_winner == victim)
		transfer_winner = fallback_transfer_winner(state, attacker, victim);
	if (transfer_winner == 0) return;

	hc_transfer_country(&state->board, ( uint8_t ) victim, ( uint8_t ) transfer_winner);
	state->score.material [transfer_winner] += state->score.material [victim];
	state->score.material [victim]           = 0;
	state->score.alive [victim]              = 0;
	++state->score.eliminations [transfer_winner];

	if (state->score.eliminations [transfer_winner] >= 2 || state->score.captures [transfer_winner] >= 30)
		set_winner(state, transfer_winner);
}

void hc_game_init(HCGameState *state) {
	memset(state, 0, sizeof(*state));
	hc_board_init(&state->board);
	score_init(&state->score);
	material_init(state);
}

void hc_game_skip_turn(HCGameState *state) {
	decay_hostility(&state->score);
	++state->score.turn;
	state->board.current_player = next_alive_player(state, state->board.current_player);
}

void hc_game_apply_move(HCGameState *state, HCMove const *move) {
	int     attacker       = state->board.current_player;
	uint8_t captured_piece = state->board.grid [move->ty][move->tx];
	int     victim         = captured_piece >> 4;
	int     piece_type     = captured_piece & 15;
	int     captured_value = piece_value(piece_type);

	decay_hostility(&state->score);
	++state->score.turn;
	if (victim > 0 && victim < MAX_COUNTRIES) state->score.material [victim] -= captured_value;
	hc_apply_move(&state->board, move);
	state->board.current_player = next_alive_player(state, attacker);

	if (move->is_capture) apply_capture_result(state, attacker, victim, piece_type);
	if (!state->score.alive [state->board.current_player])
		state->board.current_player = next_alive_player(state, state->board.current_player);
	hc_game_check_terminal(state);
}

int hc_game_check_terminal(HCGameState *state) {
	int8_t winner;
	if (state->terminal) return 1;
	if (!hc_check_terminal(&state->board, &winner)) return 0;

	set_winner(state, winner);
	return 1;
}
