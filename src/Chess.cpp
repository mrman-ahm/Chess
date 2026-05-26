#include "Chess.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

ChessBoard::ChessBoard() {
    board = {
        "rnbqkbnr",
        "pppppppp",
        "........",
        "........",
        "........",
        "........",
        "PPPPPPPP",
        "RNBQKBNR"
    };
}

void ChessBoard::display() const {
    std::cout << "  a b c d e f g h" << std::endl;
    std::cout << "  ---------------" << std::endl;
    for (int i = 0; i < 8; ++i) {
        std::cout << 8 - i << "|";
        for (int j = 0; j < 8; ++j) {
            std::cout << board[i][j] << " ";
        }
        std::cout << "|" << 8 - i << std::endl;
    }
    std::cout << "  ---------------" << std::endl;
    std::cout << "  a b c d e f g h" << std::endl;
}

bool ChessBoard::movePiece(int fromRow, int fromCol, int toRow, int toCol) {
    if (!isValidMove(fromRow, fromCol, toRow, toCol)) return false;

    char piece = board[fromRow][fromCol];
    char target = board[toRow][toCol];
    bool isPawnMove = (std::tolower(piece) == 'p');
    bool isCapture = (target != '.' || (isPawnMove && fromCol != toCol && board[toRow][toCol] == '.'));

    if (isCapture || isPawnMove) {
        halfmoveClock = 0;
    } else {
        halfmoveClock++;
    }

    // --- Special Move Execution: Castling ---
    bool isCastling = false;
    bool isKingSide = false;
    if (std::tolower(piece) == 'k') {
        if (std::abs(toCol - fromCol) == 2) {
            isCastling = true;
            isKingSide = (toCol > fromCol);
        } else if (toCol == initialRookLCol || toCol == initialRookRCol) {
            char targetPiece = board[toRow][toCol];
            if (std::tolower(targetPiece) == 'r') {
                isCastling = true;
                isKingSide = (toCol == initialRookRCol);
            }
        }
    }

    if (isCastling) {
        int rookStartCol = isKingSide ? initialRookRCol : initialRookLCol;
        int kingDestCol = isKingSide ? 6 : 2;
        int rookDestCol = isKingSide ? 5 : 3;

        board[fromRow][fromCol] = '.';
        board[fromRow][rookStartCol] = '.';

        board[toRow][kingDestCol] = piece;
        board[toRow][rookDestCol] = isWhiteTurn ? 'R' : 'r';

        if (isWhiteTurn) whiteKingMoved = true;
        else blackKingMoved = true;

        enPassantCol = -1;
        isWhiteTurn = !isWhiteTurn;
        return false; // castling never needs promotion
    }

    // Standard Capture
    if (target != '.') {
        char captured = target;
        if (isPromotedPiece[toRow][toCol]) {
            captured = isWhiteTurn ? 'p' : 'P'; // demote back to pawn
        }
        if (isWhiteTurn) {
            whiteCaptured.push_back(captured);
            whitePocket[std::toupper(captured)]++;
        } else {
            blackCaptured.push_back(captured);
            blackPocket[std::tolower(captured)]++;
        }
    }

    // --- Special Move Execution: En Passant ---
    if (isPawnMove && fromCol != toCol && board[toRow][toCol] == '.') {
        char capturedPawn = board[fromRow][toCol];
        if (isWhiteTurn) {
            whiteCaptured.push_back(capturedPawn);
            whitePocket['P']++;
        } else {
            blackCaptured.push_back(capturedPawn);
            blackPocket['p']++;
        }
        board[fromRow][toCol] = '.'; 
        isPromotedPiece[fromRow][toCol] = false;
    }

    // Move the piece and promoted status
    board[toRow][toCol] = piece;
    board[fromRow][fromCol] = '.';
    isPromotedPiece[toRow][toCol] = isPromotedPiece[fromRow][fromCol];
    isPromotedPiece[fromRow][fromCol] = false;

    // --- Special Move Execution: Promotion Check ---
    bool needsPromotion = (piece == 'P' && toRow == 0) || (piece == 'p' && toRow == 7);

    // --- Update State ---
    enPassantCol = -1;
    if (isPawnMove && std::abs(toRow - fromRow) == 2) {
        enPassantCol = toCol;
    }

    if (piece == 'K') whiteKingMoved = true;
    if (piece == 'k') blackKingMoved = true;
    if (fromRow == 7 && fromCol == initialRookLCol) whiteRookLMoved = true;
    if (fromRow == 7 && fromCol == initialRookRCol) whiteRookRMoved = true;
    if (fromRow == 0 && fromCol == initialRookLCol) blackRookLMoved = true;
    if (fromRow == 0 && fromCol == initialRookRCol) blackRookRMoved = true;

    // Only flip the turn if we don't need to wait for a promotion choice
    if (!needsPromotion) {
        isWhiteTurn = !isWhiteTurn;
        if (isKingInCheck(isWhiteTurn)) {
            if (isWhiteTurn) checksDeliveredByBlack++;
            else checksDeliveredByWhite++;
        }
    }

    return needsPromotion;
}

void ChessBoard::promotePiece(int row, int col, char newPiece) {
    board[row][col] = newPiece;
    isPromotedPiece[row][col] = true;
    isWhiteTurn = !isWhiteTurn; // Finally flip the turn after promotion
    if (isKingInCheck(isWhiteTurn)) {
        if (isWhiteTurn) checksDeliveredByBlack++;
        else checksDeliveredByWhite++;
    }
}

// Helper to check piece-specific rules without checking for King safety (prevents recursion)
bool isPseudoLegal(const std::vector<std::string>& b, int fromR, int fromC, int toR, int toC, int epCol) {
    char piece = b[fromR][fromC];
    char target = b[toR][toC];
    if (piece == '.') return false;

    bool isWhite = (piece >= 'A' && piece <= 'Z');
    if (target != '.') {
        bool isWhiteTarget = (target >= 'A' && target <= 'Z');
        if (isWhite == isWhiteTarget) return false;
    }

    int dR = toR - fromR;
    int dC = toC - fromC;
    int adR = std::abs(dR);
    int adC = std::abs(dC);
    char p = std::tolower(piece);

    auto clear = [&](int sR, int sC) {
        int r = fromR + sR, c = fromC + sC;
        while (r != toR || c != toC) {
            if (b[r][c] != '.') return false;
            r += sR; c += sC;
        }
        return true;
    };

    if (p == 'p') {
        int dir = (isWhite) ? -1 : 1;
        if (dC == 0) {
            if (target != '.') return false;
            if (dR == dir) return true;
            if (fromR == (isWhite ? 6 : 1) && dR == 2 * dir && b[fromR + dir][fromC] == '.') return true;
        } else if (adC == 1 && dR == dir) {
            if (target != '.') return true;
            if (fromR == (isWhite ? 3 : 4) && toC == epCol) return true;
        }
        return false;
    }
    if (p == 'r') return (dR == 0 || dC == 0) && clear((dR == 0 ? 0 : (dR > 0 ? 1 : -1)), (dC == 0 ? 0 : (dC > 0 ? 1 : -1)));
    if (p == 'n') return (adR == 2 && adC == 1) || (adR == 1 && adC == 2);
    if (p == 'b') return (adR == adC) && clear((dR > 0 ? 1 : -1), (dC > 0 ? 1 : -1));
    if (p == 'q') return (adR == adC || dR == 0 || dC == 0) && clear((dR == 0 ? 0 : (dR > 0 ? 1 : -1)), (dC == 0 ? 0 : (dC > 0 ? 1 : -1)));
    if (p == 'k') return adR <= 1 && adC <= 1;
    return false;
}

bool ChessBoard::isKingInCheck(bool white) const {
    int kR = -1, kC = -1;
    char kingChar = white ? 'K' : 'k';
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board[r][c] == kingChar) { kR = r; kC = c; break; }
        }
    }
    if (kR == -1) return false;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            char p = board[r][c];
            if (p != '.' && ((p >= 'A' && p <= 'Z') != white)) {
                if (isPseudoLegal(board, r, c, kR, kC, enPassantCol)) return true;
            }
        }
    }
    return false;
}

bool ChessBoard::isValidMove(int fromRow, int fromCol, int toRow, int toCol) const {
    if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8 ||
        toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8) return false;

    char piece = board[fromRow][fromCol];
    if (piece == '.') return false;

    // --- 1. Enforce Turn Order ---
    bool isPieceWhite = (piece >= 'A' && piece <= 'Z');
    if (isPieceWhite != isWhiteTurn) return false;

    // --- 2. Check Pseudo-Legality ---
    bool isCastling = false;
    bool isKingSide = false;
    if (std::tolower(piece) == 'k') {
        if (std::abs(toCol - fromCol) == 2) {
            isCastling = true;
            isKingSide = (toCol > fromCol);
        } else if (toCol == initialRookLCol || toCol == initialRookRCol) {
            char targetPiece = board[toRow][toCol];
            if (std::tolower(targetPiece) == 'r') {
                isCastling = true;
                isKingSide = (toCol == initialRookRCol);
            }
        }
    }

    if (isCastling) {
        if (isKingInCheck(isWhiteTurn)) return false;
        bool moved = isWhiteTurn ? whiteKingMoved : blackKingMoved;
        if (moved) return false;
        bool rookMoved = isKingSide ? (isWhiteTurn ? whiteRookRMoved : blackRookRMoved) 
                                    : (isWhiteTurn ? whiteRookLMoved : blackRookLMoved);
        if (rookMoved) return false;
        
        int rookStart = isKingSide ? initialRookRCol : initialRookLCol;
        int kingDest = isKingSide ? 6 : 2;
        int rookDest = isKingSide ? 5 : 3;

        // Path clear verification (excluding king and rook starting squares themselves)
        int minC = std::min({fromCol, kingDest, rookStart, rookDest});
        int maxC = std::max({fromCol, kingDest, rookStart, rookDest});
        for (int c = minC; c <= maxC; ++c) {
            if (c == fromCol || c == rookStart) continue;
            if (board[fromRow][c] != '.') return false;
        }

        // Target squares cannot contain other pieces
        if (board[fromRow][kingDest] != '.' && kingDest != fromCol && kingDest != rookStart) return false;
        if (board[fromRow][rookDest] != '.' && rookDest != fromCol && rookDest != rookStart) return false;

        // Cannot castle through check
        int step = (kingDest > fromCol) ? 1 : -1;
        for (int c = fromCol + step; ; c += step) {
            ChessBoard sim = *this;
            sim.board[fromRow][fromCol] = '.';
            sim.board[fromRow][c] = piece;
            if (sim.isKingInCheck(isWhiteTurn)) return false;
            if (c == kingDest) break;
        }
    } else {
        if (!isPseudoLegal(board, fromRow, fromCol, toRow, toCol, enPassantCol)) return false;
    }

    // --- 3. King Safety (Simulation) ---
    ChessBoard simulation = *this;
    // Execute move on simulation
    if (isCastling) {
        int rookStart = isKingSide ? initialRookRCol : initialRookLCol;
        int kingDest = isKingSide ? 6 : 2;
        int rookDest = isKingSide ? 5 : 3;
        simulation.board[fromRow][fromCol] = '.';
        simulation.board[fromRow][rookStart] = '.';
        simulation.board[toRow][kingDest] = piece;
        simulation.board[toRow][rookDest] = isWhiteTurn ? 'R' : 'r';
    } else {
        simulation.board[toRow][toCol] = simulation.board[fromRow][fromCol];
        simulation.board[fromRow][fromCol] = '.';
        if (std::tolower(piece) == 'p' && fromCol != toCol && board[toRow][toCol] == '.') {
            simulation.board[fromRow][toCol] = '.'; // En Passant
        }
    }

    if (simulation.isKingInCheck(isWhiteTurn)) return false;

    return true; 
}

bool ChessBoard::hasLegalMoves(bool white) const {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            char piece = board[r][c];
            if (piece != '.' && (piece >= 'A' && piece <= 'Z') == white) {
                // Try moving this piece to every possible square
                for (int tr = 0; tr < 8; ++tr) {
                    for (int tc = 0; tc < 8; ++tc) {
                        if (isValidMove(r, c, tr, tc)) return true;
                    }
                }
            }
        }
    }
    return false;
}

bool ChessBoard::isCheckmate(bool white) const {
    return isKingInCheck(white) && !hasLegalMoves(white);
}

bool ChessBoard::isStalemate(bool white) const {
    return !isKingInCheck(white) && !hasLegalMoves(white);
}

std::string ChessBoard::getLANMove(int fromRow, int fromCol, int toRow, int toCol, char promoChar) const {
    char piece = board[fromRow][fromCol];
    char target = board[toRow][toCol];
    
    // Castling check
    bool isCastling = false;
    bool isKingSide = false;
    if (std::tolower(piece) == 'k') {
        if (std::abs(toCol - fromCol) == 2) {
            isCastling = true;
            isKingSide = (toCol > fromCol);
        } else if (toCol == initialRookLCol || toCol == initialRookRCol) {
            char targetPiece = board[toRow][toCol];
            if (std::tolower(targetPiece) == 'r') {
                isCastling = true;
                isKingSide = (toCol == initialRookRCol);
            }
        }
    }
    if (isCastling) {
        return isKingSide ? "O-O" : "O-O-O";
    }
    
    std::string moveStr = "";
    if (std::tolower(piece) != 'p') {
        moveStr += std::toupper(piece);
    }
    
    moveStr += (char)(fromCol + 'a');
    moveStr += std::to_string(8 - fromRow);
    
    // Capture or normal move indicator
    bool isCapture = (target != '.');
    // En passant capture detection
    if (std::tolower(piece) == 'p' && fromCol != toCol && target == '.') {
        isCapture = true;
    }
    
    if (isCapture) {
        moveStr += "x";
    } else {
        moveStr += "-";
    }
    
    moveStr += (char)(toCol + 'a');
    moveStr += std::to_string(8 - toRow);
    
    if (promoChar != '\0') {
        moveStr += "=";
        moveStr += std::toupper(promoChar);
    }
    
    return moveStr;
}

bool ChessBoard::isSamePosition(const ChessBoard& other) const {
    if (board != other.board) return false;
    if (isWhiteTurn != other.isWhiteTurn) return false;
    if (whiteKingMoved != other.whiteKingMoved) return false;
    if (blackKingMoved != other.blackKingMoved) return false;
    if (whiteRookLMoved != other.whiteRookLMoved) return false;
    if (whiteRookRMoved != other.whiteRookRMoved) return false;
    if (blackRookLMoved != other.blackRookLMoved) return false;
    if (blackRookRMoved != other.blackRookRMoved) return false;
    if (enPassantCol != other.enPassantCol) return false;
    return true;
}

bool ChessBoard::isInsufficientMaterial() const {
    int wB = 0, wN = 0, bB = 0, bN = 0;
    int wP = 0, bP = 0, wR = 0, bR = 0, wQ = 0, bQ = 0;
    
    int whiteBishopLight = -1; // -1: none, 0: dark, 1: light
    int blackBishopLight = -1;
    
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            char p = board[r][c];
            if (p == '.') continue;
            bool isLightSq = ((r + c) % 2 == 0);
            
            if (p == 'P') wP++;
            else if (p == 'p') bP++;
            else if (p == 'R') wR++;
            else if (p == 'r') bR++;
            else if (p == 'Q') wQ++;
            else if (p == 'q') bQ++;
            else if (p == 'B') {
                wB++;
                whiteBishopLight = isLightSq ? 1 : 0;
            }
            else if (p == 'b') {
                bB++;
                blackBishopLight = isLightSq ? 1 : 0;
            }
            else if (p == 'N') wN++;
            else if (p == 'n') bN++;
        }
    }
    
    // If there are any pawns, rooks, or queens, it is sufficient material
    if (wP > 0 || bP > 0 || wR > 0 || bR > 0 || wQ > 0 || bQ > 0) return false;
    
    int totalPieces = 2 + wB + wN + bB + bN; // 2 kings + minors
    
    // King vs King
    if (totalPieces == 2) return true;
    
    // King + Bishop vs King
    if (totalPieces == 3 && (wB == 1 || bB == 1)) return true;
    
    // King + Knight vs King
    if (totalPieces == 3 && (wN == 1 || bN == 1)) return true;
    
    // King + Bishop vs King + Bishop (same colored bishops)
    if (totalPieces == 4 && wB == 1 && bB == 1 && whiteBishopLight == blackBishopLight) return true;
    
    return false;
}

void ChessBoard::setupChess960() {
    std::vector<int> squares = {0, 1, 2, 3, 4, 5, 6, 7};
    
    int darkB = 2 * (std::rand() % 4);
    int lightB = 2 * (std::rand() % 4) + 1;
    squares.erase(std::find(squares.begin(), squares.end(), darkB));
    squares.erase(std::find(squares.begin(), squares.end(), lightB));
    
    int qIdx = std::rand() % squares.size();
    int queen = squares[qIdx];
    squares.erase(squares.begin() + qIdx);
    
    int n1Idx = std::rand() % squares.size();
    int knight1 = squares[n1Idx];
    squares.erase(squares.begin() + n1Idx);
    
    int n2Idx = std::rand() % squares.size();
    int knight2 = squares[n2Idx];
    squares.erase(squares.begin() + n2Idx);
    
    std::sort(squares.begin(), squares.end());
    int rook1 = squares[0];
    int king = squares[1];
    int rook2 = squares[2];
    
    std::string row = "........";
    row[darkB] = 'b';
    row[lightB] = 'b';
    row[queen] = 'q';
    row[knight1] = 'n';
    row[knight2] = 'n';
    row[rook1] = 'r';
    row[king] = 'k';
    row[rook2] = 'r';
    
    board[0] = row;
    
    std::string whiteRow = row;
    for (char &c : whiteRow) c = std::toupper(c);
    board[7] = whiteRow;
    
    initialKingCol = king;
    initialRookLCol = rook1;
    initialRookRCol = rook2;
    
    whiteKingMoved = false; blackKingMoved = false;
    whiteRookLMoved = false; whiteRookRMoved = false;
    blackRookLMoved = false; blackRookRMoved = false;
}

bool ChessBoard::isValidDrop(char piece, int row, int col) const {
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return false;
    if (board[row][col] != '.') return false;
    
    bool isPieceWhite = (piece >= 'A' && piece <= 'Z');
    if (isPieceWhite != isWhiteTurn) return false;
    
    if (std::tolower(piece) == 'p' && (row == 0 || row == 7)) return false;
    
    if (isPieceWhite) {
        if (whitePocket.count(piece) == 0 || whitePocket.at(piece) <= 0) return false;
    } else {
        if (blackPocket.count(piece) == 0 || blackPocket.at(piece) <= 0) return false;
    }
    
    ChessBoard sim = *this;
    sim.board[row][col] = piece;
    if (sim.isKingInCheck(isWhiteTurn)) return false;
    
    return true;
}

void ChessBoard::dropPiece(char piece, int row, int col) {
    board[row][col] = piece;
    
    if (piece >= 'A' && piece <= 'Z') {
        whitePocket[piece]--;
    } else {
        blackPocket[piece]--;
    }
    
    isPromotedPiece[row][col] = false;
    
    enPassantCol = -1;
    halfmoveClock = 0;
    
    isWhiteTurn = !isWhiteTurn;
    if (isKingInCheck(isWhiteTurn)) {
        if (isWhiteTurn) checksDeliveredByBlack++;
        else checksDeliveredByWhite++;
    }
}
