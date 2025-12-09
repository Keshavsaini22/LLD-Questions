import { Order } from "../models/order";

export class OrderManager {
    private orders: Order[];
    private static instance: OrderManager | null = null;

    private constructor() { }

    public static getInstance(): OrderManager {
        if (!OrderManager.instance) {
            OrderManager.instance = new OrderManager();
        }

        return OrderManager.instance;
    }

    public addOrder(order: Order): void {
        this.orders.push(order);
    }

    public listOrders(): Order[] {
        return this.orders;
    }
}