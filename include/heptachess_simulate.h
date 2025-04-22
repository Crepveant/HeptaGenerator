#ifndef HEPTACHESS_SIMULATE_H
#define HEPTACHESS_SIMULATE_H

#ifdef __cplusplus
extern "C" {
#endif

// Simulate one game and export output to prefix_*.npy
// Returns number of steps played
int simulate_game(const char* prefix, int sims_per_move);

#ifdef __cplusplus
}
#endif

#endif