# Geminina - A Minimal C++ Chess Engine

## To follow the YouTube adventure
*   **https://www.youtube.com/watch?v=gDWa8_NcEJs&list=PLrlFug4Xb94Ep2yBuo7nP-lJVeazvBnVo&ab_channel=ChessTubeTree**

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
*   **Usage**
    *   Geminina is a UCI engine, which means it's designed to be used with a UCI-compatible chess graphical user interface (GUI).
    *   Compile the engine as described above.
    *   Open your preferred chess GUI (e.g., Arena, CuteChess, Tarrasch GUI, BanksiaGUI).
    *   Add Geminina as a new UCI engine in the GUI. You will usually need to browse to the location of the compiled executable.
    *   Once configured, you can play against the engine or have it analyze positions through the GUI.

## Compilation

The engine is designed to be compiled with g++ (GCC).

```bash
g++ -o Geminina main.cpp -std=c++17 -O2

-std=c++17: Specifies the C++17 standard.
-O2: Enables optimizations (optional, but recommended for better performance). You can also use -O3.

An executable named Geminina (or Geminina.exe on Windows) will be created.
