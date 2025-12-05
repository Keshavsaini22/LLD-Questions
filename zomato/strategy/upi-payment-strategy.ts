import { PaymentStrategy } from "./payment-strategy";

export class UpiPaymentStrategy implements PaymentStrategy {
  pay(amount: number): void {
    console.log(`Paid ₹${amount} using UPI.`);
  }
}
