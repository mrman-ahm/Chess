#include "Chess.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void move(ChessBoard& board, int fromRow, int fromCol, int toRow, int toCol) {
    expect(board.isValidMove(fromRow, fromCol, toRow, toCol), "expected legal move");
    board.movePiece(fromRow, fromCol, toRow, toCol);
}

void testOpeningMoves() {
    ChessBoard board;
    expect(board.board[6][4] == 'P', "white pawn starts on e2");
    expect(!board.isValidMove(6, 4, 3, 4), "pawn cannot move three squares");
    move(board, 6, 4, 4, 4);
    expect(board.board[4][4] == 'P' && board.board[6][4] == '.', "e2-e4 updates board");
    expect(!board.isWhiteTurn, "turn changes after a normal move");
}

void testFoolsMate() {
    ChessBoard board;
    move(board, 6, 5, 5, 5); // f2-f3
    move(board, 1, 4, 3, 4); // e7-e5
    move(board, 6, 6, 4, 6); // g2-g4
    move(board, 0, 3, 4, 7); // Qd8-h4
    expect(board.isKingInCheck(true), "white king is in check");
    expect(board.isCheckmate(true), "Fool's Mate is detected");
}

void testEnPassant() {
    ChessBoard board;
    move(board, 6, 4, 4, 4); // e2-e4
    move(board, 1, 0, 2, 0); // a7-a6
    move(board, 4, 4, 3, 4); // e4-e5
    move(board, 1, 3, 3, 3); // d7-d5
    move(board, 3, 4, 2, 3); // e5xd6 en passant
    expect(board.board[2][3] == 'P', "en passant pawn reaches d6");
    expect(board.board[3][3] == '.', "captured pawn is removed by en passant");
}

void testCastling() {
    ChessBoard board;
    move(board, 6, 4, 4, 4); // e2-e4
    move(board, 1, 4, 3, 4); // e7-e5
    move(board, 7, 6, 5, 5); // Ng1-f3
    move(board, 0, 1, 2, 2); // Nb8-c6
    move(board, 7, 5, 6, 4); // Bf1-e2
    move(board, 0, 6, 2, 5); // Ng8-f6
    move(board, 7, 4, 7, 6); // O-O
    expect(board.board[7][6] == 'K', "king reaches g1 when castling");
    expect(board.board[7][5] == 'R', "rook reaches f1 when castling");
}

void testPromotion() {
    ChessBoard board;
    board.board = {
        "....k...",
        "P.......",
        "........",
        "........",
        "........",
        "........",
        "........",
        "....K..."
    };
    bool needsPromotion = board.movePiece(1, 0, 0, 0);
    expect(needsPromotion, "pawn reaching final rank requests promotion");
    board.promotePiece(0, 0, 'Q');
    expect(board.board[0][0] == 'Q', "promotion replaces pawn with selected piece");
    expect(!board.isWhiteTurn, "turn changes after promotion completes");
}

} // namespace

int main() {
    testOpeningMoves();
    testFoolsMate();
    testEnPassant();
    testCastling();
    testPromotion();
    std::cout << "All chess logic tests passed.\n";
    return 0;
}
