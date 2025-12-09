import { Cart } from "../models/cart";
import { DeliveryOrder } from "../models/delivery-order";
import { MenuItem } from "../models/menu-item";
import { Order } from "../models/order";
import { PickupOrder } from "../models/pickup-order";
import { Restaurant } from "../models/restaurant";
import { User } from "../models/user";
import { PaymentStrategy } from "../strategy/payment-strategy";
import { OrderFactor } from "./order-factory";

export class NowOrderFactory extends OrderFactor {
    createOrder(user: User,
        cart: Cart,
        restaurant: Restaurant,
        menuItems: MenuItem[],
        paymentStrategy: PaymentStrategy,
        totalCost: number,
        orderType: string): Order {
        let order = null;

        if (orderType === "Delivery") {
            order = new DeliveryOrder();
            order.setUserAddress(user.getAddress());
        } else {
            order = new PickupOrder();
            order.setRestaurantAddress(restaurant.getLocation());
        }

        order.setUser(user);
        order.setRestaurant(restaurant);
        order.setItems(menuItems);
        order.setPaymentStrategy(paymentStrategy);
        order.setTotal(totalCost);
        return order;
    }
}