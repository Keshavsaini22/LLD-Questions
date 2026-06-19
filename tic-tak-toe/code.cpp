#include <iostream>
#include <bits/stdc++.h>

using namespace std;

class Player
{
private:
    int id;
    string name;
    char symbol;

public:
    Player(int id, const string &name, char symbol)
        : id(id), name(name), symbol(symbol) {}

    int getId() const
    {
        return id;
    }

    string getName() const
    {
        return name;
    }

    char getSymbol() const
    {
        return symbol;
    }
};

class Move
{
private:
    int row;
    int col;
    char symbol;

public:
    Move(int row, int col, char symbol)
        : row(row), col(col), symbol(symbol) {}

    int getRow() const
    {
        return row;
    }

    int getCol() const
    {
        return col;
    }

    char getSymbol() const
    {
        return symbol;
    }
};

class Board
{
private:
    int size;
    vector<vector<char>> cells;

public:
    Board(int size)
        : size(size), cells(size, vector<char>(size, ' ')) {}

    int getSize() const
    {
        return size;
    }

    char getCell(int row, int col) const
    {
        return cells[row][col];
    }

    bool isCellEmpty(int row, int col) const
    {
        return cells[row][col] == ' ';
    }

    bool placeMove(const Move &move)
    {
        if (!isCellEmpty(move.getRow(), move.getCol()))
        {
            return false;
        }
        cells[move.getRow()][move.getCol()] = move.getSymbol();
        return true;
    }

    bool isFull() const
    {
        for (auto &row : cells)
        {
            for (char c : row)
            {
                if (c == ' ')
                {
                    return false;
                }
            }
        }
        return true;
    }

    void display() const
    {
        cout << "\n";
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                cout << " " << cells[i][j] << " ";
                if (j < size - 1)
                {
                    cout << "|";
                }
            }
            cout << "\n";
            if (i < size - 1)
            {
                for (int j = 0; j < size; j++)
                {
                    cout << "---";
                    if (j < size - 1)
                    {
                        cout << "+";
                    }
                }
                cout << "\n";
            }
        }
        cout << "\n";
    }
};

class RuleStrategy
{
public:
    virtual bool checkWin(const Board &board, int row, int col, char symbol) = 0;
    virtual bool checkDraw(const Board &board) = 0;
    virtual ~RuleStrategy() = default;
};

class StandardRule : public RuleStrategy
{
public:
    bool checkWin(const Board &board, int row, int col, char symbol) override
    {
        int n = board.getSize();
        bool win = true;

        // Row check
        for (int j = 0; j < n; j++)
        {
            if (board.getCell(row, j) != symbol)
            {
                win = false;
                break;
            }
        }
        if (win)
        {
            return true;
        }

        // Column check
        win = true;
        for (int i = 0; i < n; i++)
        {
            if (board.getCell(i, col) != symbol)
            {
                win = false;
                break;
            }
        }
        if (win)
        {
            return true;
        }

        // Main Diagonal check
        if (row == col)
        {
            win = true;
            for (int i = 0; i < n; i++)
            {
                if (board.getCell(i, i) != symbol)
                {
                    win = false;
                    break;
                }
            }
            if (win)
            {
                return true;
            }
        }

        // Secondary Diagonal check
        if (row + col == n - 1)
        {
            win = true;
            for (int i = 0; i < n; i++)
            {
                if (board.getCell(i, n - i - 1) != symbol)
                {
                    win = false;
                    break;
                }
            }
            if (win)
            {
                return true;
            }
        }

        return false;
    }

    bool checkDraw(const Board &board) override
    {
        return board.isFull();
    }
};

enum class GameStatus
{
    IN_PROGRESS,
    DRAW,
    PLAYER1_WON,
    PLAYER2_WON
};

class TicTacToe
{
private:
    shared_ptr<Player> player1;
    shared_ptr<Player> player2;
    shared_ptr<Player> currentPlayer;
    shared_ptr<Board> board;
    shared_ptr<RuleStrategy> ruleStrategy;
    GameStatus status;
    vector<Move> moveHistory;

public:
    TicTacToe(shared_ptr<Player> player1, shared_ptr<Player> player2, int boardSize, shared_ptr<RuleStrategy> ruleStrategy)
        : player1(player1),
          player2(player2),
          currentPlayer(player1),
          board(make_shared<Board>(boardSize)),
          ruleStrategy(ruleStrategy),
          status(GameStatus::IN_PROGRESS) {}

    void switchTurn()
    {
        if (currentPlayer->getId() == player1->getId())
        {
            currentPlayer = player2;
        }
        else
        {
            currentPlayer = player1;
        }
    }

    void makeMove(int row, int col)
    {
        if (status != GameStatus::IN_PROGRESS)
        {
            cout << "Game already finished\n";
            return;
        }

        Move move(row, col, currentPlayer->getSymbol());
        bool success = board->placeMove(move);

        if (!success)
        {
            cout << "Invalid Move\n";
            return;
        }

        moveHistory.push_back(move);
        board->display();

        bool win = ruleStrategy->checkWin(*board, row, col, currentPlayer->getSymbol());
        if (win)
        {
            if (currentPlayer->getId() == player1->getId())
            {
                status = GameStatus::PLAYER1_WON;
            }
            else
            {
                status = GameStatus::PLAYER2_WON;
            }
            cout << currentPlayer->getName() << " Won the game\n";
            return;
        }

        bool draw = ruleStrategy->checkDraw(*board);
        if (draw)
        {
            status = GameStatus::DRAW;
            cout << "Game Draw\n";
            return;
        }

        switchTurn();
    }

    void startGame()
    {
        while (status == GameStatus::IN_PROGRESS)
        {
            int row;
            int col;
            cout << currentPlayer->getName() << "'s Turn\n";
            cout << "Enter Row Col : ";
            cin >> row >> col;
            makeMove(row, col);
        }
    }
};

int main()
{
    int boardSize;
    cout << "Enter Board Size : ";
    cin >> boardSize;

    string p1Name, p2Name;
    char p1Symbol, p2Symbol;

    cout << "Player1 Name : ";
    cin >> p1Name;
    cout << "Player1 Symbol : ";
    cin >> p1Symbol;

    cout << "Player2 Name : ";
    cin >> p2Name;
    cout << "Player2 Symbol : ";
    cin >> p2Symbol;

    auto player1 = make_shared<Player>(1, p1Name, p1Symbol);
    auto player2 = make_shared<Player>(2, p2Name, p2Symbol);
    auto ruleStrategy = make_shared<StandardRule>();

    TicTacToe game(player1, player2, boardSize, ruleStrategy);
    game.startGame();

    return 0;
}