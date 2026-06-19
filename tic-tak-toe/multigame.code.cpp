#include <iostream>
#include <bits/stdc++.h>

using namespace std;

class Player
{
    int id;
    string name;
    char symbol;

public:
    Player(
        int id,
        const string &name,
        char symbol)
        : id(id),
          name(name),
          symbol(symbol)
    {
    }

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
    int row;
    int col;
    char symbol;

public:
    Move(
        int row,
        int col,
        char symbol)
        : row(row),
          col(col),
          symbol(symbol)
    {
    }

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
    int size;

    vector<vector<char>> grid;

public:
    Board(int size)
        : size(size),
          grid(
              size,
              vector<char>(
                  size,
                  ' '))
    {
    }

    int getSize() const
    {
        return size;
    }

    char getCell(
        int row,
        int col) const
    {
        return grid[row][col];
    }

    bool isCellEmpty(
        int row,
        int col) const
    {
        return grid[row][col] == ' ';
    }

    bool placeMove(
        const Move &move)
    {
        if (!isCellEmpty(
                move.getRow(),
                move.getCol()))
        {
            return false;
        }

        grid[move.getRow()]
            [move.getCol()] =
                move.getSymbol();

        return true;
    }

    void removeMove(
        const Move &move)
    {
        grid[move.getRow()]
            [move.getCol()] = ' ';
    }

    bool isFull() const
    {
        for (auto &row : grid)
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
                cout
                    << " "
                    << grid[i][j]
                    << " ";

                if (j != size - 1)
                {
                    cout << "|";
                }
            }

            cout << "\n";

            if (i != size - 1)
            {
                for (int j = 0; j < size; j++)
                {
                    cout << "---";

                    if (j != size - 1)
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

class MoveValidationStrategy
{
public:
    virtual bool validate(
        const Board &board,
        const Move &move) = 0;

    virtual ~MoveValidationStrategy() = default;
};

class StandardMoveValidator
    : public MoveValidationStrategy
{
public:
    bool validate(
        const Board &board,
        const Move &move) override
    {
        int row =
            move.getRow();

        int col =
            move.getCol();

        int size =
            board.getSize();

        if (row < 0 ||
            col < 0 ||
            row >= size ||
            col >= size)
        {
            return false;
        }

        return board
            .isCellEmpty(
                row,
                col);
    }
};

class RuleStrategy
{
public:
    virtual bool checkWin(
        const Board &board,
        int row,
        int col,
        char symbol) = 0;

    virtual bool checkDraw(
        const Board &board) = 0;

    virtual ~RuleStrategy() = default;
};

class TurnStrategy
{
public:
    virtual shared_ptr<Player>
    nextPlayer(
        const vector<
            shared_ptr<Player>> &players,
        int currentIndex) = 0;

    virtual int
    nextIndex(
        int currentIndex,
        int totalPlayers) = 0;

    virtual ~TurnStrategy() = default;
};

class RoundRobinTurnStrategy
    : public TurnStrategy
{
public:
    shared_ptr<Player>
    nextPlayer(
        const vector<
            shared_ptr<Player>> &players,
        int currentIndex) override
    {
        return players[(currentIndex + 1) %
                       players.size()];
    }

    int nextIndex(
        int currentIndex,
        int totalPlayers) override
    {
        return (currentIndex + 1) %
               totalPlayers;
    }
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
    FINISHED
};

class IGameObserver
{
public:
    virtual void onMovePlayed(
        const Move &move) = 0;

    virtual void onGameFinished(
        const string &message) = 0;

    virtual ~IGameObserver() = default;
};

class ConsoleGameObserver
    : public IGameObserver
{
public:
    void onMovePlayed(
        const Move &move) override
    {
        cout
            << "Move Played -> ("
            << move.getRow()
            << ", "
            << move.getCol()
            << ") "
            << move.getSymbol()
            << endl;
    }

    void onGameFinished(
        const string &message) override
    {
        cout << message << endl;
    }
};

class GameEventManager
{
    vector<
        shared_ptr<IGameObserver>>
        observers;

public:
    void addObserver(
        shared_ptr<IGameObserver>
            observer)
    {
        observers.push_back(
            observer);
    }

    void notifyMove(
        const Move &move)
    {
        for (auto &observer :
             observers)
        {
            observer
                ->onMovePlayed(
                    move);
        }
    }

    void notifyGameFinished(
        const string &message)
    {
        for (auto &observer :
             observers)
        {
            observer
                ->onGameFinished(
                    message);
        }
    }
};

class Game
{
private:
    int gameId;

    shared_ptr<Board> board;

    vector<
        shared_ptr<Player>>
        players;

    int currentPlayerIndex;

    shared_ptr<
        RuleStrategy>
        ruleStrategy;

    shared_ptr<
        MoveValidationStrategy>
        moveValidator;

    shared_ptr<
        TurnStrategy>
        turnStrategy;

    GameStatus status;

    vector<Move> moveHistory;

    GameEventManager
        eventManager;

public:
    Game(
        int gameId,
        int boardSize,
        const vector<
            shared_ptr<Player>>
            &players,
        shared_ptr<
            RuleStrategy>
            ruleStrategy,
        shared_ptr<
            MoveValidationStrategy>
            moveValidator,
        shared_ptr<
            TurnStrategy>
            turnStrategy)
        : gameId(gameId),
          board(
              make_shared<
                  Board>(
                  boardSize)),
          players(players),
          currentPlayerIndex(
              0),
          ruleStrategy(
              ruleStrategy),
          moveValidator(
              moveValidator),
          turnStrategy(
              turnStrategy),
          status(
              GameStatus::
                  IN_PROGRESS)
    {
    }

    void addObserver(
        shared_ptr<
            IGameObserver>
            observer)
    {
        eventManager
            .addObserver(
                observer);
    }

    shared_ptr<Player>
    getCurrentPlayer()
    {
        return players[currentPlayerIndex];
    }

    GameStatus
    getStatus() const
    {
        return status;
    }

    bool makeMove(
        int row,
        int col)
    {
        if (status !=
            GameStatus::
                IN_PROGRESS)
        {
            return false;
        }

        auto player =
            getCurrentPlayer();

        Move move(
            row,
            col,
            player
                ->getSymbol());

        if (!moveValidator
                 ->validate(
                     *board,
                     move))
        {
            cout
                << "Invalid Move\n";

            return false;
        }

        board->placeMove(
            move);

        moveHistory
            .push_back(
                move);

        eventManager
            .notifyMove(
                move);

        board->display();

        bool won =
            ruleStrategy
                ->checkWin(
                    *board,
                    row,
                    col,
                    player
                        ->getSymbol());

        if (won)
        {
            status =
                GameStatus::
                    FINISHED;

            eventManager
                .notifyGameFinished(
                    player
                        ->getName() +
                    " Won");

            return true;
        }

        bool draw =
            ruleStrategy
                ->checkDraw(
                    *board);

        if (draw)
        {
            status =
                GameStatus::
                    DRAW;

            eventManager
                .notifyGameFinished(
                    "Game Draw");

            return true;
        }

        currentPlayerIndex =
            turnStrategy
                ->nextIndex(
                    currentPlayerIndex,
                    players
                        .size());

        return true;
    }

    bool undo()
    {
        if (
            moveHistory.empty())
        {
            return false;
        }

        Move move =
            moveHistory.back();

        moveHistory
            .pop_back();

        board->removeMove(
            move);

        currentPlayerIndex =
            (currentPlayerIndex -
             1 +
             players.size()) %
            players.size();

        status =
            GameStatus::
                IN_PROGRESS;

        return true;
    }

    void start()
    {
        while (
            status ==
            GameStatus::
                IN_PROGRESS)
        {
            auto player =
                getCurrentPlayer();

            int row;
            int col;

            cout
                << player
                       ->getName()
                << "'s Turn\n";

            cout
                << "Enter Row Col : ";

            cin >>
                row >>
                col;

            makeMove(
                row,
                col);
        }
    }
};

class GameManager
{
    unordered_map<
        int,
        shared_ptr<Game>>
        games;

public:
    void createGame(
        shared_ptr<Game>
            game,
        int gameId)
    {
        games[gameId] =
            game;
    }

    shared_ptr<Game>
    getGame(
        int gameId)
    {
        if (
            games.count(
                gameId))
        {
            return games[gameId];
        }

        return nullptr;
    }

    void removeGame(
        int gameId)
    {
        games.erase(
            gameId);
    }
};

class GameFactory
{
public:
    static shared_ptr<Game>
    createStandardGame(
        int gameId,
        int boardSize,
        vector<
            shared_ptr<Player>>
            players)
    {
        return make_shared<Game>(
            gameId,
            boardSize,
            players,
            make_shared<StandardRule>(),
            make_shared<StandardMoveValidator>(),
            make_shared<RoundRobinTurnStrategy>());
    }
};
// why factory pattern is used here? because construction is becoming large if we create game without it
// auto ruleStrategy =
//     make_shared<
//         StandardRule>();

// auto moveValidator =
//     make_shared<
//         StandardMoveValidator>();

// auto turnStrategy =
//     make_shared<
//         RoundRobinTurnStrategy>();

// auto game =
//     make_shared<Game>(
//         1,
//         3,
//         players,
//         ruleStrategy,
//         moveValidator,
//         turnStrategy);
// It provides a simple interface to create different types of games without exposing the creation logic to the client. It also promotes code reusability and separation of concerns, allowing for easier maintenance and scalability as new game types can be added without modifying existing code.
// auto game = GameFactory::createStandardGame(1, 3, players);

int main()
{
    auto player1 =
        make_shared<Player>(
            1,
            "Alpha",
            'X');

    auto player2 =
        make_shared<Player>(
            2,
            "Beta",
            'O');

    vector<
        shared_ptr<Player>>
        players =
            {
                player1,
                player2};

    auto game =
        GameFactory::
            createStandardGame(
                1,
                3,
                players);

    auto consoleObserver =
        make_shared<
            ConsoleGameObserver>();

    game->addObserver(
        consoleObserver);

    GameManager
        gameManager;

    gameManager
        .createGame(
            game,
            1);

    game->start();

    return 0;
}