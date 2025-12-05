import { MenuItem } from "./menu-item";
import { Restaurant } from "./restaurant";

export class Cart {
  private restaurant: Restaurant | null;
  private items: MenuItem[];

  constructor() {
    this.restaurant = null;
    this.items = [];
  }

  getRestaurant(): Restaurant {
    return this.restaurant;
  }

  getItems(): MenuItem[] {
    return this.items;
  }

  addItem(item: MenuItem): void {
    if (!this.restaurant) {
      throw new Error("No restaurant associated with the cart.");
    }

    this.items.push(item);
  }

  getTotalPrice(): number {
    return this.items.reduce((total, item) => total + item.getPrice(), 0);
  }

  isEmpty(): boolean {
    return this.restaurant === null || this.items.length === 0;
  }

  removeItem(itemCode: string): void {
    this.items = this.items.filter((item) => item.getCode() !== itemCode);
  }

  clearCart(): void {
    this.items = [];
    this.restaurant = null;
  }

  setRestaurant(restaurant: Restaurant): void {
    this.restaurant = restaurant;
  }
}
