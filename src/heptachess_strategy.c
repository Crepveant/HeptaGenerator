#include "heptachess_strategy.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MOVES              512
#define MAX_ORDERED_MOVES      32
#define TT_SIZE                4096
#define TERMINAL_VALUE         1000000
#define ELIMINATED_VALUE       -1000000
#define CAPTURE_WEIGHT         180
#define ELIMINATION_WEIGHT     2400
#define LOSS_WEIGHT            35
#define HOSTILITY_MATERIAL_DIV 8333
#define LEADER_THREAT_DIV      13
#define BETRAYAL_PENALTY       180
#define LEADER_ATTACK          420
#define ROOT_RANDOM_WINDOW     50
#define MARSHAL_THREAT_PENALTY 18000
#define MARSHAL_ESCAPE_BONUS   9000
#define QUIET_STRATEGIC_BONUS  350
#define MIN_SEARCH_NODES       64

typedef struct {
	int value [MAX_COUNTRIES];
} Payoff;

typedef struct {
	HCMove move;
	int    score;
} ScoredMove;

typedef struct {
	uint64_t key;
	int      depth;
	Payoff   payoff;
	int      used;
} TTEntry;

typedef struct {
	TTEntry *table;
	int      nodes_left;
} SearchContext;

static uint64_t splitmix64(uint64_t *state) {
	uint64_t z = (*state += UINT64_C(0x9e3779b97f4a7c15));
	z          = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
	z          = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
	return z ^ (z >> 31);
}

static int random_index(uint64_t *rng, int count) { return ( int ) (splitmix64(rng) % ( uint64_t ) count); }

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

static uint64_t hash_bytes(uint64_t hash, void const *data, size_t size) {
	uint8_t const *bytes = data;
	for (size_t i = 0; i < size; ++i) hash = (hash ^ bytes [i]) * UINT64_C(1099511628211);
	return hash;
}

static uint64_t state_hash(HCGameState const *state) {
	uint64_t hash = UINT64_C(1469598103934665603);
	hash          = hash_bytes(hash, state->board.grid, sizeof(state->board.grid));
	hash          = hash_bytes(hash, &state->board.current_player, sizeof(state->board.current_player));
	hash          = hash_bytes(hash, &state->score, sizeof(state->score));
	hash          = hash_bytes(hash, &state->winner, sizeof(state->winner));
	hash          = hash_bytes(hash, &state->terminal, sizeof(state->terminal));
	return hash;
}

static int leading_player(HCGameState const *state) {
	int leader = 0;
	int value  = 0;
		for (int player = 1; player < MAX_COUNTRIES; ++player) {
			if (state->score.material [player] <= value) continue;
			leader = player;
			value  = state->score.material [player];
		}
	return leader;
}

static int find_marshal(HCGameState const *state, int player, int *out_y, int *out_x) {
	uint8_t const *cells = &state->board.grid [0][0];
		for (int index = 0; index < BOARD_SQ; ++index) {
			uint8_t code = cells [index];
			if ((code >> 4) != player || (code & 15) != MARSHAL) continue;
			*out_y = index / SIZE;
			*out_x = index % SIZE;
			return 1;
		}
	return 0;
}

static int move_list_attacks_square(uint8_t const board [SIZE][SIZE], int player, int y, int x) {
	HCMove   buffer [MAX_MOVES];
	MoveList moves = {buffer, 0, MAX_MOVES};
	generate_legal_moves(board, ( uint8_t ) player, &moves);
	for (int i = 0; i < moves.count; ++i)
		if (moves.moves [i].ty == y && moves.moves [i].tx == x) return 1;
	return 0;
}

static int marshal_threatened(HCGameState const *state, int player) {
	int y = 0;
	int x = 0;
	if (!state->score.alive [player] || !find_marshal(state, player, &y, &x)) return 1;
		for (int enemy = 1; enemy < MAX_COUNTRIES; ++enemy) {
			if (enemy == player || !state->score.alive [enemy]) continue;
			if (move_list_attacks_square(state->board.grid, enemy, y, x)) return 1;
		}
	return 0;
}

static int quiet_move_score(HCGameState const *state, HCMove const *move) {
	int player = state->board.current_player;
	if (!marshal_threatened(state, player)) return 0;

	HCGameState child = *state;
	hc_game_apply_move(&child, move);
	return marshal_threatened(&child, player) ? 0 : MARSHAL_ESCAPE_BONUS;
}

static int move_score(HCGameState const *state, HCMove const *move, int leader) {
		if (!move->is_capture) {
			int dy = move->ty > move->fy ? move->ty - move->fy : move->fy - move->ty;
			int dx = move->tx > move->fx ? move->tx - move->fx : move->fx - move->tx;
			return dy + dx + quiet_move_score(state, move);
		}

	int     player      = state->board.current_player;
	uint8_t victim_code = state->board.grid [move->ty][move->tx];
	int     target      = victim_code >> 4;
	int     piece       = victim_code & 15;
	int     score       = piece_value(piece) + 1000 + state->score.hostility [player][target] / 4;

	if (piece == MARSHAL) score += 6000;
	if (target == leader && target != player) score += LEADER_ATTACK;
	if (target != leader && state->score.hostility [player][target] < 120) score -= BETRAYAL_PENALTY;
	return score;
}

static int same_move(HCMove const *lhs, HCMove const *rhs) {
	return lhs->fy == rhs->fy && lhs->fx == rhs->fx && lhs->ty == rhs->ty && lhs->tx == rhs->tx;
}

static int ordered_contains(ScoredMove const ordered [MAX_ORDERED_MOVES], int count, HCMove const *move) {
	for (int i = 0; i < count; ++i)
		if (same_move(&ordered [i].move, move)) return 1;
	return 0;
}

static void insert_ordered(ScoredMove ordered [MAX_ORDERED_MOVES], int *count, int limit, ScoredMove candidate) {
	if (ordered_contains(ordered, *count, &candidate.move)) return;

	int pos = *count;
		while (pos > 0 && ordered [pos - 1].score < candidate.score) {
			if (pos < limit) ordered [pos] = ordered [pos - 1];
			--pos;
		}

	if (pos < limit) ordered [pos] = candidate;
	if (*count < limit) ++*count;
}

static int clamp_width(int search_budget, int depth) {
	int width = 4;
	if (search_budget >= 32) width = 8;
	if (search_budget >= 128) width = 12;
	if (search_budget >= 512) width = 16;
	if (search_budget >= 2048) width = 24;
	if (depth <= 1 && width < 24) width *= 2;
	return width > MAX_ORDERED_MOVES ? MAX_ORDERED_MOVES : width;
}

static int ordered_moves(HCGameState const *state, ScoredMove ordered [MAX_ORDERED_MOVES], int limit) {
	HCMove   buffer [MAX_MOVES];
	MoveList move_list = {buffer, 0, MAX_MOVES};
	generate_legal_moves(state->board.grid, state->board.current_player, &move_list);

	if (limit > move_list.count) limit = move_list.count;
	int count  = 0;
	int leader = leading_player(state);
		for (int i = 0; i < move_list.count; ++i) {
			ScoredMove candidate = {move_list.moves [i], move_score(state, &move_list.moves [i], leader)};
			insert_ordered(ordered, &count, limit, candidate);
		}

	return count;
}

static void
add_quiet_candidates(HCGameState const *state, ScoredMove ordered [MAX_ORDERED_MOVES], int *count, int limit) {
	HCMove   buffer [MAX_MOVES];
	MoveList move_list = {buffer, 0, MAX_MOVES};
	generate_legal_moves(state->board.grid, state->board.current_player, &move_list);

		for (int i = 0; i < move_list.count; ++i) {
			if (move_list.moves [i].is_capture) continue;
			int score = quiet_move_score(state, &move_list.moves [i]);
			if (score <= 0) continue;
			ScoredMove candidate = {move_list.moves [i], score + QUIET_STRATEGIC_BONUS};
			insert_ordered(ordered, count, limit, candidate);
		}
}

static void evaluate_terminal(HCGameState const *state, Payoff *payoff) {
	for (int player = 1; player < MAX_COUNTRIES; ++player)
		payoff->value [player] = player == state->winner ? TERMINAL_VALUE : ELIMINATED_VALUE;
}

static void evaluate_state(HCGameState const *state, Payoff *payoff) {
	memset(payoff, 0, sizeof(*payoff));
		if (state->terminal && state->winner > 0) {
			evaluate_terminal(state, payoff);
			return;
		}

	int leader = leading_player(state);
		for (int player = 1; player < MAX_COUNTRIES; ++player) {
			int value = state->score.material [player];
			if (!state->score.alive [player]) value += ELIMINATED_VALUE;
			value                  += state->score.captures [player] * CAPTURE_WEIGHT;
			value                  += state->score.eliminations [player] * ELIMINATION_WEIGHT;
			value                  -= state->score.losses [player] * LOSS_WEIGHT;
			payoff->value [player]  = value;
			if (state->score.alive [player] && marshal_threatened(state, player))
				payoff->value [player] -= MARSHAL_THREAT_PENALTY;
		}

		for (int player = 1; player < MAX_COUNTRIES; ++player) {
			if (leader != 0 && leader != player)
				payoff->value [player]
				  -= (state->score.material [leader] - state->score.material [player]) / LEADER_THREAT_DIV;
			for (int target = 1; target < MAX_COUNTRIES; ++target)
				if (target != player)
					payoff->value [player] -= state->score.hostility [player][target]
					                        * state->score.material [target]
					                        / HOSTILITY_MATERIAL_DIV;
		}
}

static int select_depth(int search_budget) {
	if (search_budget >= 2048) return 4;
	if (search_budget >= 256) return 3;
	if (search_budget >= 32) return 2;
	return 1;
}

static int tt_lookup(TTEntry table [TT_SIZE], uint64_t key, int depth, Payoff *payoff) {
	TTEntry *entry = &table [key & (TT_SIZE - 1)];
	if (!entry->used || entry->key != key || entry->depth < depth) return 0;
	*payoff = entry->payoff;
	return 1;
}

static void tt_store(TTEntry table [TT_SIZE], uint64_t key, int depth, Payoff payoff) {
	TTEntry *entry = &table [key & (TT_SIZE - 1)];
	entry->used    = 1;
	entry->key     = key;
	entry->depth   = depth;
	entry->payoff  = payoff;
}

static Payoff maxn(HCGameState const *state, int depth, int search_budget, SearchContext *ctx) {
	HCGameState current = *state;
	Payoff      payoff;
	uint64_t    key = state_hash(&current);
	if (tt_lookup(ctx->table, key, depth, &payoff)) return payoff;
		if (ctx->nodes_left-- <= 0) {
			evaluate_state(&current, &payoff);
			return payoff;
		}

		if (hc_game_check_terminal(&current) || depth == 0) {
			evaluate_state(&current, &payoff);
			tt_store(ctx->table, key, depth, payoff);
			return payoff;
		}

	ScoredMove ordered [MAX_ORDERED_MOVES];
	int        count = ordered_moves(&current, ordered, clamp_width(search_budget, depth));
	add_quiet_candidates(&current, ordered, &count, clamp_width(search_budget, depth));
		if (count == 0) {
			hc_game_skip_turn(&current);
			return maxn(&current, depth - 1, search_budget, ctx);
		}

	int    player      = current.board.current_player;
	Payoff best        = {0};
	int    best_payoff = -TERMINAL_VALUE * 2;
		for (int i = 0; i < count; ++i) {
			HCGameState child = current;
			hc_game_apply_move(&child, &ordered [i].move);
			payoff = maxn(&child, depth - 1, search_budget, ctx);
			if (payoff.value [player] <= best_payoff) continue;
			best        = payoff;
			best_payoff = payoff.value [player];
		}

	tt_store(ctx->table, key, depth, best);
	return best;
}

HCMove hc_strategy_select(HCGameState const *state, int search_budget, uint64_t *rng) {
	int        depth = select_depth(search_budget);
	ScoredMove ordered [MAX_ORDERED_MOVES];
	int        count = ordered_moves(state, ordered, clamp_width(search_budget, depth));
	add_quiet_candidates(state, ordered, &count, clamp_width(search_budget, depth));
	if (count == 0) return (HCMove) {0};

	TTEntry       table [TT_SIZE] = {0};
	SearchContext ctx             = {table, search_budget > MIN_SEARCH_NODES ? search_budget : MIN_SEARCH_NODES};
	int           player          = state->board.current_player;
	HCMove        best_move       = ordered [0].move;
	int           best_payoff     = -TERMINAL_VALUE * 2;
	int           close_count     = 0;

		for (int i = 0; i < count; ++i) {
			HCGameState child = *state;
			hc_game_apply_move(&child, &ordered [i].move);
			Payoff payoff = maxn(&child, depth - 1, search_budget, &ctx);
			int    value  = payoff.value [player];
				if (i == 0) {
					best_move   = ordered [i].move;
					best_payoff = value;
					close_count = 1;
					continue;
				}
			int window = abs(best_payoff / 50);
			if (window < ROOT_RANDOM_WINDOW) window = ROOT_RANDOM_WINDOW;
				if (value > best_payoff + window) {
					best_move   = ordered [i].move;
					best_payoff = value;
					close_count = 1;
					continue;
				}
			if (value < best_payoff - window) continue;
			++close_count;
			if (random_index(rng, close_count) == 0) best_move = ordered [i].move;
		}

	return best_move;
}
