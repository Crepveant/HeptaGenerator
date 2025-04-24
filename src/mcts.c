#include "heptachess_board.h"
#include "heptachess_moves.h"
#include "zobrist.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define MAX_MOVES 512
#define MAX_ROUNDS 512
#define UCT_C 1.414
#define MAX_ROLLOUT_THREADS 8
#define MAX_HISTORY 1024
#define DIRICHLET_EPSILON 0.25
#define DIRICHLET_ALPHA 0.3

typedef struct MCTSNode {
    HCBoard state;
    HCMove move;
    int player;
    int visit_count;
    double win_count;
    struct MCTSNode* parent;
    struct MCTSNode** children;
    double* priors;
    int children_count;
    int is_fully_expanded;
} MCTSNode;

typedef struct {
    HCBoard board;
    int player;
    int result;
    MCTSNode* selected_child;
} RolloutJob;

typedef struct {
    uint64_t history[MAX_HISTORY];
    int count;
} HashHistory;

static int is_looping(HashHistory* h, uint64_t hash) {
    //for (int i = 0; i < h->count; ++i)
        //if (h->history[i] == hash) return 1;
    return 0;
}

static void add_hash(HashHistory* h, uint64_t hash) {
    if (h->count < MAX_HISTORY) {
        h->history[h->count++] = hash;
    }
}

static void free_node(MCTSNode* node) {
    if (!node) return;
    for (int i = 0; i < node->children_count; ++i) {
        free_node(node->children[i]);
    }
    free(node->children);
    free(node->priors);
    free(node);
}

/*static double rand_dirichlet(double alpha) {
    // Gamma(alpha, 1) using Marsaglia and Tsang’s method
    double d = alpha - 1.0 / 3.0;
    double c = 1.0 / sqrt(9.0 * d);
    double x, v;
    while (1) {
        do { x = ((double)rand() / RAND_MAX) * 2 - 1; } while (x == 0);
        double u = rand() / (RAND_MAX + 1.0);
        v = pow(1 + c * x, 3);
        if (v > 0 && log(u) < 0.5 * x * x + d - d * v + d * log(v)) break;
    }
    return d * v;
}*/
static double rand_dirichlet_safe() {
    double u = (double)rand() / (RAND_MAX + 1.0);
    return -log(u + 1e-8); 
}

static MCTSNode* create_node(const HCBoard* state, HCMove move, int player, MCTSNode* parent) {
    MCTSNode* node = malloc(sizeof(MCTSNode));
    node->state = *state;
    node->move = move;
    node->player = player;
    node->visit_count = 0;
    node->win_count = 0.0;
    node->parent = parent;
    node->children = NULL;
    node->priors = NULL;
    node->children_count = 0;
    node->is_fully_expanded = 0;
    return node;
}

static double uct_score(MCTSNode* parent, MCTSNode* child, double prior) {
    if (child->visit_count == 0) return INFINITY;
    double exploitation = child->win_count / child->visit_count;
    double exploration = UCT_C * prior * sqrt(parent->visit_count + 1) / (1 + child->visit_count);
    return exploitation + exploration;
}

static MCTSNode* select_best_child(MCTSNode* node) {
    if (!node->children || node->children_count == 0) return NULL;

    double best_score = -INFINITY;
    MCTSNode* best = NULL;
    int best_count = 0;

    for (int i = 0; i < node->children_count; ++i) {
        double score;
        if (node->children[i]->visit_count == 0) {
            score = 1e6 + ((double)rand() / RAND_MAX) * 1e-3;
        } else {
            score = uct_score(node, node->children[i], node->priors ? node->priors[i] : 1.0);
        }

        if (score > best_score) {
            best_score = score;
            best = node->children[i];
            best_count = 1;
        } else if (fabs(score - best_score) < 1e-8) {
            best_count++;
            if (rand() % best_count == 0) {
                best = node->children[i];
            }
        }
    }

    return best;
}

static void expand_node(MCTSNode* node) {
    HCMove buf[MAX_MOVES];
    MoveList mvlist = { buf, 0, MAX_MOVES };
    generate_legal_moves(node->state.grid, node->state.current_player, &mvlist);

    if (mvlist.count == 0) {
        node->children = NULL;
        node->children_count = 0;
        node->is_fully_expanded = 1;
        return;
    }

    node->children = malloc(sizeof(MCTSNode*) * mvlist.count);
    node->priors = malloc(sizeof(double) * mvlist.count);
    node->children_count = mvlist.count;

    double sum = 0.0;
    for (int i = 0; i < mvlist.count; ++i) {
        double noise = rand_dirichlet_safe();
        node->priors[i] = noise;
        sum += noise;
    }

    for (int i = 0; i < mvlist.count; ++i) {
        node->priors[i] = (1 - DIRICHLET_EPSILON) * (1.0 / mvlist.count) + DIRICHLET_EPSILON * (node->priors[i] / sum);
    }

    for (int i = 0; i < mvlist.count; ++i) {
        HCBoard new_state = node->state;
        hc_apply_move(&new_state, &mvlist.moves[i]);
        node->children[i] = create_node(&new_state, mvlist.moves[i], new_state.current_player, node);
    }

    node->is_fully_expanded = 1;
}

static int rollout(HCBoard b, int orig_player, HashHistory* hist) {
    HCMove buf[MAX_MOVES];
    MoveList mvlist = { buf, 0, MAX_MOVES };

    for (int step = 0; step < MAX_ROUNDS; ++step) {
        int8_t win;
        if (hc_check_terminal(&b, &win)) return win;

        uint64_t hash = zobrist_hash(&b);
        if (is_looping(hist, hash)) return 0;
        add_hash(hist, hash);

        generate_legal_moves(b.grid, b.current_player, &mvlist);
        if (mvlist.count == 0) {
            b.current_player = (b.current_player % 7) + 1;
            continue;
        }

        HCMove mv = mvlist.moves[rand() % mvlist.count];
        hc_apply_move(&b, &mv);
    }

    return 0;
}

void* rollout_worker(void* arg) {
    RolloutJob* job = (RolloutJob*)arg;
    HashHistory hist = {0};
    job->result = rollout(job->board, job->player, &hist);
    return NULL;
}

static void backpropagate(MCTSNode* node, int winner) {
    while (node) {
        node->visit_count++;
        if (winner == node->player) node->win_count += 1.0;
        else if (winner == 0) node->win_count += 0.3;
        node = node->parent;
    }
}

static HCMove mcts_search(const HCBoard* root_state, int sims) {
    MCTSNode* root = create_node(root_state, (HCMove){0}, root_state->current_player, NULL);
    init_zobrist();

    int batch = (sims + MAX_ROLLOUT_THREADS - 1) / MAX_ROLLOUT_THREADS;
    for (int batch_id = 0; batch_id < batch; ++batch_id) {
        int threads = sims - batch_id * MAX_ROLLOUT_THREADS;
        if (threads > MAX_ROLLOUT_THREADS) threads = MAX_ROLLOUT_THREADS;

        pthread_t tids[MAX_ROLLOUT_THREADS];
        RolloutJob jobs[MAX_ROLLOUT_THREADS];

        int valid_jobs = 0;
        for (int t = 0; t < threads; ++t) {
            MCTSNode* node = root;
            MCTSNode* first_child = NULL;

            while (node && node->is_fully_expanded && node->children_count > 0) {
                MCTSNode* next = select_best_child(node);
                if (node == root) first_child = next;
                node = next;
            }

            if (node && !node->is_fully_expanded) {
                expand_node(node);
                if (node->children_count > 0)
                    node = select_best_child(node);
                else continue;
            }

            if (!node) continue;

            jobs[valid_jobs].board = node->state;
            jobs[valid_jobs].player = root_state->current_player;
            jobs[valid_jobs].selected_child = first_child;
            int err = pthread_create(&tids[valid_jobs], NULL, rollout_worker, &jobs[valid_jobs]);
            if (err != 0) {
                fprintf(stderr, "❌ Failed to create thread: %d\n", err);
                continue;
            }

            valid_jobs++;
        }

        for (int t = 0; t < valid_jobs; ++t) {
            pthread_join(tids[t], NULL);
            if (jobs[t].selected_child)
                backpropagate(jobs[t].selected_child, jobs[t].result);
        }
    }

    MCTSNode* best = NULL;
    int best_visits = -1;
    for (int i = 0; i < root->children_count; ++i) {
        if (root->children[i]->visit_count > best_visits) {
            best_visits = root->children[i]->visit_count;
            best = root->children[i];
        }
    }

    HCMove best_move = best ? best->move : (HCMove){0};
    free_node(root);
    return best_move;
}

HCMove hc_mcts_select(const HCBoard* board, int sims_per_move) {
    return mcts_search(board, sims_per_move);
}