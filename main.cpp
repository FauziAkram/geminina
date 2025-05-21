#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <chrono> // Required for time management
#include <random>
#include <map>
#include <limits> // Required for std::numeric_limits
#include <cctype> // For toupper
#include <atomic> // For atomic flag/counter

// Piece character constants
const char EMPTY = ' ';
const char W_PAWN = 'P', W_KNIGHT = 'N', W_BISHOP = 'B', W_ROOK = 'R', W_QUEEN = 'Q', W_KING = 'K';
const char B_PAWN = 'p', B_KNIGHT = 'n', B_BISHOP = 'b', B_ROOK = 'r', B_QUEEN = 'q', B_KING = 'k';

// Piece values for material evaluation (in centipawns)
const std::map<char, int> piece_values = {
    {W_PAWN, 100}, {B_PAWN, 100},
    {W_KNIGHT, 320}, {B_KNIGHT, 320},
    {W_BISHOP, 330}, {B_BISHOP, 330},
    {W_ROOK, 500}, {B_ROOK, 500},
    {W_QUEEN, 900}, {B_QUEEN, 900},
    {W_KING, 20000}, {B_KING, 20000} 
};
// Simplified piece values for MVV-LVA (less granularity needed)
const std::map<char, int> mvv_lva_piece_values = {
    {W_PAWN, 1}, {B_PAWN, 1},
    {W_KNIGHT, 3}, {B_KNIGHT, 3},
    {W_BISHOP, 3}, {B_BISHOP, 3},
    {W_ROOK, 5}, {B_ROOK, 5},
    {W_QUEEN, 9}, {B_QUEEN, 9},
    {W_KING, 10}, {B_KING, 10} // King captures are rare but high value victim
};


// --- Piece-Square Tables (PSTs) ---
const int pawn_pst[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};
const int knight_pst[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};
const int bishop_pst[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};
const int rook_pst[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
};
const int queen_pst[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};
const int king_pst_mg[64] = { 
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};
const int king_pst_eg[64] = { 
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};


// Evaluation scores for terminal states (absolute value)
const int MATE_SCORE = 100000; 
const int DRAW_SCORE = 0;     
const int MAX_SEARCH_PLY = 64; // Max depth for iterative deepening (can be high)
const int MAX_QUIESCENCE_PLY = 6; 
const int IN_CHECK_PENALTY = 50; 

// --- Forward Declarations ---
struct BoardState;
struct Move;
void generateLegalMoves(const BoardState& state, std::vector<Move>& legal_moves, bool capturesOnly = false);
bool isKingInCheck(const BoardState& state, bool kingIsWhite);
bool isSquareAttacked(const BoardState& state, int r, int c, bool byWhiteAttacker);
void apply_raw_move_to_board(BoardState& state, const Move& move);
void master_apply_move(const Move& move); 
int evaluateBoard(const BoardState& state); 
int alphaBetaSearch(BoardState state, int depth, int alpha, int beta, bool maximizingPlayer, 
                    const std::chrono::steady_clock::time_point& startTime, 
                    const std::chrono::milliseconds& timeLimit); 
int quiescenceSearch(BoardState state, int alpha, int beta, bool maximizingPlayer,
                     const std::chrono::steady_clock::time_point& startTime,
                     const std::chrono::milliseconds& timeLimit, int quiescenceDepth);
char getPieceAt(const BoardState& state, int r, int c); 
void orderMoves(const BoardState& state, std::vector<Move>& moves);


// Global flag to signal time out (checked within search)
std::atomic<bool> time_is_up = false; 
// Global node counter for time checks
std::atomic<uint64_t> nodes_searched = 0; 


// --- Move Structure --- 
struct Move {
    int fromRow, fromCol;
    int toRow, toCol;
    char promotionPiece; 
    bool isKingSideCastle;
    bool isQueenSideCastle;
    bool isEnPassantCapture;
    int score; // For move ordering
    
    Move(int fr=0, int fc=0, int tr=0, int tc=0, char promo=EMPTY, bool ksc=false, bool qsc=false, bool ep=false)
        : fromRow(fr), fromCol(fc), toRow(tr), toCol(tc), promotionPiece(promo),
          isKingSideCastle(ksc), isQueenSideCastle(qsc), isEnPassantCapture(ep), score(0) {}
          
    std::string toUci() const {
        std::string uci = "";
        uci += (char)('a' + fromCol); uci += (char)('8' - fromRow);
        uci += (char)('a' + toCol); uci += (char)('8' - toRow);
        if (promotionPiece != EMPTY) uci += tolower(promotionPiece);
        return uci;
    }
    
    bool operator==(const Move& other) const {
        return fromRow==other.fromRow && fromCol==other.fromCol && toRow==other.toRow && 
               toCol==other.toCol && promotionPiece==other.promotionPiece && 
               isKingSideCastle==other.isKingSideCastle && isQueenSideCastle==other.isQueenSideCastle &&
               isEnPassantCapture==other.isEnPassantCapture;
    }
    
    bool isCapture(const BoardState& state) const; 
};

// --- Board State Structure --- 
struct BoardState {
    char board[8][8];
    bool whiteToMove;
    bool whiteKingSideCastle, whiteQueenSideCastle;
    bool blackKingSideCastle, blackQueenSideCastle;
    std::pair<int, int> enPassantTarget; 
    int halfmoveClock;
    int fullmoveNumber;
    std::map<std::string, int> positionCounts; 
    BoardState() { reset(); }
    void reset() {
        const char initial_board[8][8] = {
            {'r','n','b','q','k','b','n','r'}, {'p','p','p','p','p','p','p','p'},
            {EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY},{EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY},
            {EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY},{EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY,EMPTY},
            {'P','P','P','P','P','P','P','P'}, {'R','N','B','Q','K','B','N','R'}
        };
         for(int r=0; r<8; ++r) for(int c=0; c<8; ++c) board[r][c] = initial_board[r][c];
        whiteToMove = true;
        whiteKingSideCastle = whiteQueenSideCastle = true;
        blackKingSideCastle = blackQueenSideCastle = true;
        enPassantTarget = {-1,-1};
        halfmoveClock = 0; fullmoveNumber = 1;
        positionCounts.clear(); addCurrentPositionToHistory();
    }
    std::string getPositionKey() const {
        std::stringstream ss;
        for(int r=0; r<8; ++r) for(int c=0; c<8; ++c) ss << board[r][c];
        ss << (whiteToMove ? 'w' : 'b');
        ss << (whiteKingSideCastle ? 'K' : '-'); ss << (whiteQueenSideCastle ? 'Q' : '-');
        ss << (blackKingSideCastle ? 'k' : '-'); ss << (blackQueenSideCastle ? 'q' : '-');
        if (enPassantTarget.first != -1) ss << (char)('a'+enPassantTarget.second) << (char)('8'-enPassantTarget.first);
        else ss << '-';
        return ss.str();
    }
    void addCurrentPositionToHistory() { positionCounts[getPositionKey()]++; }
    void parseFen(const std::string& fenStr) {
        std::fill(&board[0][0], &board[0][0]+sizeof(board), EMPTY);
        positionCounts.clear(); 
        std::istringstream fenStream(fenStr); std::string part;
        fenStream >> part; int r=0, c=0;
        for(char sym : part) {
            if(sym=='/') { r++; c=0; } else if(isdigit(sym)) { c+=(sym-'0'); } 
            else { if(r<8 && c<8) board[r][c++] = sym; }
        }
        fenStream >> part; whiteToMove = (part=="w");
        fenStream >> part;
        whiteKingSideCastle = (part.find('K') != std::string::npos); whiteQueenSideCastle = (part.find('Q') != std::string::npos);
        blackKingSideCastle = (part.find('k') != std::string::npos); blackQueenSideCastle = (part.find('q') != std::string::npos);
        fenStream >> part;
        if(part=="-") enPassantTarget={-1,-1}; else { enPassantTarget = {'8'-part[1], part[0]-'a'}; }
        if(fenStream >> part) halfmoveClock=std::stoi(part); else halfmoveClock=0;
        if(fenStream >> part) fullmoveNumber=std::stoi(part); else fullmoveNumber=1;
        addCurrentPositionToHistory();
    }
};

BoardState currentBoard; 
std::mt19937 global_rng; 

// --- Helper Functions --- 
bool isSquareOnBoard(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }
char getPieceAt(const BoardState& state, int r, int c) { return isSquareOnBoard(r, c) ? state.board[r][c] : EMPTY; } 
bool isWhitePiece(char piece) { return piece >= 'A' && piece <= 'Z'; }
bool isBlackPiece(char piece) { return piece >= 'a' && piece <= 'z'; }

// Definition of Move::isCapture 
bool Move::isCapture(const BoardState& state) const {
    return isEnPassantCapture || (getPieceAt(state, toRow, toCol) != EMPTY);
}


// --- Evaluation Function with PSTs --- 
int evaluateBoard(const BoardState& state) {
    int score = 0;
    int total_material_no_kings = 0; 

    for(int r=0; r<8; ++r) {
        for(int c=0; c<8; ++c) {
            char piece = state.board[r][c]; 
            if(piece!=EMPTY) {
                auto it = piece_values.find(piece);
                int piece_val = 0;
                if(it != piece_values.end()) piece_val = it->second;
                
                if (toupper(piece) != W_KING) { 
                    auto mat_it = piece_values.find(toupper(piece)); 
                    if (mat_it != piece_values.end()) total_material_no_kings += mat_it->second; 
                }

                int pst_bonus = 0;
                int square_index = r * 8 + c;
                int black_square_index = (7 - r) * 8 + c; 

                if (isWhitePiece(piece)) {
                    score += piece_val;
                    if (piece == W_PAWN) pst_bonus = pawn_pst[square_index];
                    else if (piece == W_KNIGHT) pst_bonus = knight_pst[square_index];
                    else if (piece == W_BISHOP) pst_bonus = bishop_pst[square_index];
                    else if (piece == W_ROOK) pst_bonus = rook_pst[square_index];
                    else if (piece == W_QUEEN) pst_bonus = queen_pst[square_index];
                    else if (piece == W_KING) {
                        if (total_material_no_kings < 1500) pst_bonus = king_pst_eg[square_index]; 
                        else pst_bonus = king_pst_mg[square_index];
                    }
                    score += pst_bonus;
                } else { 
                    score -= piece_val;
                    if (piece == B_PAWN) pst_bonus = pawn_pst[black_square_index];
                    else if (piece == B_KNIGHT) pst_bonus = knight_pst[black_square_index];
                    else if (piece == B_BISHOP) pst_bonus = bishop_pst[black_square_index];
                    else if (piece == B_ROOK) pst_bonus = rook_pst[black_square_index];
                    else if (piece == B_QUEEN) pst_bonus = queen_pst[black_square_index];
                    else if (piece == B_KING) {
                        if (total_material_no_kings < 1500) pst_bonus = king_pst_eg[black_square_index];
                        else pst_bonus = king_pst_mg[black_square_index];
                    }
                    score -= pst_bonus; 
                }
            }
        }
    }
    return score; 
}

// --- Move Generation --- 
void addMove(const BoardState& s, int r1, int c1, int r2, int c2, std::vector<Move>& m, char promo=EMPTY, bool ksc=false, bool qsc=false, bool ep=false) {
    if(!isSquareOnBoard(r1,c1) || !isSquareOnBoard(r2,c2)) return;
    char piece=getPieceAt(s,r1,c1), target=getPieceAt(s,r2,c2);
    if(piece==EMPTY || (s.whiteToMove && isWhitePiece(target)) || (!s.whiteToMove && isBlackPiece(target))) return;
    m.emplace_back(r1,c1,r2,c2,promo,ksc,qsc,ep);
}
void generatePawnMoves(const BoardState& state, int r, int c, std::vector<Move>& moves, bool capturesOnly) {
    char piece = state.board[r][c];
    int direction = (isWhitePiece(piece)) ? -1 : 1; 
    char promotionPieces[] = {state.whiteToMove ? W_QUEEN : B_QUEEN, state.whiteToMove ? W_ROOK : B_ROOK, 
                              state.whiteToMove ? W_BISHOP : B_BISHOP, state.whiteToMove ? W_KNIGHT : B_KNIGHT};
    int promotion_rank = state.whiteToMove ? 0 : 7;
    int start_rank = state.whiteToMove ? 6 : 1;
    if (!capturesOnly && isSquareOnBoard(r + direction, c) && state.board[r + direction][c] == EMPTY) {
        if (r + direction == promotion_rank) { for (char promo : promotionPieces) addMove(state, r, c, r + direction, c, moves, promo); } 
        else { addMove(state, r, c, r + direction, c, moves); }
        if (r == start_rank && isSquareOnBoard(r + 2 * direction, c) && state.board[r + 2 * direction][c] == EMPTY) {
            addMove(state, r, c, r + 2 * direction, c, moves);
        }
    }
    for (int dc : {-1, 1}) { 
        int capture_r = r + direction; int capture_c = c + dc;
        if (isSquareOnBoard(capture_r, capture_c)) {
            char targetPiece = state.board[capture_r][capture_c];
            bool canCapture = (state.whiteToMove && isBlackPiece(targetPiece)) || (!state.whiteToMove && isWhitePiece(targetPiece));
            if (canCapture) {
                if (capture_r == promotion_rank) { for (char promo : promotionPieces) addMove(state, r, c, capture_r, capture_c, moves, promo); } 
                else { addMove(state, r, c, capture_r, capture_c, moves); }
            }
            if (capture_r == state.enPassantTarget.first && capture_c == state.enPassantTarget.second && targetPiece == EMPTY) {
                 addMove(state, r, c, capture_r, capture_c, moves, EMPTY, false, false, true); 
            }
        }
    }
}
void generateSlidingMoves(const BoardState& state, int r, int c, std::vector<Move>& moves, const std::vector<std::pair<int, int>>& directions, bool capturesOnly) {
    char piece = state.board[r][c];
    for (auto dir : directions) {
        for (int i = 1; i < 8; ++i) {
            int next_r = r + dir.first * i; int next_c = c + dir.second * i;
            if (!isSquareOnBoard(next_r, next_c)) break; 
            char targetPiece = state.board[next_r][next_c];
            if (targetPiece == EMPTY) { if (!capturesOnly) addMove(state, r, c, next_r, next_c, moves); } 
            else {
                if ((isWhitePiece(piece) && isBlackPiece(targetPiece)) || (isBlackPiece(piece) && isWhitePiece(targetPiece))) {
                    addMove(state, r, c, next_r, next_c, moves); 
                }
                break; 
            }
        }
    }
}
void generateKnightMoves(const BoardState& state, int r, int c, std::vector<Move>& moves, bool capturesOnly) {
    const std::vector<std::pair<int, int>> knight_deltas = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    for (auto d : knight_deltas) { 
        if (capturesOnly) { if(isSquareOnBoard(r+d.first, c+d.second) && getPieceAt(state, r+d.first, c+d.second) != EMPTY) addMove(state, r, c, r + d.first, c + d.second, moves); } 
        else { addMove(state, r, c, r + d.first, c + d.second, moves); }
    }
}
void generateKingMoves(const BoardState& state, int r, int c, std::vector<Move>& moves, bool capturesOnly) {
    const std::vector<std::pair<int, int>> king_deltas = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
    for (auto d : king_deltas) { 
        if (capturesOnly) { if(isSquareOnBoard(r+d.first, c+d.second) && getPieceAt(state, r+d.first, c+d.second) != EMPTY) addMove(state, r, c, r + d.first, c + d.second, moves); } 
        else { addMove(state, r, c, r + d.first, c + d.second, moves); }
    }
    if (!capturesOnly) { 
        if (state.whiteToMove) {
            if (state.whiteKingSideCastle && state.board[7][5]==EMPTY && state.board[7][6]==EMPTY &&
                !isSquareAttacked(state, 7, 4, false) && !isSquareAttacked(state, 7, 5, false) && !isSquareAttacked(state, 7, 6, false)) {
                addMove(state, 7, 4, 7, 6, moves, EMPTY, true, false, false); 
            }
            if (state.whiteQueenSideCastle && state.board[7][1]==EMPTY && state.board[7][2]==EMPTY && state.board[7][3]==EMPTY &&
                !isSquareAttacked(state, 7, 4, false) && !isSquareAttacked(state, 7, 3, false) && !isSquareAttacked(state, 7, 2, false)) {
                addMove(state, 7, 4, 7, 2, moves, EMPTY, false, true, false); 
            }
        } else { 
            if (state.blackKingSideCastle && state.board[0][5]==EMPTY && state.board[0][6]==EMPTY &&
                !isSquareAttacked(state, 0, 4, true) && !isSquareAttacked(state, 0, 5, true) && !isSquareAttacked(state, 0, 6, true)) {
                addMove(state, 0, 4, 0, 6, moves, EMPTY, true, false, false); 
            }
            if (state.blackQueenSideCastle && state.board[0][1]==EMPTY && state.board[0][2]==EMPTY && state.board[0][3]==EMPTY &&
                !isSquareAttacked(state, 0, 4, true) && !isSquareAttacked(state, 0, 3, true) && !isSquareAttacked(state, 0, 2, true)) {
                addMove(state, 0, 4, 0, 2, moves, EMPTY, false, true, false); 
            }
        }
    }
}
void generateAllPseudoLegalMoves(const BoardState& state, std::vector<Move>& moves, bool capturesOnly) {
    moves.clear();
    const std::vector<std::pair<int, int>> R_DIRS = {{0,1},{0,-1},{1,0},{-1,0}};
    const std::vector<std::pair<int, int>> B_DIRS = {{1,1},{1,-1},{-1,1},{-1,-1}};
    std::vector<std::pair<int, int>> Q_DIRS = R_DIRS;
    Q_DIRS.insert(Q_DIRS.end(), B_DIRS.begin(), B_DIRS.end());
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            char piece = state.board[r][c];
            if (piece == EMPTY || (state.whiteToMove != isWhitePiece(piece))) continue; 
            char upper_piece = toupper(piece);
            if (upper_piece == W_PAWN) generatePawnMoves(state, r, c, moves, capturesOnly);
            else if (upper_piece == W_KNIGHT) generateKnightMoves(state, r, c, moves, capturesOnly);
            else if (upper_piece == W_BISHOP) generateSlidingMoves(state, r, c, moves, B_DIRS, capturesOnly);
            else if (upper_piece == W_ROOK) generateSlidingMoves(state, r, c, moves, R_DIRS, capturesOnly);
            else if (upper_piece == W_QUEEN) generateSlidingMoves(state, r, c, moves, Q_DIRS, capturesOnly);
            else if (upper_piece == W_KING) generateKingMoves(state, r, c, moves, capturesOnly);
        }
    }
}

// Applies move to board state 
void apply_raw_move_to_board(BoardState& state, const Move& move) {
    char piece = state.board[move.fromRow][move.fromCol];
    char captured = state.board[move.toRow][move.toCol]; 
    int ep_cap_row = state.whiteToMove ? move.toRow + 1 : move.toRow - 1; 
    state.board[move.toRow][move.toCol] = piece;
    state.board[move.fromRow][move.fromCol] = EMPTY;
    if (move.promotionPiece != EMPTY) { state.board[move.toRow][move.toCol] = move.promotionPiece; } 
    else if (move.isKingSideCastle) { state.board[move.fromRow][5] = state.board[move.fromRow][7]; state.board[move.fromRow][7] = EMPTY; } 
    else if (move.isQueenSideCastle) { state.board[move.fromRow][3] = state.board[move.fromRow][0]; state.board[move.fromRow][0] = EMPTY; } 
    else if (move.isEnPassantCapture) { state.board[ep_cap_row][move.toCol] = EMPTY; }
    state.enPassantTarget = {-1, -1}; 
    if (toupper(piece) == W_PAWN && abs(move.toRow - move.fromRow) == 2) { state.enPassantTarget = {(move.fromRow + move.toRow) / 2, move.fromCol}; }
    if (piece == W_KING) state.whiteKingSideCastle = state.whiteQueenSideCastle = false;
    else if (piece == B_KING) state.blackKingSideCastle = state.blackQueenSideCastle = false;
    else if (piece == W_ROOK) { if (move.fromRow == 7 && move.fromCol == 0) state.whiteQueenSideCastle = false; else if (move.fromRow == 7 && move.fromCol == 7) state.whiteKingSideCastle = false; } 
    else if (piece == B_ROOK) { if (move.fromRow == 0 && move.fromCol == 0) state.blackQueenSideCastle = false; else if (move.fromRow == 0 && move.fromCol == 7) state.blackKingSideCastle = false; }
    if (captured == W_ROOK) { if (move.toRow == 7 && move.toCol == 0) state.whiteQueenSideCastle = false; else if (move.toRow == 7 && move.toCol == 7) state.whiteKingSideCastle = false; } 
    else if (captured == B_ROOK) { if (move.toRow == 0 && move.toCol == 0) state.blackQueenSideCastle = false; else if (move.toRow == 0 && move.toCol == 7) state.blackKingSideCastle = false; }
    state.whiteToMove = !state.whiteToMove;
}

// --- Check Detection --- 
bool isSquareAttacked(const BoardState& state, int r, int c, bool byWhiteAttacker) {
    int pawn_dir = byWhiteAttacker ? 1 : -1; 
    char attacking_pawn = byWhiteAttacker ? W_PAWN : B_PAWN;
    if (getPieceAt(state, r + pawn_dir, c - 1) == attacking_pawn) return true;
    if (getPieceAt(state, r + pawn_dir, c + 1) == attacking_pawn) return true;
    char attacking_knight = byWhiteAttacker ? W_KNIGHT : B_KNIGHT;
    const std::vector<std::pair<int, int>> knight_deltas = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    for (auto d : knight_deltas) { if (getPieceAt(state, r + d.first, c + d.second) == attacking_knight) return true; }
    char attacking_rook = byWhiteAttacker ? W_ROOK : B_ROOK;
    char attacking_bishop = byWhiteAttacker ? W_BISHOP : B_BISHOP;
    char attacking_queen = byWhiteAttacker ? W_QUEEN : B_QUEEN;
    const std::vector<std::pair<int, int>> rook_dirs = {{0,1},{0,-1},{1,0},{-1,0}};
    const std::vector<std::pair<int, int>> bishop_dirs = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (auto dir : rook_dirs) {
        for (int i = 1; i < 8; ++i) {
            int nr=r+dir.first*i, nc=c+dir.second*i; if (!isSquareOnBoard(nr, nc)) break;
            char p=state.board[nr][nc]; if (p==attacking_rook || p==attacking_queen) return true; if (p!=EMPTY) break;
        }
    }
    for (auto dir : bishop_dirs) {
        for (int i = 1; i < 8; ++i) {
            int nr=r+dir.first*i, nc=c+dir.second*i; if (!isSquareOnBoard(nr, nc)) break;
            char p=state.board[nr][nc]; if (p==attacking_bishop || p==attacking_queen) return true; if (p!=EMPTY) break;
        }
    }
    char attacking_king = byWhiteAttacker ? W_KING : B_KING;
     const std::vector<std::pair<int, int>> king_deltas = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
    for (auto d : king_deltas) { if (getPieceAt(state, r + d.first, c + d.second) == attacking_king) return true; }
    return false; 
}
bool isKingInCheck(const BoardState& state, bool kingIsWhite) {
    int kr = -1, kc = -1; char k_char = kingIsWhite ? W_KING : B_KING;
    for (int r = 0; r < 8 && kr == -1; ++r) { for (int c = 0; c < 8; ++c) { if (state.board[r][c] == k_char) { kr = r; kc = c; break; } } }
    return (kr != -1) && isSquareAttacked(state, kr, kc, !kingIsWhite); 
}

// Modified to optionally generate only captures
void generateLegalMoves(const BoardState& S, std::vector<Move>& legal_moves, bool capturesOnly) {
    legal_moves.clear();
    std::vector<Move> pseudo; generateAllPseudoLegalMoves(S, pseudo, capturesOnly); 
    bool isWhite = S.whiteToMove;
    for (const auto& m : pseudo) {
        BoardState temp = S; apply_raw_move_to_board(temp, m);
        if (!isKingInCheck(temp, isWhite)) legal_moves.push_back(m);
    }
}


// --- Quiescence Search ---
int quiescenceSearch(BoardState state, int alpha, int beta, bool maximizingPlayer,
                     const std::chrono::steady_clock::time_point& startTime,
                     const std::chrono::milliseconds& timeLimit, int quiescenceDepth) {
    if (time_is_up.load(std::memory_order_relaxed)) return 0;
    nodes_searched++;

    const uint64_t CHECK_TIME_MASK = 1023; 
    if ((nodes_searched.load(std::memory_order_relaxed) & CHECK_TIME_MASK) == 0) {
        if (std::chrono::steady_clock::now() - startTime >= timeLimit) {
            time_is_up.store(true, std::memory_order_relaxed); 
            return 0; 
        }
    }
    if (quiescenceDepth <= 0) return evaluateBoard(state); 

    int stand_pat = evaluateBoard(state); 
    bool in_check = isKingInCheck(state, state.whiteToMove);

    // Apply penalty if stand-pat is in check
    if (in_check) { 
        if (maximizingPlayer) stand_pat -= IN_CHECK_PENALTY; 
        else stand_pat += IN_CHECK_PENALTY;
    }

    if (maximizingPlayer) {
        // Stand-pat cutoff only if NOT in check. If in check, we must make a move.
        if (stand_pat >= beta && !in_check) return beta; 
        alpha = std::max(alpha, stand_pat);
    } else {
        if (stand_pat <= alpha && !in_check) return alpha; 
        beta = std::min(beta, stand_pat);
    }

    std::vector<Move> q_moves;
    // If in check, generate all legal moves to find an escape.
    // Otherwise, only generate captures for quiescence.
    generateLegalMoves(state, q_moves, !in_check); 
    orderMoves(state, q_moves); 

    // If in check and no legal moves, it's checkmate.
    if (in_check && q_moves.empty()) {
        return maximizingPlayer ? (-MATE_SCORE - MAX_SEARCH_PLY - quiescenceDepth) : (MATE_SCORE + MAX_SEARCH_PLY + quiescenceDepth);
    }
    // If not in check and no captures, return stand_pat.
    if (!in_check && q_moves.empty()) {
        return stand_pat; 
    }
    
    if (maximizingPlayer) {
        for (const auto& move : q_moves) {
            BoardState nextState = state;
            apply_raw_move_to_board(nextState, move);
            int score = quiescenceSearch(nextState, alpha, beta, false, startTime, timeLimit, quiescenceDepth - 1);
            if (time_is_up.load(std::memory_order_relaxed)) return 0;
            alpha = std::max(alpha, score);
            if (alpha >= beta) break; 
        }
        return alpha;
    } else {
        for (const auto& move : q_moves) {
            BoardState nextState = state;
            apply_raw_move_to_board(nextState, move);
            int score = quiescenceSearch(nextState, alpha, beta, true, startTime, timeLimit, quiescenceDepth - 1);
            if (time_is_up.load(std::memory_order_relaxed)) return 0;
            beta = std::min(beta, score);
            if (alpha >= beta) break; 
        }
        return beta;
    }
}

// --- MVV-LVA Move Ordering ---
void orderMoves(const BoardState& state, std::vector<Move>& moves) {
    for (auto& move : moves) {
        move.score = 0; 
        if (move.isCapture(state)) {
            char movingPieceType = toupper(state.board[move.fromRow][move.fromCol]);
            char capturedPieceType;
            if (move.isEnPassantCapture) {
                capturedPieceType = W_PAWN; 
            } else {
                capturedPieceType = toupper(state.board[move.toRow][move.toCol]);
            }

            int victimValue = 0;
            auto victim_it = mvv_lva_piece_values.find(capturedPieceType);
            if(victim_it != mvv_lva_piece_values.end()) victimValue = victim_it->second;

            int attackerValue = 10; 
            auto attacker_it = mvv_lva_piece_values.find(movingPieceType);
            if(attacker_it != mvv_lva_piece_values.end()) attackerValue = attacker_it->second;
            
            move.score = (victimValue * 100) - attackerValue; 
        }
        if (move.promotionPiece != EMPTY) {
            auto promo_it = mvv_lva_piece_values.find(toupper(move.promotionPiece));
            if (promo_it != mvv_lva_piece_values.end()){
                 move.score += promo_it->second * 100; // Promotions are very valuable
            } else {
                 move.score += mvv_lva_piece_values.at(W_QUEEN) * 100; // Default to Queen value if not found
            }
        }
    }
    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
        return a.score > b.score;
    });
}


// --- Alpha-Beta Search with Quiescence & Move Ordering ---
int alphaBetaSearch(BoardState state, int depth, int alpha, int beta, bool maximizingPlayer, 
                    const std::chrono::steady_clock::time_point& startTime, 
                    const std::chrono::milliseconds& timeLimit) 
{
    if (time_is_up.load(std::memory_order_relaxed)) return 0; 
    nodes_searched++; 

    std::vector<Move> legalMoves;
    generateLegalMoves(state, legalMoves, false); 

    if (legalMoves.empty()) {
        if (isKingInCheck(state, state.whiteToMove)) return maximizingPlayer ? (-MATE_SCORE - depth) : (MATE_SCORE + depth); 
        else return DRAW_SCORE; 
    }
    if (state.positionCounts[state.getPositionKey()] >= 3 || state.halfmoveClock >= 100) return DRAW_SCORE; 
    
    if (depth == 0) {
        return quiescenceSearch(state, alpha, beta, maximizingPlayer, startTime, timeLimit, MAX_QUIESCENCE_PLY);
    }

    const uint64_t CHECK_TIME_MASK = 1023; 
    if ((nodes_searched.load(std::memory_order_relaxed) & CHECK_TIME_MASK) == 0) {
        if (std::chrono::steady_clock::now() - startTime >= timeLimit) {
            time_is_up.store(true, std::memory_order_relaxed); 
            return 0; 
        }
    }
    
    orderMoves(state, legalMoves); 


    if (maximizingPlayer) {
        int maxEval = std::numeric_limits<int>::min();
        for (const auto& move : legalMoves) { 
            BoardState nextState = state; apply_raw_move_to_board(nextState, move); 
            int eval = alphaBetaSearch(nextState, depth - 1, alpha, beta, false, startTime, timeLimit); 
            if (time_is_up.load(std::memory_order_relaxed)) return 0; 
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval); 
            if (beta <= alpha) break; 
        }
        return maxEval;
    } else { 
        int minEval = std::numeric_limits<int>::max();
        for (const auto& move : legalMoves) { 
            BoardState nextState = state; apply_raw_move_to_board(nextState, move); 
            int eval = alphaBetaSearch(nextState, depth - 1, alpha, beta, true, startTime, timeLimit); 
            if (time_is_up.load(std::memory_order_relaxed)) return 0;
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval); 
            if (beta <= alpha) break; 
        }
        return minEval;
    }
}

// --- Game Logic --- 
void master_apply_move(const Move& move) {
    char piece = currentBoard.board[move.fromRow][move.fromCol];
    char captured = currentBoard.board[move.toRow][move.toCol]; 
    bool isPawn = (toupper(piece) == W_PAWN);
    bool isCap = (captured != EMPTY) || move.isEnPassantCapture;
    apply_raw_move_to_board(currentBoard, move); 
    if (isPawn || isCap) { currentBoard.halfmoveClock = 0; } else { currentBoard.halfmoveClock++; }
    if (!currentBoard.whiteToMove) { currentBoard.fullmoveNumber++; }
    currentBoard.addCurrentPositionToHistory(); 
}
bool isCheckmate() { std::vector<Move> m; generateLegalMoves(currentBoard, m, false); return m.empty() && isKingInCheck(currentBoard, currentBoard.whiteToMove); }
bool isStalemate() { std::vector<Move> m; generateLegalMoves(currentBoard, m, false); return m.empty() && !isKingInCheck(currentBoard, currentBoard.whiteToMove); }
bool isThreefoldRepetition() { return currentBoard.positionCounts[currentBoard.getPositionKey()] >= 3; }
bool isFiftyMoveDraw() { return currentBoard.halfmoveClock >= 100; }
std::string checkGameEndStatus() {
    if (isCheckmate()) return currentBoard.whiteToMove ? "0-1 {Black mates}" : "1-0 {White mates}";
    if (isStalemate()) return "1/2-1/2 {Stalemate}";
    if (isThreefoldRepetition()) return "1/2-1/2 {Draw by threefold repetition}";
    if (isFiftyMoveDraw()) return "1/2-1/2 {Draw by fifty-move rule}";
    return ""; 
}

// --- UCI Handling --- 
void handleUci() { std::cout << "id name Geminina\nid author LLM Developer\nuciok" << std::endl; } 
void handleIsReady() { std::cout << "readyok" << std::endl; }
void handleUciNewGame() { currentBoard.reset(); }
void handlePosition(std::istringstream& iss) {
    std::string token, fen_str; iss >> token; 
    if (token == "startpos") { currentBoard.reset(); iss >> token; } 
    else if (token == "fen") {
        while(iss >> token && token != "moves") { fen_str += token + " "; }
        if (!fen_str.empty()) fen_str.pop_back(); 
        currentBoard.parseFen(fen_str);
    } 
    if (token == "moves") { 
        while (iss >> token) { 
            Move pMove; if (token.length() < 4) continue; 
            pMove.fromCol = token[0] - 'a'; pMove.fromRow = '8' - token[1];
            pMove.toCol = token[2] - 'a'; pMove.toRow = '8' - token[3];
            pMove.promotionPiece = EMPTY;
            if (token.length() == 5) { 
                char promo = token[4]; char pieceColor = currentBoard.whiteToMove ? 'W' : 'B';
                if (promo == 'q') pMove.promotionPiece = (pieceColor == 'W' ? W_QUEEN : B_QUEEN);
                else if (promo == 'r') pMove.promotionPiece = (pieceColor == 'W' ? W_ROOK : B_ROOK);
                else if (promo == 'b') pMove.promotionPiece = (pieceColor == 'W' ? W_BISHOP : B_BISHOP);
                else if (promo == 'n') pMove.promotionPiece = (pieceColor == 'W' ? W_KNIGHT : B_KNIGHT);
            }
            std::vector<Move> legal_moves; generateLegalMoves(currentBoard, legal_moves, false);
            Move moveToApply; bool found = false;
            for (const auto& legal_m : legal_moves) {
                if (legal_m.fromRow == pMove.fromRow && legal_m.fromCol == pMove.fromCol &&
                    legal_m.toRow == pMove.toRow && legal_m.toCol == pMove.toCol &&
                    legal_m.promotionPiece == pMove.promotionPiece ) {
                    moveToApply = legal_m; found = true; break;
                }
            }
            if (found) { master_apply_move(moveToApply); } else { break; }
        }
    }
}

// --- Main Search Control (handleGo) with Dynamic Time Allocation ---
void handleGo(std::istringstream& iss) {
    std::string token; 
    long long wtime_ms = -1, btime_ms = -1, winc_ms = 0, binc_ms = 0;
    int movestogo = 0; 
    long long movetime_ms = -1; 

    while(iss >> token) { 
        if (token == "wtime") iss >> wtime_ms;
        else if (token == "btime") iss >> btime_ms;
        else if (token == "winc") iss >> winc_ms;
        else if (token == "binc") iss >> binc_ms;
        else if (token == "movestogo") iss >> movestogo;
        else if (token == "movetime") iss >> movetime_ms;
    }
    
    long long allocated_ms;
    long long time_buffer_ms = 100; 

    if (movetime_ms != -1) {
        allocated_ms = std::max(10LL, movetime_ms - time_buffer_ms);
    } else {
        long long my_time = currentBoard.whiteToMove ? wtime_ms : btime_ms;
        long long my_inc = currentBoard.whiteToMove ? winc_ms : binc_ms;
        if (my_time != -1) {
             int moves_remaining = (movestogo > 0 && movestogo < 80) ? movestogo : 35; 
             allocated_ms = (my_time / moves_remaining) + my_inc - time_buffer_ms;
             allocated_ms = std::min(allocated_ms, my_time / 2 - time_buffer_ms); 
             allocated_ms = std::max(10LL, allocated_ms); 
        } else {
            allocated_ms = 2000 - time_buffer_ms; 
        }
    }
    
    std::chrono::milliseconds timeLimit(allocated_ms);
    
    auto startTime = std::chrono::steady_clock::now();
    time_is_up.store(false, std::memory_order_relaxed); 
    nodes_searched.store(0, std::memory_order_relaxed); 

    std::vector<Move> legalEngineMoves;
    generateLegalMoves(currentBoard, legalEngineMoves, false);
    if (legalEngineMoves.empty()) { std::cout << "bestmove 0000" << std::endl; return; }

    orderMoves(currentBoard, legalEngineMoves); 

    Move bestMoveOverall = legalEngineMoves[0]; 
    Move bestMoveThisIteration = legalEngineMoves[0];
    int bestEvalOverall = currentBoard.whiteToMove ? std::numeric_limits<int>::min() : std::numeric_limits<int>::min(); // Engine maximizes its own score

    bool isEngineWhite = currentBoard.whiteToMove;

    // Iterative Deepening Loop
    for (int currentDepth = 1; currentDepth <= MAX_SEARCH_PLY; ++currentDepth) {
        auto iterationStartTime = std::chrono::steady_clock::now(); 
        int currentIterBestEval = std::numeric_limits<int>::min(); 
        std::vector<Move> candidateBestMovesThisIteration;
        
        uint64_t nodes_at_start_of_iter = nodes_searched.load(std::memory_order_relaxed); 

        for (const auto& engineMove : legalEngineMoves) { 
            BoardState boardAfterEngineMove = currentBoard;
            apply_raw_move_to_board(boardAfterEngineMove, engineMove); 
            int evalFromWhitePerspective = alphaBetaSearch(boardAfterEngineMove, currentDepth - 1, 
                                                           std::numeric_limits<int>::min(), 
                                                           std::numeric_limits<int>::max(), 
                                                           !isEngineWhite, 
                                                           startTime, timeLimit);
            
            if (time_is_up.load(std::memory_order_relaxed)) break; 

            int currentMoveScoreForEngine; 
            if (isEngineWhite) { 
                currentMoveScoreForEngine = evalFromWhitePerspective;
            } else { 
                currentMoveScoreForEngine = -evalFromWhitePerspective; 
            }
            
            if (currentMoveScoreForEngine > currentIterBestEval) {
                currentIterBestEval = currentMoveScoreForEngine;
                candidateBestMovesThisIteration.clear();
                candidateBestMovesThisIteration.push_back(engineMove);
            } else if (currentMoveScoreForEngine == currentIterBestEval) {
                candidateBestMovesThisIteration.push_back(engineMove);
            }
        } 

        if (time_is_up.load(std::memory_order_relaxed)) { break; }

        if (!candidateBestMovesThisIteration.empty()) {
            std::uniform_int_distribution<int> distrib(0, candidateBestMovesThisIteration.size() - 1);
            bestMoveThisIteration = candidateBestMovesThisIteration[distrib(global_rng)];
            bestMoveOverall = bestMoveThisIteration; 
            bestEvalOverall = currentIterBestEval; 
            
            auto iterationEndTime = std::chrono::steady_clock::now();
            auto iterationDuration = std::chrono::duration_cast<std::chrono::milliseconds>(iterationEndTime - iterationStartTime);
            uint64_t nodes_this_iter = nodes_searched.load(std::memory_order_relaxed) - nodes_at_start_of_iter;
            uint64_t nps = (iterationDuration.count() > 0) ? (nodes_this_iter * 1000 / iterationDuration.count()) : 0;

            int uci_score_val = bestEvalOverall;
            std::string uci_score_type = "cp";

            // Refined mate score reporting
            if (abs(uci_score_val) > MATE_SCORE - MAX_SEARCH_PLY * 2 ) { 
                uci_score_type = "mate";
                // Calculate mate in X moves. Positive if engine is mating, negative if engine is being mated.
                // The number of ply to mate from the *root* of the current ID iteration
                int ply_to_mate_from_root_id = MATE_SCORE - abs(uci_score_val); 
                // Convert ply to moves (1 move = 2 ply, but for the last move of a mate it's 1 ply)
                int moves_to_mate = (ply_to_mate_from_root_id + 1) / 2; 
                uci_score_val = (bestEvalOverall > 0) ? moves_to_mate : -moves_to_mate;
            }

            std::cout << "info depth " << currentDepth 
                      << " score " << uci_score_type << " " << uci_score_val
                      << " time " << iterationDuration.count() 
                      << " nodes " << nodes_this_iter
                      << " nps " << nps
                      << " pv " << bestMoveOverall.toUci() << std::endl; 

        } else { break; }

        if (std::chrono::steady_clock::now() - startTime >= timeLimit) { break; }
        if (abs(bestEvalOverall) >= MATE_SCORE - MAX_SEARCH_PLY*2) { break; }

    } // End Iterative Deepening Loop

    std::cout << "bestmove " << bestMoveOverall.toUci() << std::endl;
}

// Main loop 
int main() {
    std::ios_base::sync_with_stdio(false); 
    global_rng.seed(std::chrono::steady_clock::now().time_since_epoch().count()); 
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line); std::string command; iss >> command;
        if (command == "uci") { handleUci(); } 
        else if (command == "isready") { handleIsReady(); } 
        else if (command == "ucinewgame") { handleUciNewGame(); } 
        else if (command == "position") { handlePosition(iss); } 
        else if (command == "go") { handleGo(iss); } 
        else if (command == "quit") { break; }
    }
    return 0;
}
