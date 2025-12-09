import { Order } from "./order";

export class DeliveryOrder extends Order{
    private userAddress: string="";
    constructor(){
        super();
    }

    public getType(): string {
        return "DeliveryOrder";
    }

    public getUserAddress(): string {
        return this.userAddress;
    }

    public setUserAddress(address: string): void {
        this.userAddress = address;
    }

} 