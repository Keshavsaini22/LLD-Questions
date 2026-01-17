//Observer Pattern - Notification System
interface IObserver {
    update(message: string): void;
}

class SnakeAndLadderConsoleNotifier implements IObserver {
    update(message: string): void {
        console.log(`Console Notification: ${message}`);
    }
}

//Dice Class
class Dice {
    private faces: number;

    constructor(faces: number) {
        this.faces = faces;
    }

    roll(): number {
        return Math.floor(Math.random() * this.faces) + 1;
    }
}

//Base class for Snake and Ladder as both have start and end positions
abstract class BoardElement {
    constructor(public start: number, public end: number) { }

    getStart(): number {
        return this.start;
    }

    getEnd(): number {
        return this.end;
    }

    abstract name(): string;

    abstract display(): void;

}

//Snake Class
class Snake extends BoardElement {
    constructor(start: number, end: number) {
        if (end >= start) {
            throw new Error("Invalid Snake: End position must be less than start position.");
        }

        super(start, end);
    }

    name(): string {
        return "Snake";
    }
    display(): void {
        console.log(`Snake from ${this.start} to ${this.end}`);
    }
}

//Ladder Class
class Ladder extends BoardElement {
    constructor(start: number, end: number) {
        if (end <= start) {
            throw new Error("Invalid Ladder: End position must be greater than start position.");
        }
        super(start, end);
    }

    name(): string {
        return "Ladder";
    }

    display(): void {
        console.log(`Ladder from ${this.start} to ${this.end}`);
    }
}

//Player Class
class Player {
    private position: number;
    private playerId: number;
    private name: string;
    private score: number;

    constructor(playerId: number, name: string) {
        this.playerId = playerId;
        this.name = name;
        this.position = 0;
        this.score = 0;
    }

    getPosition(): number {
        return this.position;
    }

    setPosition(position: number): void {
        this.position = position;
    }

    getName(): string {
        return this.name;
    }

    getPlayerId(): number {
        return this.playerId;
    }

    getScore(): number {
        return this.score;
    }

    incrementScore(): void {
        this.score += 1;
    }
}

//Board Class
class Board {
    private size: number;
    private snakesAndLadders: BoardElement[] = [];
    private boardElementMap: Map<number, BoardElement> = new Map();

    constructor(s: number) {
        this.size = s * s; // m*m board
    }

    canAddEntity(position: number): boolean {
        return !this.boardElementMap.has(position);
    }

    addBoardEntity(boardEntity: BoardElement): void {
        const start = boardEntity.getStart();

        if (this.canAddEntity(start)) {
            this.snakesAndLadders.push(boardEntity);
            this.boardElementMap.set(start, boardEntity);
        }
    }

    setupBoard(strategy: BoardSetupStrategy): void {
        strategy.setup(this);
    }

    getElement(position: number): BoardElement | null {
        return this.boardElementMap.get(position) ?? null;
    }

    getBoardSize(): number {
        return this.size;
    }

    display(): void {
        console.log("\n=== Board Configuration ===");
        console.log(`Board Size: ${this.size} cells`);

        let snakeCount = 0;
        let ladderCount = 0;

        for (const entity of this.snakesAndLadders) {
            if (entity.name() === "SNAKE") snakeCount++;
            else ladderCount++;
        }

        console.log(`\nSnakes: ${snakeCount}`);
        for (const entity of this.snakesAndLadders) {
            if (entity.name() === "SNAKE") {
                entity.display();
            }
        }

        console.log(`\nLadders: ${ladderCount}`);
        for (const entity of this.snakesAndLadders) {
            if (entity.name() === "LADDER") {
                entity.display();
            }
        }

        console.log("=========================");
    }

}

//Strategy pattern for Board setup
interface BoardSetupStrategy {
    setup(board: Board): void;
}

class StandardBoardSetupStrategy implements BoardSetupStrategy {

    setup(board: Board): void {
        if (board.getBoardSize() !== 100) {
            console.log("Standard setup only works for 10x10 board!");
            return;
        }

        // Snakes
        board.addBoardEntity(new Snake(99, 54));
        board.addBoardEntity(new Snake(95, 75));
        board.addBoardEntity(new Snake(92, 88));
        board.addBoardEntity(new Snake(89, 68));
        board.addBoardEntity(new Snake(74, 53));
        board.addBoardEntity(new Snake(64, 60));
        board.addBoardEntity(new Snake(62, 19));
        board.addBoardEntity(new Snake(49, 11));
        board.addBoardEntity(new Snake(46, 25));
        board.addBoardEntity(new Snake(16, 6));

        // Ladders
        board.addBoardEntity(new Ladder(2, 38));
        board.addBoardEntity(new Ladder(7, 14));
        board.addBoardEntity(new Ladder(8, 31));
        board.addBoardEntity(new Ladder(15, 26));
        board.addBoardEntity(new Ladder(21, 42));
        board.addBoardEntity(new Ladder(28, 84));
        board.addBoardEntity(new Ladder(36, 44));
        board.addBoardEntity(new Ladder(51, 67));
        board.addBoardEntity(new Ladder(71, 91));
        board.addBoardEntity(new Ladder(78, 98));
        board.addBoardEntity(new Ladder(87, 94));
    }
}

class CustomCountBoardSetupStrategy implements BoardSetupStrategy {

    private numSnakes: number;
    private numLadders: number;
    private randomPositions: boolean;
    private snakePositions: Array<[number, number]> = [];
    private ladderPositions: Array<[number, number]> = [];

    constructor(snakes: number, ladders: number, random: boolean) {
        this.numSnakes = snakes;
        this.numLadders = ladders;
        this.randomPositions = random;
    }

    addSnakePosition(start: number, end: number): void {
        this.snakePositions.push([start, end]);
    }

    addLadderPosition(start: number, end: number): void {
        this.ladderPositions.push([start, end]);
    }

    setup(board: Board): void {
        const boardSize = board.getBoardSize();

        if (this.randomPositions) {
            // Random snakes
            let snakesAdded = 0;
            while (snakesAdded < this.numSnakes) {
                const start = Math.floor(Math.random() * (boardSize - 10)) + 10;
                const end = Math.floor(Math.random() * (start - 1)) + 1;

                if (board.canAddEntity(start)) {
                    board.addBoardEntity(new Snake(start, end));
                    snakesAdded++;
                }
            }

            // Random ladders
            let laddersAdded = 0;
            while (laddersAdded < this.numLadders) {
                const start = Math.floor(Math.random() * (boardSize - 10)) + 1;
                const end = Math.floor(Math.random() * (boardSize - start)) + start + 1;

                if (board.canAddEntity(start) && end < boardSize) {
                    board.addBoardEntity(new Ladder(start, end));
                    laddersAdded++;
                }
            }

        } else {
            // User defined positions
            for (const [start, end] of this.snakePositions) {
                if (board.canAddEntity(start)) {
                    board.addBoardEntity(new Snake(start, end));
                }
            }

            for (const [start, end] of this.ladderPositions) {
                if (board.canAddEntity(start)) {
                    board.addBoardEntity(new Ladder(start, end));
                }
            }
        }
    }
}


enum Difficulty {
    EASY,
    MEDIUM,
    HARD
}

class RandomBoardSetupStrategy implements BoardSetupStrategy {

    private difficulty: Difficulty;

    constructor(difficulty: Difficulty) {
        this.difficulty = difficulty;
    }

    private setupWithProbability(board: Board, snakeProbability: number): void {
        const boardSize = board.getBoardSize();
        const totalEntities = Math.floor(boardSize / 10);

        for (let i = 0; i < totalEntities; i++) {
            const prob = Math.random();

            if (prob < snakeProbability) {
                // Add snake
                let attempts = 0;
                while (attempts < 50) {
                    const start = Math.floor(Math.random() * (boardSize - 10)) + 10;
                    const end = Math.floor(Math.random() * (start - 1)) + 1;

                    if (board.canAddEntity(start)) {
                        board.addBoardEntity(new Snake(start, end));
                        break;
                    }
                    attempts++;
                }
            } else {
                // Add ladder
                let attempts = 0;
                while (attempts < 50) {
                    const start = Math.floor(Math.random() * (boardSize - 10)) + 1;
                    const end = Math.floor(Math.random() * (boardSize - start)) + start + 1;

                    if (board.canAddEntity(start) && end < boardSize) {
                        board.addBoardEntity(new Ladder(start, end));
                        break;
                    }
                    attempts++;
                }
            }
        }
    }

    setup(board: Board): void {
        switch (this.difficulty) {
            case Difficulty.EASY:
                this.setupWithProbability(board, 0.3);
                break;
            case Difficulty.MEDIUM:
                this.setupWithProbability(board, 0.5);
                break;
            case Difficulty.HARD:
                this.setupWithProbability(board, 0.7);
                break;
        }
    }
}

//Strategy pattern for game rules
interface SnakeAndLadderRules {
    isValidMove(currentPos: number, diceValue: number, boardSize: number): boolean;
    calculateNewPosition(currentPos: number, diceValue: number, board: Board): number;
    checkWinCondition(position: number, boardSize: number): boolean;
}

class StandardSnakeAndLadderRules implements SnakeAndLadderRules {

    isValidMove(currentPos: number, diceValue: number, boardSize: number): boolean {
        return (currentPos + diceValue) <= boardSize;
    }

    calculateNewPosition(currentPos: number, diceValue: number, board: Board): number {
        const newPos = currentPos + diceValue;
        const entity = board.getElement(newPos);

        if (entity !== null) {
            return entity.getEnd();
        }
        return newPos;
    }

    checkWinCondition(position: number, boardSize: number): boolean {
        return position === boardSize;
    }
}

class SnakeAndLadderGame {
    private board: Board;
    private dice: Dice;
    private players: Player[] = [];
    private rules: SnakeAndLadderRules;
    private observers: IObserver[] = [];
    private gameOver: boolean = false;

    constructor(b: Board, d: Dice) {
        this.board = b;
        this.dice = d;
        this.rules = new StandardSnakeAndLadderRules();
    }

    addPlayer(player: Player): void {
        this.players.push(player);
    }

    addObserver(observer: IObserver): void {
        this.observers.push(observer);
    }

    notify(msg: string): void {
        for (const observer of this.observers) {
            observer.update(msg);
        }
    }

    displayPlayerPositions(): void {
        console.log("\n=== Current Positions ===");
        for (const player of this.players) {
            console.log(`${player.getName()}: ${player.getPosition()}`);
        }
        console.log("======================");
    }

    play(): void {
        if (this.players.length < 2) {
            console.log("Need at least 2 players!");
            return;
        }

        this.notify("Game started");
        this.board.display();

        while (!this.gameOver) {
            const currentPlayer = this.players[0];

            console.log(`\n${currentPlayer.getName()}'s turn.`);

            const diceValue = this.dice.roll();
            console.log("Rolled:", diceValue);

            const currentPos = currentPlayer.getPosition();

            if (this.rules.isValidMove(currentPos, diceValue, this.board.getBoardSize())) {
                const intermediatePos = currentPos + diceValue;
                const newPos = this.rules.calculateNewPosition(currentPos, diceValue, this.board);

                currentPlayer.setPosition(newPos);

                const entity = this.board.getElement(intermediatePos);

                if (entity !== null) {
                    const isSnake = entity.name() === "Snake";

                    if (isSnake) {
                        console.log(`Oh no! Snake at ${intermediatePos}! Going down to ${newPos}`);
                        this.notify(`${currentPlayer.getName()} encountered snake at ${intermediatePos} now going down to ${newPos}`);
                    } else {
                        console.log(`Great! Ladder at ${intermediatePos}! Going up to ${newPos}`);
                        this.notify(`${currentPlayer.getName()} encountered ladder at ${intermediatePos} now going up to ${newPos}`);
                    }
                }

                this.notify(`${currentPlayer.getName()} played. New Position : ${newPos}`);
                this.displayPlayerPositions();

                if (this.rules.checkWinCondition(newPos, this.board.getBoardSize())) {
                    console.log(`\n${currentPlayer.getName()} wins!`);
                    currentPlayer.incrementScore();
                    this.notify(`Game Ended. Winner is : ${currentPlayer.getName()}`);
                    this.gameOver = true;
                } else {
                    this.players.shift();
                    this.players.push(currentPlayer);
                }
            } else {
                console.log(`Need exact roll to reach ${this.board.getBoardSize()}!`);
                this.players.shift();
                this.players.push(currentPlayer);
            }
        }
    }
}

//Game Factor
class SnakeAndLadderGameFactory {

    static createStandardGame(): SnakeAndLadderGame {
        const board = new Board(10);
        const strategy = new StandardBoardSetupStrategy();
        board.setupBoard(strategy);

        const dice = new Dice(6);
        return new SnakeAndLadderGame(board, dice);
    }

    static createRandomGame(boardSize: number, difficulty: Difficulty): SnakeAndLadderGame {
        const board = new Board(boardSize);
        const strategy = new RandomBoardSetupStrategy(difficulty);
        board.setupBoard(strategy);

        const dice = new Dice(6);
        return new SnakeAndLadderGame(board, dice);
    }

    static createCustomGame(boardSize: number, strategy: BoardSetupStrategy): SnakeAndLadderGame {
        const board = new Board(boardSize);
        board.setupBoard(strategy);

        const dice = new Dice(6);
        return new SnakeAndLadderGame(board, dice);
    }
}

function main() {
    console.log("=== SNAKE AND LADDER GAME ===");

    let game: SnakeAndLadderGame | null = null;

    const choice = Number(prompt("Choose setup:\n1. Standard\n2. Random\n3. Custom"));

    if (choice === 1) {
        game = SnakeAndLadderGameFactory.createStandardGame();
    }
    else if (choice === 2) {
        const boardSize = Number(prompt("Enter board size:"));
        const diffChoice = Number(prompt("1. Easy\n2. Medium\n3. Hard"));

        let diff = Difficulty.MEDIUM;
        if (diffChoice === 1) diff = Difficulty.EASY;
        else if (diffChoice === 2) diff = Difficulty.MEDIUM;
        else if (diffChoice === 3) diff = Difficulty.HARD;

        game = SnakeAndLadderGameFactory.createRandomGame(boardSize, diff);
    }
    else if (choice === 3) {
        const boardSize = Number(prompt("Enter board size:"));
        const customChoice = Number(prompt("1. Counts\n2. Positions"));

        if (customChoice === 1) {
            const numSnakes = Number(prompt("Snakes:"));
            const numLadders = Number(prompt("Ladders:"));

            const strategy = new CustomCountBoardSetupStrategy(numSnakes, numLadders, true);
            game = SnakeAndLadderGameFactory.createCustomGame(boardSize, strategy);
        }
        else {
            const numSnakes = Number(prompt("Snakes:"));
            const numLadders = Number(prompt("Ladders:"));

            const strategy = new CustomCountBoardSetupStrategy(numSnakes, numLadders, false);

            for (let i = 0; i < numSnakes; i++) {
                const start = Number(prompt("Snake start:"));
                const end = Number(prompt("Snake end:"));
                strategy.addSnakePosition(start, end);
            }

            for (let i = 0; i < numLadders; i++) {
                const start = Number(prompt("Ladder start:"));
                const end = Number(prompt("Ladder end:"));
                strategy.addLadderPosition(start, end);
            }

            game = SnakeAndLadderGameFactory.createCustomGame(boardSize, strategy);
        }
    }

    if (!game) {
        console.log("Invalid choice!");
        return;
    }

    const notifier = new SnakeAndLadderConsoleNotifier();
    game.addObserver(notifier);

    const numPlayers = Number(prompt("Number of players:"));

    for (let i = 0; i < numPlayers; i++) {
        const name = prompt("Player name:")!;
        const player = new Player(i + 1, name);
        game.addPlayer(player);
    }

    game.play();
}

main();
