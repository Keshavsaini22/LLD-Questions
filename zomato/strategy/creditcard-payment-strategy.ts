import { PaymentStrategy } from "./payment-strategy";

export class CreditCardPaymentStrategy implements PaymentStrategy {
    pay(amount: number): void {
        console.log(`Paid ₹${amount} using Credit Card.`);
    }
}