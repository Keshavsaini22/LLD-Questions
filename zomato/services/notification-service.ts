import { Order } from "../models/order";

export class NotificationService {
    sendOrderConfirmation(order: Order) {
        console.log(`Email sent to ${order.getUser().getName()}: Your ${order.getType()} is confirmed!`);
    }
}