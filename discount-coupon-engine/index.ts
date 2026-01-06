//Strategy Pattern for Discount Calculation
interface DiscountStrategy {
    calculate(baseAmount: number): number;
}

class FlatDiscountStrategy implements DiscountStrategy {
    constructor(private amount: number) { }

    calculate(baseAmount: number): number {
        return Math.min(this.amount, baseAmount);
    }
}

class PercentageDiscountStrategy implements DiscountStrategy {
    constructor(private percentage: number) { }

    calculate(baseAmount: number): number {
        return baseAmount * (this.percentage / 100);
    }
}

class PercentageWithCapDiscountStrategy implements DiscountStrategy {

    constructor(private percentage: number, private cap: number) { }

    calculate(baseAmount: number): number {
        const discount = baseAmount * (this.percentage / 100);
        return Math.min(discount, this.cap);
    }
}

enum StrategyType {
    FLAT,
    PERCENTAGE,
    PERCENTAGE_WITH_CAP
}


//Discount Startegy Manager (Factory-Singleton)
class DiscountStrategyManager {
    private static instance: DiscountStrategyManager;
    private constructor() { }

    static getInstance(): DiscountStrategyManager {
        if (!this.instance) {
            this.instance = new DiscountStrategyManager();
        }
        return this.instance;
    }

    getStrategy(type: StrategyType, param1: number, param2: number = 0): DiscountStrategy {
        switch (type) {
            case StrategyType.FLAT:
                return new FlatDiscountStrategy(param1);
            case StrategyType.PERCENTAGE:
                return new PercentageDiscountStrategy(param1);
            case StrategyType.PERCENTAGE_WITH_CAP:
                return new PercentageWithCapDiscountStrategy(param1, param2);
            default:
                throw new Error("Invalid Strategy Type");
        }
    }
}

//Product and Cart on which discount is applied
class Product {
    constructor(
        private name: string,
        private category: string,
        private price: number
    ) { }

    getName() {
        return this.name;
    }

    getCategory() {
        return this.category;
    }

    getPrice() {
        return this.price;
    }
}

class CartItem {
    constructor(private product: Product, private quantity: number) { }

    itemTotal(): number {
        return this.product.getPrice() * this.quantity;
    }

    getProduct(): Product {
        return this.product;
    }
}

class Cart {
    private items: CartItem[] = [];
    private originalTotal: number = 0;
    private currentTotal: number = 0;
    private loyaltyCustomer: boolean = false;
    private paymentBank: string = "";

    addProduct(product: Product, quantity: number = 1): void {
        const item = new CartItem(product, quantity);
        this.items.push(item);
        this.originalTotal += item.itemTotal();
        this.currentTotal += item.itemTotal();
    }

    applyDiscount(amount: number): void {
        this.currentTotal = Math.max(0, this.currentTotal - amount);
    }

    getOriginalTotal(): number {
        return this.originalTotal;
    }

    getCurrentTotal(): number {
        return this.currentTotal;
    }

    getItems(): CartItem[] {
        return this.items;
    }

    setLoyaltyCustomer(isLoyal: boolean): void {
        this.loyaltyCustomer = isLoyal;
    }

    isLoyaltyCustomer(): boolean {
        return this.loyaltyCustomer;
    }

    setPaymentBank(bank: string): void {
        this.paymentBank = bank;
    }

    getPaymentBank(): string {
        return this.paymentBank;
    }
}


//Coupon(Chain of Responsibility Pattern)

abstract class Coupon {
    private next?: Coupon;

    setNext(coupon: Coupon): void {
        this.next = coupon;
    }

    apply(cart: Cart): void {
        if (this.isApplicable(cart)) {
            const discount = this.getDiscount(cart);
            cart.applyDiscount(discount);
            console.log(`Applied ${this.name()} coupon for discount: $${discount.toFixed(2)}`);

            if (!this.isCombinable()) {
                return; // Stop further processing if not combinable
            }
        }
        this.next?.apply(cart); // Pass to next coupon in the chain

    }

    protected abstract isApplicable(cart: Cart): boolean;
    protected abstract getDiscount(cart: Cart): number;
    protected abstract name(): string;
    protected isCombinable(): boolean {
        return true;
    }
}

//Concrete Coupons
class SeasonalCoupon extends Coupon { //coupon applicable to particular products not whole cart
    private strategy: DiscountStrategy;

    constructor(private percent: number, private category: string) {
        super();
        this.strategy = DiscountStrategyManager.getInstance()
            .getStrategy(StrategyType.PERCENTAGE, percent);
    }

    protected isApplicable(cart: Cart): boolean {
        return cart.getItems().some(item => item.getProduct().getCategory() === this.category);
    }

    protected getDiscount(cart: Cart): number {
        const subtotal = cart.getItems()
            .filter(i => i.getProduct().getCategory() === this.category)
            .reduce((sum, i) => sum + i.itemTotal(), 0);

        return this.strategy.calculate(subtotal);
    }

    protected name(): string {
        return `Seasonal Offer ${this.percent}% off ${this.category}`;
    }

}

class LoyaltyCoupon extends Coupon { //coupon applicable to loyalty customers on whole cart
    private strategy: DiscountStrategy;

    constructor(private percent: number) {
        super();
        this.strategy = DiscountStrategyManager.getInstance()
            .getStrategy(StrategyType.PERCENTAGE, percent);
    }

    protected isApplicable(cart: Cart): boolean {
        return cart.isLoyaltyCustomer();
    }

    protected getDiscount(cart: Cart): number {
        return this.strategy.calculate(cart.getCurrentTotal());
    }

    protected name(): string {
        return `Loyalty Discount ${this.percent}%`;
    }
}

class BulkPurchaseCoupon extends Coupon { //coupon applicable on whole cart if total exceeds certain threshold
    private strategy: DiscountStrategy;

    constructor(private threshold: number, private flatOff: number) {
        super();
        this.strategy = DiscountStrategyManager.getInstance()
            .getStrategy(StrategyType.FLAT, flatOff);
    }

    protected isApplicable(cart: Cart): boolean {
        return cart.getOriginalTotal() >= this.threshold;
    }

    protected getDiscount(cart: Cart): number {
        return this.strategy.calculate(cart.getCurrentTotal());
    }

    protected name(): string {
        return `Bulk Purchase ₹${this.flatOff} off over ₹${this.threshold}`;
    }
}

class BankingCoupon extends Coupon {
    private strategy: DiscountStrategy;

    constructor(
        private bank: string,
        private minSpend: number,
        private percent: number,
        private cap: number
    ) {
        super();
        this.strategy = DiscountStrategyManager.getInstance()
            .getStrategy(StrategyType.PERCENTAGE_WITH_CAP, percent, cap);
    }

    protected isApplicable(cart: Cart): boolean {
        return (
            cart.getPaymentBank() === this.bank &&
            cart.getOriginalTotal() >= this.minSpend
        );
    }

    protected getDiscount(cart: Cart): number {
        return this.strategy.calculate(cart.getCurrentTotal());
    }

    protected name(): string {
        return `${this.bank} Bank ${this.percent}% off upto ₹${this.cap}`;
    }
}

//Coupon manager (singleton)
class CouponManager {
    private static instance: CouponManager;
    private head?: Coupon;
    private constructor() { }

    static getInstance(): CouponManager {
        if (!this.instance) {
            this.instance = new CouponManager();
        }
        return this.instance;
    }

    registerCoupon(coupon: Coupon): void {
        if (!this.head) {
            this.head = coupon;
            return;
        }

        let current = this.head;
        while ((current as any).next) {
            current = (current as any).next;
        }
        current.setNext(coupon);
    }

    applyAll(cart: Cart): number {
        this.head?.apply(cart);
        return cart.getCurrentTotal();
    }
}


//client code 

const mgr = CouponManager.getInstance();

mgr.registerCoupon(new SeasonalCoupon(10, "Clothing"));
mgr.registerCoupon(new LoyaltyCoupon(5));
mgr.registerCoupon(new BulkPurchaseCoupon(1000, 100));
mgr.registerCoupon(new BankingCoupon("ABC", 2000, 15, 500));

const p1 = new Product("Winter Jacket", "Clothing", 1000);
const p2 = new Product("Smartphone", "Electronics", 20000);
const p3 = new Product("Jeans", "Clothing", 1000);
const p4 = new Product("Headphones", "Electronics", 2000);

const cart = new Cart();
cart.addProduct(p1, 1);
cart.addProduct(p2, 1);
cart.addProduct(p3, 2);
cart.addProduct(p4, 1);


cart.setLoyaltyCustomer(true);
cart.setPaymentBank("ABC");

console.log("Original Total:", cart.getOriginalTotal());
console.log("Final Total:", mgr.applyAll(cart));