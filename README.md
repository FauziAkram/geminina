# Geminina - A Minimal C++ Chess Engine

## Overview

Geminina is a UCI (Universal Chess Interface) compatible chess engine written in C++. It's designed with a focus on clarity, correctness, and simplicity, providing a foundational example of a legal-move-generating chess engine. While not aiming for grandmaster strength in its current iteration, it implements core chess logic, search algorithms, and UCI communication, making it suitable for learning and as a base for further development.

This engine was developed iteratively, with each step adding a meaningful improvement to its playing strength or functionality.

## Features

*   **UCI Compatibility:** Supports standard UCI commands for easy integration with chess GUIs (e.g., Arena, CuteChess, BanksiaGUI).
    *   `uci`
    *   `isready`
    *   `ucinewgame`
    *   `position [startpos | fen <fenstring>] moves <move1> <move2> ...`
    *   `go [wtime <ms> btime <ms> winc <ms> binc <ms> movestogo <n> | movetime <ms>]`
    *   `quit`
*   **Legal Move Generation:** Generates all fully legal moves for the current player, including:
    *   Standard piece movements
    *   Pawn promotions (auto-queens for simplicity in some internal contexts, but respects UCI promotion character)
    *   Castling (Kingside and Queenside)
    *   En passant
*   **Board Representation:** Uses an 8x8 array (`char board[8][8]`) for straightforward board representation.
*   **Search Algorithm:**
    *   Iterative Deepening: Searches to increasing depths.
    *   Alpha-Beta Pruning: Optimizes the search by cutting off unpromising branches.
    *   Quiescence Search: Extends the search for tactical sequences (captures) beyond the main search depth to mitigate the horizon effect.
*   **Evaluation Function:**
    *   Material Count: Basic scoring based on piece values.
    *   Piece-Square Tables (PSTs): Positional bonuses for pieces based on their location, encouraging better development and control.
*   **Game End Detection:** Explicitly checks for and recognizes:
    *   Checkmate
    *   Stalemate
    *   Draw by Threefold Repetition
    *   Draw by Fifty-Move Rule
*   **Time Management:**
    *   Parses UCI time controls (`wtime`, `btime`, `winc`, `binc`, `movestogo`, `movetime`).
    *   Dynamically allocates time per move based on remaining time, increments, and moves to go.
    *   Responsive time checks within the search to adhere to time limits.
*   **Move Ordering:** Implements simple move ordering (captures first) at the root of the search and within the main search to improve alpha-beta pruning efficiency.
*   **Single File Implementation:** All code is contained within `main.cpp` for simplicity.

## Compilation

The engine is designed to be compiled with g++ (GCC).

```bash
g++ -o Geminina main.cpp -std=c++17 -O2

-std=c++17: Specifies the C++17 standard.
-O2: Enables optimizations (optional, but recommended for better performance). You can also use -O3.

An executable named Geminina (or Geminina.exe on Windows) will be created.

**Usage**
Geminina is a UCI engine, which means it's designed to be used with a UCI-compatible chess graphical user interface (GUI).
Compile the engine as described above.
Open your preferred chess GUI (e.g., Arena, CuteChess, Tarrasch GUI, BanksiaGUI).
Add Geminina as a new UCI engine in the GUI. You will usually need to browse to the location of the compiled executable.
Once configured, you can play against the engine or have it analyze positions through the GUI.

**Development Journey**
The engine was built incrementally:
Round 1: Basic board representation, fully legal move generation (including special moves), random move selection, UCI compatibility for core commands, and game-ending condition detection.
Strength Improvement 1: Replaced random move selection with a basic material evaluation (greedy 0-ply search).
Strength Improvement 2: Implemented a 1-ply minimax search, looking one move ahead for the opponent.
Strength Improvement 3: Extended search to 2-ply minimax.
Strength Improvement 4: Introduced Alpha-Beta Pruning to the minimax search for efficiency.
Time Management 1: Added Iterative Deepening with basic time checks based on movetime.
Time Management 2 (Fix): Implemented more responsive node-based time checks within the search and more conservative initial time allocation.
Time Management 3 (Fix): Added parsing for standard UCI time controls (wtime, btime, etc.) for dynamic time allocation and made search exit more immediate on timeout.
Strength Improvement 5: Incorporated Quiescence Search to improve tactical play and reduce horizon effect.
Strength Improvement 6: Added Piece-Square Tables to the evaluation function for basic positional understanding.
Bug Fix (White/Black Imbalance): Corrected inconsistencies in evaluation score handling, particularly for mate scores, to ensure balanced play for both White and Black.

**Future Enhancements (Potential)**
More sophisticated move ordering (e.g., MVV-LVA, Killer Heuristics, History Heuristic)
Transposition Table (Hash Table) for storing previously evaluated positions
More advanced evaluation features (e.g., pawn structure, king safety, mobility, passed pawns, bishop pair)
Null Move Pruning and other advanced search techniques (Late Move Reductions, Futility Pruning)
Opening book
Endgame tablebases (for perfect play in certain endgames)

**License**
This project is open source. You are free to use, modify, and distribute it. (Consider adding a specific license like MIT or GPL if desired).
