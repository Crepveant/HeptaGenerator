# HeptaGenerator

A high-performance **chessboard simulator for HeptachessNet**, designed to generate training data via self-play. Supports full 7-nation rules, bounded non-zero-sum MaxN search, multi-threaded simulation, and compact `.npy` export.

---

## Build Instructions

Make sure `xmake` is installed and a C99 compiler with pthread support is available.

### Configure

```bash
xmake f -m release
```

On Windows, use a MinGW/llvm-mingw SDK because the simulator uses pthread:

```powershell
xmake f -p mingw --sdk=C:\path\to\llvm-mingw -m release
```

### Build

```bash
xmake
```

### Run Through xmake

```bash
xmake run heptagenerator selfplay_data 1000 6 200
```

The built binary is produced under `build/` according to the active xmake platform and mode.

---

## Usage

```bash
heptagenerator <output_dir> <num_games> <num_threads> <search_budget>
```

### Example:

```bash
heptagenerator selfplay_data 1000 6 200
```

This runs **1000 self-play games** using **6 threads** and a **200-point MaxN search budget**, then saves training data as `.npy` files in `selfplay_data/`.

### Run in background:

```bash
nohup heptagenerator selfplay_data 1000 6 200 > log.txt 2>&1 &
```

### View live progress:

```bash
tail -f log.txt
```

---

## Output Format

Each game produces 4 files:
- `game_000001_boards.npy`: `uint8 [step, 19, 19]` board before each move.
- `game_000001_moves.npy`: `int16 [step, 5]` move as `from_y, from_x, to_y, to_x, is_capture`.
- `game_000001_players.npy`: `int8 [step]` player to move at each recorded board.
- `game_000001_winner.npy`: `int8 [1]` final winner ID.

Board cells use the runtime encoding directly: high 4 bits are player ID, low 4 bits are piece code. A trainer can expand this compact board tensor into one-hot channels when batching.

---

## AI Strategy

- Bounded **MaxN** search for non-zero-sum multiplayer payoff
- Real node budget with early static evaluation when the search budget is exhausted
- Rule-aware search state with live-only country transfer, cumulative losses, eliminations, and capture victory conditions
- Capture ordering plus quiet tactical candidates for marshal escape and key defensive moves
- Material, capture, elimination, hostility, leader pressure, and marshal safety evaluation
- Multi-threaded self-play across games
