#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <cmath>
#include <sstream>
#include <windows.h>    
#include <conio.h>
#include <cstdlib>
#include <ctime>

using namespace std;

struct User {
    string username;
    string passwordHash;
    int elo;
};

User player1, player2;

// Chess game global variables
bool possible[8][8] = { false };
const int BOARD_SIZE = 8;
char board[BOARD_SIZE][BOARD_SIZE];
int cursorRow = 7, cursorCol = 0;
bool pieceSelected = false;
int selectedRow, selectedCol;

bool whiteKingMoved = false;     // CASTLING
bool blackKingMoved = false;     // CASTLING
bool whiteRookLeftMoved = false; // CASTLING
bool whiteRookRightMoved = false;
bool blackRookLeftMoved = false;
bool blackRookRightMoved = false;

// User authentication functions
string hashPassword(const string& password) {
    int hash = 0;
    for (char c : password) {
        hash = (hash * 31 + c) % 1000000007;
    }
    return to_string(hash);
}

void printTitle() {
    cout << "==============================\n";
    cout << "     Welcome to CLI Chess     \n";
    cout << "==============================\n";
}

void pause() {
    cout << "\nPress any key to continue...";
    _getch();
}

bool userExists(const string& username) {
    ifstream file("users.txt");
    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string user;
        iss >> user;
        if (user == username) return true;
    }
    return false;
}

bool registerUser() {
    string username, password;
    cout << "\nChoose a username: ";
    cin >> username;

    if (userExists(username)) {
        cout << "Username already exists.\n";
        return false;
    }

    cout << "Choose a password: ";
    char ch;
    password = "";
    while ((ch = _getch()) != '\r') { // Enter key
        if (ch == '\b') {
            if (!password.empty()) {
                cout << "\b \b";
                password.pop_back();
            }
        }
        else {
            password.push_back(ch);
            cout << '*';
        }
    }
    cout << endl;

    ofstream file("users.txt", ios::app);
    file << username << " " << hashPassword(password) << " 1200\n";
    cout << "Registration successful! Starting ELO: 1200\n";
    return true;
}

bool loginUser(User& user) {
    string username, password;
    cout << "\nUsername: ";
    cin >> username;

    cout << "Password: ";
    char ch;
    password = "";
    while ((ch = _getch()) != '\r') {
        if (ch == '\b') {
            if (!password.empty()) {
                cout << "\b \b";
                password.pop_back();
            }
        }
        else {
            password.push_back(ch);
            cout << '*';
        }
    }
    cout << endl;

    ifstream file("users.txt");
    string line;
    string hashed = hashPassword(password);

    while (getline(file, line)) {
        istringstream iss(line);
        string userInFile, passHash;
        int elo;
        iss >> userInFile >> passHash >> elo;
        if (userInFile == username && passHash == hashed) {
            user.username = username;
            user.passwordHash = passHash;
            user.elo = elo;
            cout << "\nLogin successful! ELO: " << elo << "\n";
            return true;
        }
    }
    cout << "\nLogin failed.\n";
    return false;
}

void printMenu(int selected) {
    system("cls");
    printTitle();
    string options[] = { "Login", "Register", "Exit" };
    cout << "\nUse UP/DOWN arrows to navigate, ENTER to select:\n\n";
    for (int i = 0; i < 3; ++i) {
        if (i == selected) cout << " > ";
        else cout << "   ";
        cout << options[i] << "\n";
    }
}

int navigateMenu() {
    int selected = 0;
    while (true) {
        printMenu(selected);
        int key = _getch();

        if (key == 224) {
            key = _getch();
            if (key == 72 && selected > 0) selected--;       // Up
            else if (key == 80 && selected < 2) selected++;  // Down
        }
        else if (key == 13) {
            return selected + 1;
        }
    }
}

// ELO Rating System
void updateEloRatings(User& winner, User& loser) {
    // Simple ELO calculation
    // Expected score for player A
    double expectedA = 1.0 / (1.0 + pow(10, (loser.elo - winner.elo) / 400.0));
    double expectedB = 1.0 / (1.0 + pow(10, (winner.elo - loser.elo) / 400.0));

    // K-factor (usually between 16 and 32)
    int k = 32;

    // Update ELOs
    winner.elo = winner.elo + k * (1 - expectedA);
    loser.elo = loser.elo + k * (0 - expectedB);

    // Make sure ELO doesn't go below 100
    if (loser.elo < 100) loser.elo = 100;
}

void saveUserElos() {
    ifstream inFile("users.txt");
    ofstream outFile("users_temp.txt");
    string line;

    while (getline(inFile, line)) {
        istringstream iss(line);
        string username, passHash;
        int elo;
        iss >> username >> passHash >> elo;

        if (username == player1.username) {
            outFile << username << " " << passHash << " " << player1.elo << "\n";
        }
        else if (username == player2.username) {
            outFile << username << " " << passHash << " " << player2.elo << "\n";
        }
        else {
            outFile << line << "\n";
        }
    }

    inFile.close();
    outFile.close();

    remove("users.txt");
    rename("users_temp.txt", "users.txt");
}

// Chess game functions
void clearScreen() {
    cout << "\033[2J\033[1;1H";
}

void initializeBoard() {
    for (int i = 0; i < BOARD_SIZE; ++i) {
        board[1][i] = 'p';
        board[6][i] = 'P';
    }
    const char white_pieces[] = { 'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R' };
    const char black_pieces[] = { 'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r' };
    for (int i = 0; i < BOARD_SIZE; ++i) {
        board[0][i] = black_pieces[i];
        board[7][i] = white_pieces[i];
    }
    for (int i = 2; i < 6; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            board[i][j] = '-';
}

void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void drawCursorBoard() {
    const int padding = 10;
    cout << string(padding + 7, ' ') << "  a b c d e f g h\n";
    for (int i = BOARD_SIZE - 1; i >= 0; --i) {
        cout << string(padding + 7, ' ') << i + 1 << " ";
        for (int j = 0; j < BOARD_SIZE; ++j) {
            char piece = board[i][j];
            bool isCursor = (i == cursorRow && j == cursorCol);
            bool isSelected = pieceSelected && i == selectedRow && j == selectedCol;
            bool isPossible = possible[i][j];

            if (piece == '-') setColor(8);
            else if (isupper(piece)) setColor(15);
            else setColor(11);

            if (isSelected) setColor(79);
            else if (isCursor) setColor(160);
            else if (isPossible) setColor(10);

            cout << (piece == '-' ? "_ " : string(1, piece) + " ");
            setColor(7);
        }
        cout << i + 1 << "\n";
    }
    cout << string(padding + 7, ' ') << "  a b c d e f g h\n";
}

bool isPathClear(int row, int col1, int col2) {
    if (col1 > col2) swap(col1, col2);
    for (int c = col1 + 1; c < col2; ++c)
        if (board[row][c] != '-') return false;
    return true;
}

bool isCastlingMove(int srcRow, int srcCol, int destRow, int destCol, char player) {
    if (player == 'w' && srcRow == 7 && srcCol == 4 && destRow == 7 && (destCol == 6 || destCol == 2)) {
        if (whiteKingMoved) return false;
        if (destCol == 6 && !whiteRookRightMoved && board[7][7] == 'R' && isPathClear(7, 4, 7)) return true;
        if (destCol == 2 && !whiteRookLeftMoved && board[7][0] == 'R' && isPathClear(7, 0, 4)) return true;
    }
    if (player == 'b' && srcRow == 0 && srcCol == 4 && destRow == 0 && (destCol == 6 || destCol == 2)) {
        if (blackKingMoved) return false;
        if (destCol == 6 && !blackRookRightMoved && board[0][7] == 'r' && isPathClear(0, 4, 7)) return true;
        if (destCol == 2 && !blackRookLeftMoved && board[0][0] == 'r' && isPathClear(0, 0, 4)) return true;
    }
    return false;
}

void performCastling(int row, int destCol) {
    if (destCol == 6) {
        board[row][5] = board[row][7];
        board[row][7] = '-';
    }
    else if (destCol == 2) {
        board[row][3] = board[row][0];
        board[row][0] = '-';
    }
}

bool isValidPawnMove(int srcRow, int srcCol, int destRow, int destCol, char player) {
    int direction = (player == 'w') ? -1 : 1;
    bool isOpponentPiece = (player == 'w') ? islower(board[destRow][destCol]) : isupper(board[destRow][destCol]);

    if (srcCol == destCol && board[destRow][destCol] == '-') {
        return (destRow - srcRow == direction) ||
            (srcRow == (player == 'w' ? 6 : 1) && destRow - srcRow == 2 * direction && board[srcRow + direction][srcCol] == '-');
    }

    if (abs(srcCol - destCol) == 1 && destRow - srcRow == direction && isOpponentPiece) {
        return true;
    }

    return false;
}

bool isValidRookMove(int srcRow, int srcCol, int destRow, int destCol, char player) {
    if (srcRow != destRow && srcCol != destCol) return false;
    int rowStep = (srcRow < destRow) ? 1 : (srcRow > destRow ? -1 : 0);
    int colStep = (srcCol < destCol) ? 1 : (srcCol > destCol ? -1 : 0);
    for (int i = srcRow + rowStep, j = srcCol + colStep; i != destRow || j != destCol; i += rowStep, j += colStep) {
        if (board[i][j] != '-') return false;
    }
    char target = board[destRow][destCol];
    return target == '-' || (player == 'w' ? islower(target) : isupper(target));
}

bool isValidBishopMove(int srcRow, int srcCol, int destRow, int destCol, char player) {
    if (abs(srcRow - destRow) != abs(srcCol - destCol)) return false;
    int rowStep = (srcRow < destRow) ? 1 : -1;
    int colStep = (srcCol < destCol) ? 1 : -1;
    for (int i = srcRow + rowStep, j = srcCol + colStep; i != destRow; i += rowStep, j += colStep)
        if (board[i][j] != '-') return false;
    char target = board[destRow][destCol];
    return target == '-' || (player == 'w' ? islower(target) : isupper(target));
}

bool isValidKnightMove(int srcRow, int srcCol, int destRow, int destCol, char player) {
    int dr = abs(srcRow - destRow), dc = abs(srcCol - destCol);
    char target = board[destRow][destCol];
    return (dr == 2 && dc == 1 || dr == 1 && dc == 2) &&
        (target == '-' || (player == 'w' ? islower(target) : isupper(target)));
}

bool isValidQueenMove(int srcRow, int srcCol, int destRow, int destCol, char player) {
    return isValidRookMove(srcRow, srcCol, destRow, destCol, player) ||
        isValidBishopMove(srcRow, srcCol, destRow, destCol, player);
}

bool isValidKingMove(int srcRow, int srcCol, int destRow, int destCol, char player) {
    if (isCastlingMove(srcRow, srcCol, destRow, destCol, player)) return true;
    int dr = abs(srcRow - destRow), dc = abs(srcCol - destCol);
    char target = board[destRow][destCol];
    return dr <= 1 && dc <= 1 &&
        (target == '-' || (player == 'w' ? islower(target) : isupper(target)));
}

bool isValidMove(int srcRow, int srcCol, int destRow, int destCol, char player) {
    if (srcRow == destRow && srcCol == destCol) return false;
    char piece = board[srcRow][srcCol];
    if (player == 'w' && !isupper(piece)) return false;
    if (player == 'b' && !islower(piece)) return false;
    piece = tolower(piece);
    switch (piece) {
    case 'p': return isValidPawnMove(srcRow, srcCol, destRow, destCol, player);
    case 'r': return isValidRookMove(srcRow, srcCol, destRow, destCol, player);
    case 'n': return isValidKnightMove(srcRow, srcCol, destRow, destCol, player);
    case 'b': return isValidBishopMove(srcRow, srcCol, destRow, destCol, player);
    case 'q': return isValidQueenMove(srcRow, srcCol, destRow, destCol, player);
    case 'k': return isValidKingMove(srcRow, srcCol, destRow, destCol, player);
    default: return false;
    }
}

bool isKingInCheck(char player) {
    char king = (player == 'w') ? 'K' : 'k';
    int kr = -1, kc = -1;
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            if (board[i][j] == king) kr = i, kc = j;
    char opponent = (player == 'w') ? 'b' : 'w';
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            if ((opponent == 'w' && isupper(board[i][j])) ||
                (opponent == 'b' && islower(board[i][j])))
                if (isValidMove(i, j, kr, kc, opponent)) return true;
    return false;
}

bool hasAnyLegalMove(char player) {
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j) {
            char piece = board[i][j];
            if ((player == 'w' && !isupper(piece)) || (player == 'b' && !islower(piece))) continue;
            for (int ni = 0; ni < 8; ++ni)
                for (int nj = 0; nj < 8; ++nj)
                    if (isValidMove(i, j, ni, nj, player)) {
                        char backup = board[ni][nj];
                        board[ni][nj] = board[i][j];
                        board[i][j] = '-';
                        bool stillCheck = isKingInCheck(player);
                        board[i][j] = board[ni][nj];
                        board[ni][nj] = backup;
                        if (!stillCheck) return true;
                    }
        }
    return false;
}

bool isCheckmate(char player) {
    return isKingInCheck(player) && !hasAnyLegalMove(player);
}

void possibleMoves(int row, int col, char player) {
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            possible[i][j] = isValidMove(row, col, i, j, player);
}

void promotePawn(int row, int col, char player) {
    char newPiece = (player == 'w') ? 'Q' : 'q';
    board[row][col] = newPiece;
}

void playGame() {
    char currentPlayer = 'w';
    bool gameOver = false;

    while (!gameOver) {
        clearScreen();
        drawCursorBoard();
        User& currentPlayerUser = (currentPlayer == 'w') ? player1 : player2;
        User& otherPlayerUser = (currentPlayer == 'w') ? player2 : player1;

        cout << endl << "\t        -> " << currentPlayerUser.username << "'s Turn ("
            << (currentPlayer == 'w' ? "UpperCase" : "LowerCase") << ") - ELO: "
            << currentPlayerUser.elo << "\n";

        if (isCheckmate(currentPlayer)) {
            cout << "\n\t CHECKMATE! " << otherPlayerUser.username
                << " (" << (currentPlayer == 'w' ? "LowerCase" : "UpperCase") << ") wins!\n";

            // Update ELO ratings
            updateEloRatings(otherPlayerUser, currentPlayerUser);

            cout << "\n\t New ELO ratings:\n";
            cout << "\t " << player1.username << ": " << player1.elo << "\n";
            cout << "\t " << player2.username << ": " << player2.elo << "\n";

            // Save updated ELO ratings
            saveUserElos();

            cout << "\t Press any key to exit...";
            _getch();
            gameOver = true;
            continue;
        }
        else if (isKingInCheck(currentPlayer)) {
            cout << "\n\t " << currentPlayerUser.username << " ("
                << (currentPlayer == 'w' ? "UpperCase" : "LowerCase") << ") is in CHECK!";
        }

        int ch = _getch();
        if (ch == 224) {
            int arrow = _getch();
            if (arrow == 72 && cursorRow < 7) cursorRow++;
            else if (arrow == 80 && cursorRow > 0) cursorRow--;
            else if (arrow == 75 && cursorCol > 0) cursorCol--;
            else if (arrow == 77 && cursorCol < 7) cursorCol++;
        }
        else if (ch == 13) {
            if (!pieceSelected) {
                if ((currentPlayer == 'w' && isupper(board[cursorRow][cursorCol])) ||
                    (currentPlayer == 'b' && islower(board[cursorRow][cursorCol]))) {
                    selectedRow = cursorRow;
                    selectedCol = cursorCol;
                    pieceSelected = true;
                    possibleMoves(selectedRow, selectedCol, currentPlayer);
                }
            }
            else {
                if (isValidMove(selectedRow, selectedCol, cursorRow, cursorCol, currentPlayer)) {
                    char movedPiece = board[selectedRow][selectedCol];
                    char captured = board[cursorRow][cursorCol];
                    bool castling = isCastlingMove(selectedRow, selectedCol, cursorRow, cursorCol, currentPlayer);

                    board[cursorRow][cursorCol] = movedPiece;
                    board[selectedRow][selectedCol] = '-';
                    if (castling) performCastling(cursorRow, cursorCol);

                    if (isKingInCheck(currentPlayer)) {
                        board[selectedRow][selectedCol] = movedPiece;
                        board[cursorRow][cursorCol] = captured;
                        if (castling) performCastling(cursorRow, cursorCol); // Undo castling
                        cout << "\nInvalid move! King would be in check.\n";
                    }
                    else {
                        // Update castling flags
                        if (movedPiece == 'K') whiteKingMoved = true;
                        if (movedPiece == 'k') blackKingMoved = true;
                        if (movedPiece == 'R' && selectedCol == 0 && selectedRow == 7) whiteRookLeftMoved = true;
                        if (movedPiece == 'R' && selectedCol == 7 && selectedRow == 7) whiteRookRightMoved = true;
                        if (movedPiece == 'r' && selectedCol == 0 && selectedRow == 0) blackRookLeftMoved = true;
                        if (movedPiece == 'r' && selectedCol == 7 && selectedRow == 0) blackRookRightMoved = true;

                        // Promotion
                        if ((movedPiece == 'P' && cursorRow == 0) || (movedPiece == 'p' && cursorRow == 7)) {
                            promotePawn(cursorRow, cursorCol, currentPlayer);
                        }

                        currentPlayer = (currentPlayer == 'w') ? 'b' : 'w';
                    }
                }
                pieceSelected = false;
                for (int i = 0; i < 8; ++i)
                    for (int j = 0; j < 8; ++j)
                        possible[i][j] = false;
            }
        }
        else if (ch == 'q' || ch == 'Q') {
            gameOver = true;
        }
    }
}

int main() {
    srand(time(NULL));
    printTitle();

    cout << "\nWelcome to CLI Chess with ELO Rating!\n";
    cout << "Two players need to log in to play.\n\n";

    // First player login
    cout << "PLAYER 1 (UpperCase/White):\n";
    while (true) {
        int choice = navigateMenu();
        if (choice == 1) {
            if (loginUser(player1)) break;
            pause();
        }
        else if (choice == 2) {
            registerUser();
            pause();
        }
        else {
            cout << "Exiting...\n";
            return 0;
        }
    }

    // Second player login
    cout << "\nPLAYER 2 (LowerCase/Black):\n";
    while (true) {
        int choice = navigateMenu();
        if (choice == 1) {
            if (loginUser(player2) && player2.username != player1.username) break;
            if (player2.username == player1.username) {
                cout << "Same user cannot play against themselves. Please log in with a different account.\n";
                pause();
            }
            else {
                pause();
            }
        }
        else if (choice == 2) {
            registerUser();
            pause();
        }
        else {
            cout << "Exiting...\n";
            return 0;
        }
    }

    // After both players logged in
    system("cls");
    printTitle();
    cout << "\nMatch setup:\n";
    cout << "White (UpperCase): " << player1.username << " (ELO: " << player1.elo << ")\n";
    cout << "Black (LowerCase): " << player2.username << " (ELO: " << player2.elo << ")\n\n";
    cout << "Press any key to start the game...";
    _getch();

    // Initialize and play the game
    initializeBoard();
    playGame();

    return 0;
}
