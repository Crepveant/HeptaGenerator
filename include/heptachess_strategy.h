#ifndef HEPTACHESS_STRATEGY_H
#define HEPTACHESS_STRATEGY_H

#include "heptachess_game.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Select one move using bounded MaxN search for a non-zero-sum multiplayer game. */
HCMove hc_strategy_select(HCGameState const *state, int search_budget, uint64_t *rng);

#ifdef __cplusplus
}
#endif

#endif
