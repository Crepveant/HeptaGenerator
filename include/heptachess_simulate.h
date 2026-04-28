#ifndef HEPTACHESS_SIMULATE_H
#define HEPTACHESS_SIMULATE_H

#ifdef __cplusplus
extern "C"
{
#endif

/** Simulate one game and export output to prefix_*.npy. */
int simulate_game(char const *prefix, int search_budget);

#ifdef __cplusplus
}
#endif

#endif
