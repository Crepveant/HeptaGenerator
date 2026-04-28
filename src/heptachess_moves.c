#include "heptachess_moves.h"

static int const DIR_Y [8]             = {-1, 1, 0, 0, -1, -1, 1, 1};
static int const DIR_X [8]             = {0, 0, -1, 1, -1, 1, -1, 1};

static int const KNIGHT_DIAG [4][2][2] = {
  {{-1, -1}, {-1, 1}},
  {{1, -1},  {1, 1} },
  {{-1, -1}, {1, -1}},
  {{-1, 1},  {1, 1} },
};

static int  get_player(uint8_t code) { return code >> 4; }

static int  get_type(uint8_t code) { return code & 15; }

static int  inside_board(int y, int x) { return y >= 0 && y < SIZE && x >= 0 && x < SIZE; }

static void add_move(MoveList *out, int y0, int x0, int y1, int x1, int is_capture) {
	if (out->count >= out->capacity) return;
	out->moves [out->count++]
	  = (HCMove) {( uint8_t ) y0, ( uint8_t ) x0, ( uint8_t ) y1, ( uint8_t ) x1, ( uint8_t ) is_capture};
}

static int can_capture(uint8_t dst, uint8_t player) {
	int type = get_type(dst);
	return dst != EMPTY && get_player(dst) != player && type != PEDESTRIAN && type != EMPEROR;
}

static void
add_step_target(uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out, int y, int x, int ny, int nx) {
	if (!inside_board(ny, nx)) return;

	uint8_t dst = board [ny][nx];
	if (dst == EMPTY) add_move(out, y, x, ny, nx, 0);
	if (can_capture(dst, player)) add_move(out, y, x, ny, nx, 1);
}

static void
add_slide_ray(uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out, int y, int x, int dir, int max_range) {
	int ny   = y + DIR_Y [dir];
	int nx   = x + DIR_X [dir];
	int step = 1;

		while (inside_board(ny, nx) && step <= max_range) {
			uint8_t dst = board [ny][nx];
			if (dst == EMPTY) add_move(out, y, x, ny, nx, 0);
				if (dst != EMPTY) {
					if (can_capture(dst, player)) add_move(out, y, x, ny, nx, 1);
					return;
				}
			ny += DIR_Y [dir];
			nx += DIR_X [dir];
			++step;
		}
}

static void add_slider_moves(uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out, int y, int x, int type) {
	int dir_start = 0;
	int dir_end   = 8;
	int max_range = SIZE;

	if (type == CHANCELLOR) dir_end = 4;
	if (type == DIPLOMAT) dir_start = 4;
	if (type == CROSSBOW) max_range = 5;
	if (type == BOW) max_range = 4;

	for (int dir = dir_start; dir < dir_end; ++dir) add_slide_ray(board, player, out, y, x, dir, max_range);
}

static void
add_cannon_quiet_ray(uint8_t const board [SIZE][SIZE], MoveList *out, int y, int x, int dir, int *ny, int *nx) {
		while (inside_board(*ny, *nx) && board [*ny][*nx] == EMPTY) {
			add_move(out, y, x, *ny, *nx, 0);
			*ny += DIR_Y [dir];
			*nx += DIR_X [dir];
		}
}

static void add_cannon_capture_ray(
  uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out, int y, int x, int dir, int ny, int nx
) {
	ny += DIR_Y [dir];
	nx += DIR_X [dir];
		while (inside_board(ny, nx)) {
			uint8_t dst = board [ny][nx];
			if (can_capture(dst, player)) add_move(out, y, x, ny, nx, 1);
			if (dst != EMPTY) return;
			ny += DIR_Y [dir];
			nx += DIR_X [dir];
		}
}

static void add_cannon_moves(uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out, int y, int x) {
		for (int dir = 0; dir < 4; ++dir) {
			int ny = y + DIR_Y [dir];
			int nx = x + DIR_X [dir];
			add_cannon_quiet_ray(board, out, y, x, dir, &ny, &nx);
			add_cannon_capture_ray(board, player, out, y, x, dir, ny, nx);
		}
}

static void
add_step_moves(uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out, int y, int x, int dir_start) {
	for (int dir = dir_start; dir < dir_start + 4; ++dir)
		add_step_target(board, player, out, y, x, y + DIR_Y [dir], x + DIR_X [dir]);
}

static int blocked_knight_leg(uint8_t const board [SIZE][SIZE], int y, int x) {
	return !inside_board(y, x) || board [y][x] != EMPTY;
}

static void add_knight_diag(
  uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out, int y, int x, int ly, int lx, int dy, int dx
) {
		for (int step = 1; step <= 3; ++step) {
			int ny = ly + step * dy;
			int nx = lx + step * dx;
			if (!inside_board(ny, nx)) return;
			if (step > 1 && board [ly + (step - 1) * dy][lx + (step - 1) * dx] != EMPTY) return;
			add_step_target(board, player, out, y, x, ny, nx);
			if (board [ny][nx] != EMPTY) return;
		}
}

static void add_knight_moves(uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out, int y, int x) {
		for (int dir = 0; dir < 4; ++dir) {
			int ly = y + DIR_Y [dir];
			int lx = x + DIR_X [dir];
			if (blocked_knight_leg(board, ly, lx)) continue;
			for (int diag = 0; diag < 2; ++diag)
				add_knight_diag(
				  board, player, out, y, x, ly, lx, KNIGHT_DIAG [dir][diag][0], KNIGHT_DIAG [dir][diag][1]
				);
		}
}

static void add_pedestrian_ray(uint8_t const board [SIZE][SIZE], MoveList *out, int y, int x, int dir) {
	int ny = y + DIR_Y [dir];
	int nx = x + DIR_X [dir];
		while (inside_board(ny, nx) && board [ny][nx] == EMPTY) {
			add_move(out, y, x, ny, nx, 0);
			ny += DIR_Y [dir];
			nx += DIR_X [dir];
		}
}

static void add_pedestrian_moves(uint8_t const board [SIZE][SIZE], MoveList *out, int y, int x) {
	for (int dir = 0; dir < 8; ++dir) add_pedestrian_ray(board, out, y, x, dir);
}

static void add_piece_moves(uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out, int y, int x, int type) {
	if (type == MARSHAL || type == CHANCELLOR || type == DIPLOMAT || type == CROSSBOW || type == BOW)
		add_slider_moves(board, player, out, y, x, type);
	if (type == CANNON) add_cannon_moves(board, player, out, y, x);
	if (type == SWORD) add_step_moves(board, player, out, y, x, 0);
	if (type == DAGGER) add_step_moves(board, player, out, y, x, 4);
	if (type == KNIGHT) add_knight_moves(board, player, out, y, x);
	if (type == PEDESTRIAN) add_pedestrian_moves(board, out, y, x);
}

void generate_legal_moves(uint8_t const board [SIZE][SIZE], uint8_t player, MoveList *out) {
	out->count = 0;

		for (int y = 0; y < SIZE; ++y) {
				for (int x = 0; x < SIZE; ++x) {
					uint8_t code = board [y][x];
					if (code == EMPTY || get_player(code) != player) continue;
					add_piece_moves(board, player, out, y, x, get_type(code));
				}
		}
}
