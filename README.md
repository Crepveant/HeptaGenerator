# HeptaGenerator

A high-performance **chessboard simulator for HeptachessNet**, designed to generate training data via self-play. Supports full 7-nation rules, UCT-based MCTS AI, multi-threaded simulation, and `.npy` export for large-scale reinforcement learning pipelines.

---

## ⚙️ Build Instructions

Make sure you have `clang` or `gcc` installed with pthread support.

### 🔧 Recommended (with Makefile):

```bash
make
```

This compiles all source files in `src/` and `main.c` into a single executable:

```
./heptagenerator
```

### 🔧 Manual build (if needed):

```bash
clang -O3 -pthread -Iinclude -o heptagenerator \
  main.c \
  src/heptachess_board.c \
  src/heptachess_moves.c \
  src/heptachess_simulate.c \
  src/initial_board.c \
  src/mcts.c \
  src/zobrist.c \
  src/numpy_gen.c

```

---

## 🚀 Usage

```bash
./heptagenerator <output_dir> <num_games> <num_threads>
```

### Example:

```bash
./heptagenerator selfplay_data 1000 6
```

This runs **1000 self-play games** using **6 threads**, and saves training data (states, moves, players, winner) as `.npy` files in `selfplay_data/`.

### Run in background:

```bash
nohup ./heptagenerator selfplay_data 1000 6 > log.txt 2>&1 &
```

### View live progress:

```bash
tail -f log.txt
```

---

## 📁 Output Format

Each game will produce 4 files:
- `game_000001_states.npy`: `[step, 160, 19, 19]` encoded tensor
- `game_000001_moves.npy`: `[step, 5]` move details
- `game_000001_players.npy`: `[step]` player at each step
- `game_000001_winner.npy`: `[1]` final winner ID

---

## 🧠 AI Strategy

- Full tree-based **UCT MCTS** (Upper Confidence Bound for Trees)
- Multi-threaded rollout
- Supports early termination, randomized branching, and rule-accurate elimination

---

## 🛠️ To Do

- [ ] Reuse tree across steps
- [ ] Transposition table w/ Zobrist hashing
- [ ] Evaluation head / NN integration
- [ ] Heuristic-guided rollout