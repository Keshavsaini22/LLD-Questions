import { Cart } from "./cart";

export class User{
    private id: string;
    private name: string;
    private address: string;
    private cart:Cart;

    constructor(id: string, name: string, address: string){
        this.id = id;
        this.name = name;
        this.address = address;
        this.cart = new Cart();
    }

    public getId(): string {
        return this.id;
    }

    public getName(): string {
        return this.name;
    }

    public getAddress(): string {
        return this.address;
    }

    public getCart(): Cart {
        return this.cart;
    }

    setAddress(address: string): void {
        this.address = address;
    }

    setCart(cart: Cart): void {
        this.cart = cart;
    }

    setName(name: string): void {
        this.name = name;
    }

    
}