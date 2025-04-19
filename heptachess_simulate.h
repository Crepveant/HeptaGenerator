// heptachess_simulate.h — header for full self-play game simulation
#ifndef HEPTACHESS_SIMULATE_H
#define HEPTACHESS_SIMULATE_H

#ifdef __cplusplus
extern "C" {
#endif

// Simulate one game and export output to prefix_*.npy
// Returns number of steps played
int simulate_game(const char* prefix);

#ifdef __cplusplus
}
#endif

#endif