#ifndef MCTS_H
#define MCTS_H

#include "heptachess_board.h"
#include "heptachess_moves.h"

#ifdef __cplusplus
extern "C" {
#endif

HCMove hc_mcts_select(const HCBoard* board, int sims_per_move);

#ifdef __cplusplus
}
#endif

#endif