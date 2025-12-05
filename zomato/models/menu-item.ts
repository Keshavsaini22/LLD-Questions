export class MenuItem {
  private name: string;
  private price: number;
  private code: string;

  constructor(name: string, price: number, code: string) {
    this.name = name;
    this.price = price;
    this.code = code;
  }

  getName(): string {
    return this.name;
  }

  getPrice(): number {
    return this.price;
  }

  getCode(): string {
    return this.code;
  }

  setName(name: string): void {
    this.name = name;
  }

  setPrice(price: number): void {
    this.price = price;
  }

  setCode(code: string): void {
    this.code = code;
  }
}
