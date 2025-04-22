#include "heptachess_moves.h"
#include <string.h>

#define ADD_MOVE(y0, x0, y1, x1, is_cap) \
    do { \
        if (out->count < out->capacity) { \
            out->moves[out->count++] = (HCMove){y0, x0, y1, x1, is_cap}; \
        } \
    } while (0)

static const int DIR_Y[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
static const int DIR_X[8] = { 0, 0,-1, 1, -1,  1,-1, 1};

static const int KNIGHT_DIAG[4][2][2] = {
    {{-1, -1}, {-1, 1}},
    {{ 1, -1}, { 1, 1}},
    {{-1, -1}, { 1, -1}},
    {{-1,  1}, { 1,  1}},
};

static inline int get_player(uint8_t code) { return code >> 4; }
static inline int get_type(uint8_t code) { return code & 15; }

void generate_legal_moves(uint8_t board[SIZE][SIZE], uint8_t player, MoveList* out) {
    out->count = 0;

    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            uint8_t code = board[y][x];
            if (code == EMPTY || get_player(code) != player) continue;
            int pt = get_type(code);

            if (pt == MARSHAL || pt == CHANCELLOR || pt == DIPLOMAT || pt == CROSSBOW || pt == BOW) {
                int dir_start = 0, dir_end = 8, max_range = 19;
                if (pt == CHANCELLOR) dir_end = 4;
                if (pt == DIPLOMAT) dir_start = 4;
                if (pt == CROSSBOW) max_range = 5;
                if (pt == BOW) max_range = 4;

                for (int d = dir_start; d < dir_end; ++d) {
                    int ny = y + DIR_Y[d], nx = x + DIR_X[d], step = 1;
                    while (ny >= 0 && ny < SIZE && nx >= 0 && nx < SIZE && step <= max_range) {
                        uint8_t dst = board[ny][nx];
                        if (dst == EMPTY) ADD_MOVE(y, x, ny, nx, 0);
                        else {
                            int t = get_type(dst), p = get_player(dst);
                            if (t != PEDESTRIAN && t != EMPEROR && p != player)
                                ADD_MOVE(y, x, ny, nx, 1);
                            break;
                        }
                        ny += DIR_Y[d]; nx += DIR_X[d]; step++;
                    }
                }
            }
            else if (pt == CANNON) {
                for (int d = 0; d < 4; ++d) {
                    int ny = y + DIR_Y[d], nx = x + DIR_X[d];
                    while (ny >= 0 && ny < SIZE && nx >= 0 && nx < SIZE) {
                        if (board[ny][nx] == EMPTY) {
                            ADD_MOVE(y, x, ny, nx, 0);
                            ny += DIR_Y[d]; nx += DIR_X[d];
                        } else break;
                    }
                    ny += DIR_Y[d]; nx += DIR_X[d];
                    while (ny >= 0 && ny < SIZE && nx >= 0 && nx < SIZE) {
                        uint8_t dst = board[ny][nx];
                        if (dst == EMPTY) {
                            ny += DIR_Y[d]; nx += DIR_X[d]; continue;
                        }
                        int t = get_type(dst), p = get_player(dst);
                        if (t != PEDESTRIAN && t != EMPEROR && p != player)
                            ADD_MOVE(y, x, ny, nx, 1);
                        break;
                    }
                }
            }
            else if (pt == SWORD || pt == DAGGER) {
                int dir_start = (pt == SWORD) ? 0 : 4;
                int dir_end = dir_start + 4;
                for (int d = dir_start; d < dir_end; ++d) {
                    int ny = y + DIR_Y[d], nx = x + DIR_X[d];
                    if (ny >= 0 && ny < SIZE && nx >= 0 && nx < SIZE) {
                        uint8_t dst = board[ny][nx];
                        if (dst == EMPTY) ADD_MOVE(y, x, ny, nx, 0);
                        else if (get_type(dst) != PEDESTRIAN && get_type(dst) != EMPEROR && get_player(dst) != player)
                            ADD_MOVE(y, x, ny, nx, 1);
                    }
                }
            }
            else if (pt == KNIGHT) {
                for (int d = 0; d < 4; ++d) {
                    int ly = y + DIR_Y[d], lx = x + DIR_X[d];
                    if (ly < 0 || ly >= SIZE || lx < 0 || lx >= SIZE || board[ly][lx] != EMPTY) continue;
                    for (int diag = 0; diag < 2; ++diag) {
                        int dy = KNIGHT_DIAG[d][diag][0], dx = KNIGHT_DIAG[d][diag][1];
                        for (int k = 1; k <= 3; ++k) {
                            int ny = ly + k * dy, nx = lx + k * dx;
                            if (ny < 0 || ny >= SIZE || nx < 0 || nx >= SIZE) break;
                            if (k > 1) {
                                int my = ly + (k - 1) * dy, mx = lx + (k - 1) * dx;
                                if (board[my][mx] != EMPTY) break;
                            }
                            uint8_t dst = board[ny][nx];
                            if (dst == EMPTY) ADD_MOVE(y, x, ny, nx, 0);
                            else {
                                int t = get_type(dst), p = get_player(dst);
                                if (t != PEDESTRIAN && t != EMPEROR && p != player)
                                    ADD_MOVE(y, x, ny, nx, 1);
                                break;
                            }
                        }
                    }
                }
            }
            else if (pt == PEDESTRIAN) {
                for (int d = 0; d < 8; ++d) {
                    int ny = y + DIR_Y[d], nx = x + DIR_X[d];
                    while (ny >= 0 && ny < SIZE && nx >= 0 && nx < SIZE && board[ny][nx] == EMPTY) {
                        ADD_MOVE(y, x, ny, nx, 0);
                        ny += DIR_Y[d]; nx += DIR_X[d];
                    }
                }
            }
        }
    }
}
