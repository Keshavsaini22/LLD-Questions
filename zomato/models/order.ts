import { PaymentStrategy } from "../strategy/payment-strategy";
import { MenuItem } from "./menu-item";
import { Restaurant } from "./restaurant";
import { User } from "./user";

export abstract class Order {
  private static nextOrderId = 0;

  protected orderId: number;
  protected user: User | null = null;
  protected restaurant: Restaurant | null = null;
  protected items: MenuItem[] = [];
  protected paymentStrategy: PaymentStrategy | null = null;
  protected total: number = 0;
  protected scheduled: string = "";

  constructor() {
    this.orderId = ++Order.nextOrderId;
  }

  public processPayment(): boolean {
    if (this.paymentStrategy) {
      this.paymentStrategy.pay(this.total);
      return true;
    } else {
      console.log("Please choose a payment mode first");
      return false;
    }
  }

  public abstract getType(): string;

  public getOrderId(): number {
    return this.orderId;
  }

  public setUser(user: User): void {
    this.user = user;
  }

  public setRestaurant(restaurant: Restaurant): void {
    this.restaurant = restaurant;
  }

  public getTotal(): number {
    return this.total;
  }

  public getScheduled(): string {
    return this.scheduled;
  }

  public setScheduled(scheduled: string): void {
    this.scheduled = scheduled;
  }

  public getUser(): User | null {
    return this.user;
  }

  public setPaymentStrategy(paymentStrategy: PaymentStrategy): void {
    this.paymentStrategy = paymentStrategy;
  }

  public getRestaurant(): Restaurant | null {
    return this.restaurant;
  }

  public getPaymentStrategy(): PaymentStrategy | null {
    return this.paymentStrategy;
  }

  public getItems(): MenuItem[] {
    return this.items;
  }

  public setItems(items: MenuItem[]): void {
    this.items = items;
    this.total = 0;
    for (const item of this.items) {
      this.total += item.getPrice();
    }
  }
}
