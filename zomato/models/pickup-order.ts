import { Order } from "./order";

export class PickupOrder extends Order {
    private restaurantAddress: string = "";
    constructor() {
        super();
    }

    public getType(): string {
        return "Pickup";
    }

    public setRestaurantAddress(addr: string): void {
        this.restaurantAddress = addr;
    }

    public getRestaurantAddress(): string {
        return this.restaurantAddress;
    }
}