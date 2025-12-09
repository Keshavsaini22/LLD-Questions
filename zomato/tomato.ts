import { NowOrderFactory } from "./factories/now-order-factory";
import { ScheduledOrderFactory } from "./factories/schedule-order-factory";
import { OrderManager } from "./managers/order-manager";
import { RestaurantManager } from "./managers/restaurant-manager";
import { MenuItem } from "./models/menu-item";
import { Order } from "./models/order";
import { Restaurant } from "./models/restaurant";
import { User } from "./models/user";
import { NotificationService } from "./services/notification-service";
import { PaymentStrategy } from "./strategy/payment-strategy";

export class TomatoApp {
    constructor() {
        this.initializeRestaurants();
    }

    private initializeRestaurants(): void {
        const restaurant1 = new Restaurant("Bikaner", "Delhi");
        restaurant1.setMenu(new MenuItem("Chole Bhature", 120, "P1",));
        restaurant1.setMenu(new MenuItem("Paneer Tikka", 150, "P2",));
        restaurant1.setMenu(new MenuItem("Lassi", 60, "P3",));

        const restaurant2 = new Restaurant("Haldiram", "Kolkata");
        restaurant2.setMenu(new MenuItem("Aloo Chaat", 80, "P1",));
        restaurant2.setMenu(new MenuItem("Rajma Chawal", 110, "P2",));
        restaurant2.setMenu(new MenuItem("Jalebi", 70, "P3",));

        const restaurant3 = new Restaurant("Saravana Bhavan", "Chennai");
        restaurant3.setMenu(new MenuItem("Masala Dosa", 90, "P1",));
        restaurant3.setMenu(new MenuItem("Idli Sambar", 70, "P2",));
        restaurant3.setMenu(new MenuItem("Filter Coffee", 50, "P3",));

        const restaurantManager = RestaurantManager.getInstance();
        restaurantManager.addRestaurant(restaurant1);
        restaurantManager.addRestaurant(restaurant2);
        restaurantManager.addRestaurant(restaurant3);
    }

    public searchRestaurants(location: string): Restaurant[] {
        return RestaurantManager.getInstance().searchRestaurantByLocation(location);
    }

    public selectRestaurant(user: User, restaurant: Restaurant): void {
        const cart = user.getCart();
        cart.setRestaurant(restaurant);
    }

    public addToCart(user: User, itemCode: string): void {
        const cart = user.getCart();
        const restaurant = cart.getRestaurant();

        if (!restaurant) {
            console.log("Please select a restaurant first.");
            return;
        }

        for (const item of restaurant.getMenus()) {
            if (item.getCode() === itemCode) {
                cart.addItem(item);
                return;
            }
        }
    }

    public checkoutNow(
        user: User,
        orderType: string,
        paymentStrategy: PaymentStrategy
    ): Order | null {
        return this.checkout(user, orderType, paymentStrategy, new NowOrderFactory());
    }

    public checkoutScheduled(
        user: User,
        orderType: string,
        paymentStrategy: PaymentStrategy,
        scheduleTime: string
    ): Order | null {
        return this.checkout(
            user,
            orderType,
            paymentStrategy,
            new ScheduledOrderFactory(scheduleTime)
        );
    }

    private checkout(
        user: User,
        orderType: string,
        paymentStrategy: PaymentStrategy,
        orderFactory: any
    ): Order | null {
        const cart = user.getCart();

        if (cart.isEmpty()) return null;

        const restaurant = cart.getRestaurant()!;
        const items = cart.getItems();
        const totalCost = cart.getTotalPrice();

        const order: Order = orderFactory.createOrder(
            user,
            cart,
            restaurant,
            items,
            paymentStrategy,
            totalCost,
            orderType
        );

        OrderManager.getInstance().addOrder(order);

        return order;
    }

    public payForOrder(user: User, order: Order): void {
        const isPaymentSuccess = order.processPayment();

        if (isPaymentSuccess) {
            const notification = new NotificationService();
            notification.sendOrderConfirmation(order);
            user.getCart().clearCart();
        }
    }

    public printUserCart(user: User): void {
        console.log("Items in cart:");
        console.log("------------------------------------");

        for (const item of user.getCart().getItems()) {
            console.log(`${item.getCode()} : ${item.getName()} : ₹${item.getPrice()}`);
        }

        console.log("------------------------------------");
        console.log(`Grand total : ₹${user.getCart().getTotalPrice()}`);
    }

}