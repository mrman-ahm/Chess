#pragma once
#include <vector>
#include <string>
#include <map>

class ChessBoard {
public:
    std::vector<std::string> board;

    ChessBoard();
    void display() const;
    bool movePiece(int fromRow, int fromCol, int toRow, int toCol);
    void promotePiece(int row, int col, char newPiece);
    bool isValidMove(int fromRow, int fromCol, int toRow, int toCol) const;
    bool isKingInCheck(bool white) const;
    bool hasLegalMoves(bool white) const;
    bool isCheckmate(bool white) const;
    bool isStalemate(bool white) const;
    std::string getLANMove(int fromRow, int fromCol, int toRow, int toCol, char promoChar = '\0') const;

    // Special state for unique moves
    bool isWhiteTurn = true;
    bool whiteKingMoved = false, blackKingMoved = false;
    bool whiteRookLMoved = false, whiteRookRMoved = false;
    bool blackRookLMoved = false, blackRookRMoved = false;
    int enPassantCol = -1; // Column where en passant is possible (-1 if none)
    int halfmoveClock = 0; // Moves since last capture or pawn move
    int checksDeliveredByWhite = 0; // Checks White has delivered to Black's king (3-Check)
    int checksDeliveredByBlack = 0; // Checks Black has delivered to White's king (3-Check)

    // Chess 960 initial state columns
    int initialKingCol = 4;
    int initialRookLCol = 0;
    int initialRookRCol = 7;

    // Crazyhouse pockets: uppercase for White (P, N, B, R, Q), lowercase for Black (p, n, b, r, q)
    std::map<char, int> whitePocket;
    std::map<char, int> blackPocket;

    // Tracker for promoted pieces on the board to handle captures in Crazyhouse (reverts promoted pieces back to pawns)
    // We can store coordinates of promoted pieces, but wait, a simpler way is to track which squares contain a promoted piece,
    // e.g. std::vector<std::vector<bool>> isPromotedPiece;
    std::vector<std::vector<bool>> isPromotedPiece = std::vector<std::vector<bool>>(8, std::vector<bool>(8, false));

    void setupChess960();
    bool isValidDrop(char piece, int row, int col) const;
    void dropPiece(char piece, int row, int col);

    bool isSamePosition(const ChessBoard& other) const;
    bool isInsufficientMaterial() const;

    std::vector<char> whiteCaptured; // Pieces captured BY white
    std::vector<char> blackCaptured; // Pieces captured BY black
};
