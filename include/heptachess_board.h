#ifndef HEPTACHESS_BOARD_H
#define HEPTACHESS_BOARD_H

#include <stdint.h>
#include "heptachess_moves.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_COUNTRIES 8

/** Encoded piece grid and side to move. */
typedef struct {
	uint8_t grid [SIZE][SIZE];
	uint8_t current_player;
} HCBoard;

/** Initialize the standard starting board layout. */
void hc_board_init(HCBoard *b);

/** Apply a legal move and advance the side to move. */
void hc_apply_move(HCBoard *b, HCMove const *m);

/** Transfer ownership of all remaining pieces from loser to winner. */
void hc_transfer_country(HCBoard *b, uint8_t loser, uint8_t winner);

/** Copy the board into a zero-initialized 160x19x19 one-hot tensor. */
void hc_encode_board(HCBoard const *b, uint8_t out [160][19][19]);

/** Set board features in an already zeroed 160x19x19 one-hot tensor. */
void hc_encode_board_features(HCBoard const *b, uint8_t out [160][19][19]);

/** Return nonzero and write the surviving winner when the board is terminal. */
int  hc_check_terminal(HCBoard const *b, int8_t *out_winner);

#ifdef __cplusplus
}
#endif

#endif
