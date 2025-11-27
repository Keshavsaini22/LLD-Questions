//ENTITIES

//PLAYER
//BOARD
//GAME
//SYMBOL
//RULE

//PATTERNS TO IMPLEMENT
//FACTORY - create players, boards, games
//STRATEGY - different winning strategies
//OBSERVER - notify players of game state changes

// Represents a cell marked by a player's symbol
export class SymbolCell {
  private mark: string;

  constructor(mark: string) {
    this.mark = mark;
  }

  getMark(): string {
    return this.mark;
  }
}

export type Cell = SymbolCell | null;

export type Pos = { row: number; col: number };

export enum GameStatus {
  ONGOING = "ONGOING",
  DRAW = "DRAW",
  WIN = "WIN",
}

export enum MoveResultType {
  OK = "OK",
  INVALID = "INVALID",
  WIN = "WIN",
  DRAW = "DRAW",
}

// Represents a player in the game
export class Player {
  private name: string;
  private symbol: SymbolCell;
  private id: number;

  constructor(name: string, symbol: SymbolCell, id: number) {
    this.name = name;
    this.symbol = symbol;
    this.id = id;
  }

  getName(): string {
    return this.name;
  }

  getSymbol(): SymbolCell {
    return this.symbol;
  }

  getId(): number {
    return this.id;
  }
}

//Observer Pattern
export interface NotificationEvent {
  type: "move" | "win" | "draw" | "invalid";
  payload: any;
}

export interface IObserver {
  update(event: NotificationEvent): void;
}

//One of the subscribers
export class ConsoleNotifier implements IObserver {
  update(event: NotificationEvent): void {
    switch (event.type) {
      case "move":
        console.log(
          `[move] ${event.payload.player.name} placed ${event.payload.symbol.getMark()} at (${event.payload.pos.row},${
            event.payload.pos.col
          })`
        );
        break;

      case "win":
        console.log(`[win] ${event.payload.player.name} won!`);
        break;

      case "draw":
        console.log(`[draw] It's a draw!`);
        break;

      case "invalid":
        console.log(`[invalid] Invalid move by ${event.payload.player.name}`);
        break;
    }
  }
}

// Represents the game board
export class Board {
  private grid: Cell[][];
  private size: number;

  constructor(size: number) {
    this.size = size;
    this.grid = Array.from({ length: size }, () => Array(size).fill(null));
  }

  isInBounds(pos: Pos): boolean {
    return pos.row >= 0 && pos.row < this.size && pos.col >= 0 && pos.col < this.size;
  }

  getCell(pos: Pos): Cell {
    if (!this.isInBounds(pos)) {
      return null; //Here I can show notification that position is out of bounds
    }

    return this.grid[pos.row][pos.col];
  }

  setCell(pos: Pos, symbol: SymbolCell): boolean {
    if (!this.isInBounds(pos)) {
      return false; //Here I can show notification that position is out of bounds
    }

    this.grid[pos.row][pos.col] = symbol;
    return true;
  }

  isFull(): boolean {
    return this.grid.every((row) => row.every((cell) => cell !== null));
  }

  prettyPrint(): void {
    console.log(this.grid.map((row) => row.map((cell) => (cell ? cell.getMark() : ".")).join(" ")).join("\n"));
  }
}

//Strategy Pattern - Represents the Rules
export interface IRule {
  evaluateMove(board: Board, pos: Pos, player: Player): { result: MoveResultType; winner?: Player };
}

// Implements the standard Tic-Tac-Toe winning rule
export class StandardRule implements IRule {
  private winLength: number;

  constructor(winLength: number = 3) {
    this.winLength = winLength;
  }

  private countDirection(board: Board, start: Pos, dr: number, dc: number, mark: string): number {
    let r = start.row + dr;
    let c = start.col + dc;
    let cnt = 0;

    while (board.isInBounds({ row: r, col: c })) {
      const cell = board.getCell({ row: r, col: c });

      if (!cell || cell.getMark() !== mark) break;
      cnt++;
      r += dr;
      c += dc;
    }

    return cnt;
  }

  evaluateMove(board: Board, pos: Pos, player: Player): { result: MoveResultType; winner?: Player } {
    const boardSymbol = board.getCell(pos);

    if (!board.isInBounds(pos)) {
      //Cell is out of bounds
      return { result: MoveResultType.INVALID };
    }

    if (!boardSymbol || boardSymbol.getMark() !== player.getSymbol().getMark()) {
      //Cell is empty or does not match player's symbol
      return { result: MoveResultType.INVALID };
    }

    const directions = [
      { dr: 0, dc: 1 }, // Horizontal
      { dr: 1, dc: 0 }, // Vertical
      { dr: 1, dc: 1 }, // Diagonal \
      { dr: 1, dc: -1 }, // Diagonal /
    ];

    const targetMark = player.getSymbol().getMark();

    for (const d of directions) {
      let count = 1; // Count the current cell

      count += this.countDirection(board, pos, d.dr, d.dc, targetMark); // Count in the positive direction
      count += this.countDirection(board, pos, -d.dr, -d.dc, targetMark); // Count in the negative direction

      if (count >= this.winLength) {
        return { result: MoveResultType.WIN, winner: player };
      }
    }

    if (board.isFull()) {
      return { result: MoveResultType.DRAW };
    }

    return { result: MoveResultType.OK };
  }
}

//Game and Factory Pattern
export class Game {
  public board: Board;
  public players: Player[];
  public rule: IRule;
  private observers: IObserver[] = [];
  public status: GameStatus = GameStatus.ONGOING;
  private currentPlayerIndex: number = 0;

  constructor(board: Board, players: Player[], rule: IRule) {
    if (players.length < 2) {
      throw new Error("At least two players are required to start the game.");
    }

    this.board = board;
    this.players = players;
    this.rule = rule;
  }

  addObserver(observer: IObserver): void {
    this.observers.push(observer);
  }

  removeObserver(observer: IObserver): void {
    this.observers = this.observers.filter((obs) => obs !== observer);
  }

  notifyObservers(event: NotificationEvent): void {
    for (const observer of this.observers) {
      observer.update(event);
    }
  }

  getCurrentPlayer(): Player {
    return this.players[this.currentPlayerIndex];
  }

  makeMove(player: Player, pos: Pos): { result: MoveResultType; winner?: Player | null } {
    if (this.status !== GameStatus.ONGOING) {
      const reason = `Game already finished: ${this.status}`;
      this.notifyObservers({ type: "invalid", payload: { reason } });

      return { result: MoveResultType.INVALID };
    }

    if (player !== this.getCurrentPlayer()) {
      const reason = `It's not ${player.getName()}'s turn.`;
      this.notifyObservers({ type: "invalid", payload: { player, reason } });

      return { result: MoveResultType.INVALID };
    }

    if (!this.board.isInBounds(pos)) {
      const reason = `Position (${pos.row}, ${pos.col}) is out of bounds.`;
      this.notifyObservers({ type: "invalid", payload: { player, reason } });

      return { result: MoveResultType.INVALID };
    }

    if (this.board.getCell(pos) !== null) {
      const reason = `Cell at (${pos.row}, ${pos.col}) is already occupied.`;
      this.notifyObservers({ type: "invalid", payload: { player, reason } });

      return { result: MoveResultType.INVALID };
    }

    const setOk = this.board.setCell(pos, player.getSymbol());

    if (!setOk) {
      const reason = `Failed to set cell at (${pos.row}, ${pos.col}).`;
      this.notifyObservers({ type: "invalid", payload: { player, reason } });
      return { result: MoveResultType.INVALID };
    }

    this.notifyObservers({ type: "move", payload: { player, pos, symbol: player.getSymbol() } });

    const evaluation = this.rule.evaluateMove(this.board, pos, player);

    if (evaluation.result === MoveResultType.WIN) {
      this.status = GameStatus.WIN;
      this.notifyObservers({ type: "win", payload: { player, symbol: player.getSymbol() } });
    } else if (evaluation.result === MoveResultType.DRAW) {
      this.status = GameStatus.DRAW;
      this.notifyObservers({ type: "draw", payload: {} });
    } else {
      // Switch to the next player
      this.currentPlayerIndex = (this.currentPlayerIndex + 1) % this.players.length;
    }

    return { result: evaluation.result, winner: evaluation.winner ?? null };
  }
}

//Factory to create a standard Tic-Tac-Toe game
export enum GameType {
  STANDARD = "STANDARD",
}

// FACTORY PATTERN - GameFactory
export class GameFactory {
  static createGame(type: GameType, size: number = 3): Game {
    switch (type) {
      case GameType.STANDARD: {
        const board = new Board(size);

        const player1 = new Player("Player1", new SymbolCell("X"), 1);
        const player2 = new Player("Player2", new SymbolCell("O"), 2);

        const rule = new StandardRule(3);

        const game = new Game(board, [player1, player2], rule);

        // Add console observer by default
        game.addObserver(new ConsoleNotifier());

        return game;
      }

      default:
        throw new Error("Unsupported game type");
    }
  }
}

// CLIENT CLASS
export class GameClient {
  private game: Game;

  constructor() {
    // Create a 3×3 standard game using factory
    this.game = GameFactory.createGame(GameType.STANDARD, 3);
  }

  play() {
    console.log("🎮 Starting Tic-Tac-Toe Game...\n");
    this.game.board.prettyPrint();
    console.log("");

    const moves: Pos[] = [
      { row: 0, col: 0 },
      { row: 0, col: 1 },
      { row: 1, col: 1 },
      { row: 0, col: 2 },
      { row: 2, col: 2 }, // Win
    ];

    for (const move of moves) {
      const current = this.game.getCurrentPlayer();
      console.log(`→ ${current.getName()} attempts move at (${move.row}, ${move.col})`);

      const result = this.game.makeMove(current, move);

      this.game.board.prettyPrint();
      console.log("");

      if (result.result === MoveResultType.WIN) {
        console.log(`🏆 Game Over — ${result.winner!.getName()} Wins!`);
        break;
      }

      if (result.result === MoveResultType.DRAW) {
        console.log("🤝 Game is a Draw!");
        break;
      }
    }
  }
}


// MAIN FUNCTION
export function main() {
  const client = new GameClient();
  client.play();
}

main();
