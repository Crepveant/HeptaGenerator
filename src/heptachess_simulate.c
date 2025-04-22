#include "heptachess_board.h"
#include "heptachess_moves.h"
#include "numpy_gen.h"
#include "mcts.h"
#include "zobrist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define MAX_STEPS 1024
#define MAX_MOVES 512
#define PIECE_MARSHAL 1

int simulate_game(const char* prefix, int sims_per_move) {
    srand(time(NULL));
    init_zobrist();
    fflush(stdout);
    HCBoard board;
    hc_board_init(&board);

    // Heap-allocated buffers to avoid stack overflow
    uint8_t (*states)[160][19][19] = malloc(sizeof(uint8_t) * MAX_STEPS * 160 * 19 * 19);
    int16_t (*moves)[5] = malloc(sizeof(int16_t) * MAX_STEPS * 5);
    int8_t* players = malloc(sizeof(int8_t) * MAX_STEPS);
    int8_t winner = 0;

    if (!states || !moves || !players) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    int step = 0;

    int alive[8] = {0,1,1,1,1,1,1,1};
    int elim_count[8] = {0};
    int capture_count[8] = {0};
    int lost_count[8] = {0};

    uint8_t detailed[8][8] = {0};

    HCMove movebuf[MAX_MOVES];
    MoveList mvlist = { movebuf, 0, MAX_MOVES };

    while (step < MAX_STEPS) {
        int8_t term;
        if (hc_check_terminal(&board, &term)) {
            winner = term;
            break;
        }
        generate_legal_moves(board.grid, board.current_player, &mvlist);
        if (mvlist.count == 0) {
            board.current_player = (board.current_player % 7) + 1;
            continue;
        }
        HCMove mv = hc_mcts_select(&board, sims_per_move);

        /*printf("Step %3d | Player %d | Move %2d,%2d -> %2d,%2d | capture=%d\n",
            step,
            board.current_player,
            mv.fy, mv.fx,
            mv.ty, mv.tx,
            mv.is_capture);
        fflush(stdout);*/

        // record state
        hc_encode_board(&board, states[step]);
        moves[step][0] = mv.fy;
        moves[step][1] = mv.fx;
        moves[step][2] = mv.ty;
        moves[step][3] = mv.tx;
        moves[step][4] = mv.is_capture;
        players[step] = board.current_player;

        if (mv.fy == 0 && mv.fx == 0 && mv.ty == 0 && mv.tx == 0 && !mv.is_capture) {
            fprintf(stderr, "⚠️ MCTS returned dummy move. Step = %d\n", step);
        }        

        // uint8_t src = board.grid[mv.fy][mv.fx]; //unused
        uint8_t dst = board.grid[mv.ty][mv.tx];
        int attacker = board.current_player;
        int victim = (dst >> 4);
        int type = (dst & 15);

        hc_apply_move(&board, &mv);
        step++;

        if (mv.is_capture && victim && alive[victim]) {
            capture_count[attacker]++;
            detailed[victim][attacker]++;
            lost_count[victim]++;

            if (type == PIECE_MARSHAL || lost_count[victim] >= 10) {
                int chosen = attacker;
                if (type != PIECE_MARSHAL) {
                    int max = 0;
                    for (int p = 1; p <= 7; ++p)
                        if (detailed[victim][p] > max) max = detailed[victim][p];
                    for (int rev = step - 1; rev >= 0; --rev) {
                        int ply = players[rev];
                        if (detailed[victim][ply] == max) {
                            chosen = ply;
                            break;
                        }
                    }
                }
                hc_transfer_country(&board, victim, chosen);
                alive[victim] = 0;
                elim_count[chosen]++;

                if (elim_count[chosen] >= 2 || capture_count[chosen] >= 30) {
                    winner = chosen;
                    break;
                }
            }
        }
    }

    // write outputs
    char path[256];
    snprintf(path, sizeof(path), "%s_states.npy", prefix);
    write_npy_4d(path, step, 160, 19, 19, &states[0][0][0][0]);
    snprintf(path, sizeof(path), "%s_moves.npy", prefix);
    write_npy_2d(path, step, 5, &moves[0][0]);
    snprintf(path, sizeof(path), "%s_players.npy", prefix);
    write_npy_1d(path, step, &players[0]);
    snprintf(path, sizeof(path), "%s_winner.npy", prefix);
    write_npy_1d(path, 1, &winner);

    printf("Saved game with %d steps, winner = %d\n", step, winner);

    // cleanup
    free(states);
    free(moves);
    free(players);

    return step;
}