export type Pos = {
    row: number;
    col: number;
}

export enum GameStatus {
    ONGOING, WIN, DRAW,
}

export class Player {
    constructor(private id: number, private symbol: string) { }

    getId(): number {
        return this.id;
    }

    getSymbol() {
        return this.symbol;
    }
}

export class Board {
    private grid: (string | null)[][];

    constructor(private size: number) {
        this.grid = Array.from({ length: size }, () => Array(size).fill(null));
    }

    getSize() {
        return this.size;
    }

    isValid(pos: Pos) {
        return pos.row >= 0 &&
            pos.row < this.size &&
            pos.col >= 0 && pos.col < this.size;
    }

    get(pos: Pos) {
        return this.grid[pos.row][pos.col];
    }

    set(pos: Pos, symbol: string) {
        this.grid[pos.row][pos.col] = symbol;
    }

    isFull() {
        return this.grid.every(row => row.every(cell => cell !== null));
    }

    print() {
        console.log(
            this.grid.map(row => row.map(cell => cell ?? '.').join(' ')).join('\n')
        );
    }
}

//========= Strategy pattern : Rule ==========
export interface Rule {
    checkWin(board: Board, last: Pos, symbol: string): boolean;
}

export class StandardRule implements Rule {
    constructor(private winLength: number = 3) { }

    private count(board: Board, start: Pos, dR: number, dC: number, symbol: string) {
        let r = start.row + dR;
        let c = start.col + dC;
        let count = 0;

        while (board.isValid({ row: r, col: c }) && board.get({ row: r, col: c }) === symbol) {
            count++;
            r += dR;
            c += dC;
        }

        return count;
    }

    checkWin(board: Board, last: Pos, symbol: string): boolean {
        const directions = [[0, 1], [1, 0], [1, 1], [1, -1]]; // right, down, down-right, down-left

        for (const [dR, dC] of directions) {
            const count = 1 + this.count(board, last, dR, dC, symbol) + this.count(board, last, -dR, -dC, symbol);
            if (count >= this.winLength) {
                return true;
            }
        }

        return false;

    }
}

export class Game {
    private current = 0;

    private status: GameStatus = GameStatus.ONGOING;

    constructor(private board: Board,
        private players: Player[],
        private rule: Rule
    ) { }

    getStatus() {
        return this.status;
    }

    getCurrentPlayer() {
        return this.players[this.current];
    }

    makeMove(pos: Pos): GameStatus {
        if (this.status !== GameStatus.ONGOING) {
            return this.status;
        }

        const player = this.getCurrentPlayer();
        const symbol = player.getSymbol();

        if (!this.board.isValid(pos) || this.board.get(pos) !== null) {
            throw new Error('Invalid move');
        }

        this.board.set(pos, symbol);

        if (this.rule.checkWin(this.board, pos, symbol)) {
            this.status = GameStatus.WIN;
            return this.status;
        }

        if (this.board.isFull()) {
            this.status = GameStatus.DRAW;
            return this.status;
        }

        this.current = (this.current + 1) % this.players.length;
        return this.status;
    }

    printBoard() {
        this.board.print();
    }
}

function main() {
    const board = new Board(3);
    const players = [new Player(1, 'X'), new Player(2, 'O')];
    const rule = new StandardRule(3);
    const game = new Game(board, players, rule);

    const moves: Pos[] = [
        { row: 0, col: 0 },
        { row: 0, col: 1 },
        { row: 1, col: 1 },
        { row: 0, col: 2 },
        { row: 2, col: 2 },
    ];

    for (const move of moves) {
        const status = game.makeMove(move);
        game.printBoard();
        console.log('===============================================');

        if (status === GameStatus.WIN) {
            console.log(`Player ${game.getCurrentPlayer().getId()} wins!`);
            break;
        }
        if (status === GameStatus.DRAW) {
            console.log('Game is a draw!');
            break;
        }

    }
}

main()