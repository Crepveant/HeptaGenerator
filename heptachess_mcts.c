// heptachess_mcts.c — Light‑v1 MCTS (random rollout based)
#include "heptachess_board.h"
#include "heptachess_moves.h"
#include <stdlib.h>
#include <string.h>

#define MAX_MOVES 512
#define MAX_ROUNDS 512

static int random_rollout(HCBoard b) {
    HCMove buf[MAX_MOVES];
    MoveList mvlist = { buf, 0, MAX_MOVES };
    int alive[8] = {0,1,1,1,1,1,1,1};
    int elim_count[8] = {0};
    int capture_count[8] = {0};
    int lost_count[8] = {0};
    uint8_t detail[8][8] = {0};

    for (int step = 0; step < MAX_ROUNDS; ++step) {
        int8_t win;
        if (hc_check_terminal(&b, &win)) return win;

        generate_legal_moves(b.grid, b.current_player, &mvlist);
        if (mvlist.count == 0) {
            b.current_player = (b.current_player % 7) + 1;
            continue;
        }

        HCMove mv = mvlist.moves[rand() % mvlist.count];
        uint8_t dst = b.grid[mv.ty][mv.tx];
        int victim = dst >> 4;
        int type = dst & 15;

        hc_apply_move(&b, &mv);

        if (mv.is_capture && victim && alive[victim]) {
            capture_count[b.current_player]++;
            detail[victim][b.current_player]++;
            lost_count[victim]++;

            if (type == 1 || lost_count[victim] >= 10) {
                int killer = b.current_player;
                if (type != 1) {
                    int max = 0;
                    for (int p = 1; p <= 7; ++p)
                        if (detail[victim][p] > max) max = detail[victim][p];
                    killer = 1;
                    while (killer <= 7 && detail[victim][killer] != max) ++killer;
                }
                hc_transfer_country(&b, victim, killer);
                alive[victim] = 0;
                elim_count[killer]++;

                if (elim_count[killer] >= 2 || capture_count[killer] >= 30)
                    return killer;
            }
        }
    }
    return 0;  // draw
}

HCMove hc_mcts_select(const HCBoard* orig, int sims_per_move) {
    HCMove buf[MAX_MOVES];
    MoveList mvlist = { buf, 0, MAX_MOVES };
    generate_legal_moves((uint8_t (*)[19])orig->grid, orig->current_player, &mvlist);

    int counts[MAX_MOVES] = {0};
    int wins[MAX_MOVES] = {0};

    for (int i = 0; i < mvlist.count; ++i) {
        for (int s = 0; s < sims_per_move; ++s) {
            HCBoard sim = *orig;
            hc_apply_move(&sim, &mvlist.moves[i]);
            int winner = random_rollout(sim);
            counts[i]++;
            if (winner == orig->current_player) wins[i]++;
        }
    }

    int best = 0;
    double best_rate = -1.0;
    for (int i = 0; i < mvlist.count; ++i) {
        double rate = (counts[i] > 0) ? ((double)wins[i] / counts[i]) : 0;
        if (rate > best_rate) {
            best = i;
            best_rate = rate;
        }
    }

    return mvlist.moves[best];
}
