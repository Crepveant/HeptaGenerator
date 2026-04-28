#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <direct.h>
#else
  #include <sys/stat.h>
#endif

#include "heptachess_simulate.h"

#define MAX_THREADS 64

typedef struct {
	int  start_index;
	int  game_count;
	int  search_budget;
	char outdir [256];
} ThreadArgs;

static int create_output_dir(char const *outdir) {
#ifdef _WIN32
	int result = _mkdir(outdir);
#else
	int result = mkdir(outdir, 0755);
#endif
	return result == 0 || errno == EEXIST;
}

static void handle_fpe(int sig) {
	( void ) sig;
	fprintf(stderr, "Floating point exception occurred.\n");
	exit(1);
}

static void *thread_worker(void *arg) {
	ThreadArgs *args = ( ThreadArgs * ) arg;
	char        prefix [512];
		for (int i = 0; i < args->game_count; ++i) {
			snprintf(prefix, sizeof(prefix), "%s/game_%06d", args->outdir, args->start_index + i);
			simulate_game(prefix, args->search_budget);
		}
	return NULL;
}

int main(int argc, char **argv) {
	signal(SIGFPE, handle_fpe);

		if (argc < 5) {
			fprintf(stderr, "Usage: %s [outdir] [num_games] [num_threads] [search_budget]\n", argv [0]);
			return 1;
		}

	char const *outdir      = argv [1];
	int         total_games = atoi(argv [2]);
	int         num_threads = atoi(argv [3]);
	int         budget      = atoi(argv [4]);
		if (total_games <= 0 || num_threads <= 0 || budget <= 0) {
			fprintf(stderr, "num_games, num_threads, and search_budget must be positive.\n");
			return 1;
		}
	if (num_threads > MAX_THREADS) num_threads = MAX_THREADS;
		if (!create_output_dir(outdir)) {
			fprintf(stderr, "Failed to create output directory: %s\n", outdir);
			return 1;
		}

	pthread_t  threads [MAX_THREADS];
	ThreadArgs args [MAX_THREADS];
	int        games_per_thread = total_games / num_threads;
	int        remainder        = total_games % num_threads;
	int        idx              = 0;
	int        started_threads  = 0;

		for (int i = 0; i < num_threads; ++i) {
			int count              = games_per_thread + (i < remainder ? 1 : 0);
			args [i].start_index   = idx;
			args [i].game_count    = count;
			args [i].search_budget = budget;
			strncpy(args [i].outdir, outdir, sizeof(args [i].outdir) - 1);
			args [i].outdir [sizeof(args [i].outdir) - 1] = '\0';
			int err = pthread_create(&threads [i], NULL, thread_worker, &args [i]);
				if (err != 0) {
					fprintf(stderr, "Failed to create worker thread: %d\n", err);
					for (int thread = 0; thread < started_threads; ++thread) pthread_join(threads [thread], NULL);
					return 1;
				}
			++started_threads;
			idx += count;
		}

	for (int i = 0; i < started_threads; ++i) pthread_join(threads [i], NULL);

	printf("All %d games finished.\n", total_games);
	return 0;
}
