import { User } from "./models/user";
import { UpiPaymentStrategy } from "./strategy/upi-payment-strategy";
import { Order } from "./models/order";
import { TomatoApp } from "./tomato";

function main() {
    // Create TomatoApp Object
    const tomato = new TomatoApp();

    // Simulate a user coming in (Happy Flow)
    const user = new User("101", "Aditya", "Delhi");
    console.log(`User: ${user.getName()} is active.`);

    // User searches for restaurants by location
    const restaurantList = tomato.searchRestaurants("Delhi");

    if (restaurantList.length === 0) {
        console.log("No restaurants found!");
        return;
    }

    console.log("Found Restaurants:");
    for (const r of restaurantList) {
        console.log(" - " + r.getName());
    }

    // User selects a restaurant
    tomato.selectRestaurant(user, restaurantList[0]);

    console.log("Selected restaurant: " + restaurantList[0].getName());

    // User adds items to the cart
    tomato.addToCart(user, "P1");
    tomato.addToCart(user, "P2");

    tomato.printUserCart(user);

    // User checkout the cart
    const order: Order | null = tomato.checkoutNow(
        user,
        "Delivery",
        new UpiPaymentStrategy()
    );

    if (!order) {
        console.log("Cart is empty!");
        return;
    }

    // User pays for the order. If payment is success, notification is sent.
    tomato.payForOrder(user, order);
}

// Run the main function
main();
