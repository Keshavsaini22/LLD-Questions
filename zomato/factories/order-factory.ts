import { Cart } from "../models/cart";
import { MenuItem } from "../models/menu-item";
import { Order } from "../models/order";
import { Restaurant } from "../models/restaurant";
import { User } from "../models/user";
import { PaymentStrategy } from "../strategy/payment-strategy";

export abstract class OrderFactor {
    abstract createOrder(user: User,
        cart: Cart,
        restaurant: Restaurant,
        menuItems: MenuItem[],
        paymentStrategy: PaymentStrategy,
        totalCost: number,
        orderType: string): Order;
}