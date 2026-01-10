//Product and factory

class Product {
    constructor(private sku: number, private name: string, private price: number) { }

    getSku(): number {
        return this.sku;
    }
    getName(): string {
        return this.name;
    }
    getPrice(): number {
        return this.price;
    }
}

class ProductFactory {

    static createProduct(sku: number): Product {
        let name = `Item #${sku}`;
        let price = 100;

        switch (sku) {
            case 101:
                name = "Apple";
                price = 20;
                break;
            case 102:
                name = "Banana";
                price = 10;
                break;
            case 103:
                name = "Chocolate";
                price = 50;
                break;
            case 201:
                name = "T-Shirt";
                price = 500;
                break;
            case 202:
                name = "Jeans";
                price = 1000;
                break;
        }

        return new Product(sku, name, price);
    }
}


//inventory store
interface InventoryStore {
    addProduct(product: Product, quantity: number): void;
    removeProduct(sku: number, quantity: number): void;
    checkStock(sku: number): number;
    listAvailableProducts(): Product[];
}

class DbInventoryStore implements InventoryStore {
    private stock = new Map<number, number>();
    private products = new Map<number, Product>();

    addProduct(product: Product, quantity: number): void {
        const sku = product.getSku();
        if (!this.products.has(sku)) {
            this.products.set(sku, product);
        }
        this.stock.set(sku, (this.stock.get(sku) || 0) + quantity);
    }

    removeProduct(sku: number, qty: number): void {
        const current = this.stock.get(sku) || 0;
        const remaining = current - qty;

        if (remaining > 0) this.stock.set(sku, remaining);
        else this.stock.delete(sku);
    }

    checkStock(sku: number): number {
        return this.stock.get(sku) || 0;
    }

    listAvailableProducts(): Product[] {
        const result: Product[] = [];
        for (const [sku, qty] of this.stock) {
            if (qty > 0 && this.products.has(sku)) {
                result.push(this.products.get(sku)!);
            }
        }
        return result;
    }
}

class InventoryManager { //Not singleton because each manager will have its own store
    constructor(private store: InventoryStore) { }

    addStock(sku: number, quantity: number): void {
        const product = ProductFactory.createProduct(sku);
        this.store.addProduct(product, quantity);
    }

    removeStock(sku: number, quantity: number): void {
        this.store.removeProduct(sku, quantity);
    }

    checkStock(sku: number): number {
        return this.store.checkStock(sku);
    }

    getAvailableProducts(): Product[] {
        return this.store.listAvailableProducts();
    }
}

//Replenishment strategy
interface ReplenishmentStrategy {
    replenish(manager: InventoryManager, items: Map<number, number>): void;
}

class ThresholdReplenishmentStrategy implements ReplenishmentStrategy {
    constructor(private threshold: number) { }
    replenish(manager: InventoryManager, items: Map<number, number>): void {
        for (const [sku, qty] of items) {
            const currentStock = manager.checkStock(sku);
            if (currentStock < this.threshold) {
                manager.addStock(sku, qty);
                console.log(`Replenished ${qty} units of SKU ${sku}`);
            }
        }
    }
}

class WeeklyReplenishmentStrategy implements ReplenishmentStrategy {
    replenish(manager: InventoryManager, items: Map<number, number>): void {
        for (const [sku, qty] of items) {
            manager.addStock(sku, qty);
            console.log(`Weekly replenished ${qty} units of SKU ${sku}`);
        }
    }
}

//More replenishment strategies can be added here

//Darkstore
class Darkstore {
    private inventoryManager: InventoryManager = new InventoryManager(new DbInventoryStore());
    private replenishmentStrategy: ReplenishmentStrategy;

    constructor(private name: string, private x: number, private y: number) { }

    setReplenishmentStrategy(strategy: ReplenishmentStrategy): void {
        this.replenishmentStrategy = strategy;
    }

    distanceTo(x: number, y: number): number {
        return Math.sqrt(Math.pow(this.x - x, 2) + Math.pow(this.y - y, 2));
    }

    addStock(sku: number, quantity: number): void {
        this.inventoryManager.addStock(sku, quantity);
    }

    removeStock(sku: number, quantity: number): void {
        this.inventoryManager.removeStock(sku, quantity);
    }

    checkStock(sku: number): number {
        return this.inventoryManager.checkStock(sku);
    }

    getAllProducts(): Product[] {
        return this.inventoryManager.getAvailableProducts();
    }

    getName(): string {
        return this.name;
    }
}

//Darkstore manager (singleton)
class DarkstoreManager {
    private static instance: DarkstoreManager;
    private stores: Darkstore[] = [];
    private constructor() { }

    static getInstance(): DarkstoreManager {
        if (!this.instance) {
            this.instance = new DarkstoreManager();
        }
        return this.instance;
    }

    registerStore(store: Darkstore): void {
        this.stores.push(store);
    }

    getNearbyStores(x: number, y: number, maxDistance: number): Darkstore[] {
        return this.stores.map(ds => ({ ds, dist: ds.distanceTo(x, y) }))
            .filter(e => e.dist <= maxDistance)
            .sort((a, b) => a.dist - b.dist)
            .map(e => e.ds);
    }
}

//Cart and User
class CartItem {
    constructor(public product: Product, public quantity: number) { }
}

class Cart {
    private items: CartItem[] = [];

    addItem(sku: number, qty: number) {
        this.items.push(new CartItem(ProductFactory.createProduct(sku), qty));
    }

    getItems() {
        return this.items;
    }

    getTotal() {
        return this.items.reduce((total, item) => total + item.product.getPrice() * item.quantity, 0);
    }
}

class User {
    cart: Cart = new Cart();

    constructor(
        public name: string,
        public x: number,
        public y: number
    ) { }

    getCart(): Cart {
        return this.cart;
    }
}

//Order and order manager(singleton)
class DeliveryPartner {
    constructor(public name: string) { }
}

class Order {
    static nextId = 1;
    id = Order.nextId++;
    items: CartItem[] = [];
    partners: DeliveryPartner[] = [];
    total = 0;

    constructor(public user: User) { }
}

class OrderManager {
    private static instance: OrderManager
    private orders: Order[] = [];

    private constructor() { }

    static getInstance(): OrderManager {
        if (!this.instance) {
            this.instance = new OrderManager();
        }
        return this.instance;
    }

    placeOrder(user: User, cart: Cart): void {
        // const cartItems = user.cart.getItems();

        // const stores = DarkstoreManager.getInstance().getNearbyStores(user.x, user.y, 5); //max distance 5 units or 5km vale stores le ayo

        // if (stores.length === 0) {
        //     console.log("No nearby darkstores available.");
        //     return;
        // }

        // const order = new Order(user);

        // for (const item of cartItems) {
        //     let quantityNeeded = item.quantity;
        //     for (const store of stores) {
        //         if (quantityNeeded <= 0) break; //item fulfilled
        //         const availableStock = store.checkStock(item.product.getSku());

        //         const takenQty = Math.min(availableStock, quantityNeeded);

        //         if (takenQty > 0) {
        //             store.removeStock(item.product.getSku(), takenQty);
        //             order.items.push(new CartItem(item.product, takenQty));
        //             quantityNeeded -= takenQty;
        //             order.partners.push(new DeliveryPartner(`Partner from ${store.getName()}`));
        //         }
        //     }
        // }

        // order.total = order.items.reduce((sum, item) => sum + item.product.getPrice() * item.quantity, 0);
        // this.orders.push(order);
        // console.log(`Order #${order.id} placed successfully for user ${user.name}. Total: $${order.total}`);

        console.log(`\n[OrderManager] Placing Order for: ${user.name}`);

        const requestedItems = cart.getItems(); // {product, qty}[]

        // 1) Nearby stores
        const nearbyStores = DarkstoreManager.getInstance()
            .getNearbyStores(user.x, user.y, 5);

        if (nearbyStores.length === 0) {
            console.log("  No dark stores within 5 KM. Cannot fulfill order.");
            return;
        }

        // 2) Check first store has stock for all items
        const firstStore = nearbyStores[0];

        let allInFirst = true;
        for (const item of requestedItems) {
            const sku = item.product.getSku();
            const qty = item.quantity;

            if (firstStore.checkStock(sku) < qty) {
                allInFirst = false;
                break;
            }
        }

        const order = new Order(user);

        //Single store fulfillment
        if (allInFirst) {
            console.log(`  Fulfilling entire order from ${firstStore.getName()}`);
            for (const item of requestedItems) {
                const sku = item.product.getSku();
                const qty = item.quantity;
                firstStore.removeStock(sku, qty);
                order.items.push(item);
            }
            order.total = cart.getTotal();
            order.partners.push(new DeliveryPartner(`Partner from ${firstStore.getName()}`));
        }
        else {
            console.log(`  Fulfilling order from multiple stores.`);

            const allItems = new Map<number, number>();
            for (const item of requestedItems) {
                allItems.set(item.product.getSku(), item.quantity);
            }

            let partnerId = 1;

            for (const store of nearbyStores) {
                if (allItems.size === 0) break; //all items fulfilled

                console.log(`    Checking store: ${store.getName()}`);

                let assigned = false;

                const toErase: number[] = [];

                for (const [sku, qtyNeeded] of allItems.entries()) {
                    const availableStock = store.checkStock(sku);

                    if (availableStock <= 0) continue; //no stock

                    const takenQty = Math.min(availableStock, qtyNeeded);

                    store.removeStock(sku, takenQty);
                    console.log(` ${store.getName()} supplies SKU ${sku} x${takenQty}`);

                    order.items.push({
                        product: ProductFactory.createProduct(sku),
                        quantity: takenQty
                    });

                    if (qtyNeeded > takenQty) {
                        allItems.set(sku, qtyNeeded - takenQty);
                    } else {
                        toErase.push(sku);
                    }

                    assigned = true;
                }

                for (const sku of toErase) {
                    allItems.delete(sku);
                }

                if (assigned) {
                    const pname = `Partner${partnerId++}`;
                    order.partners.push(new DeliveryPartner(pname));
                    console.log(`     Assigned: ${pname} for ${store.getName()}`);

                }
            }

            if (allItems.size > 0) {
                console.log("  Could not fulfill entire order. Missing items:");
                for (const [sku, qty] of allItems.entries()) {
                    console.log(`   SKU ${sku} x${qty}`);
                }
            }

            //Calculate total
            let sum = 0;
            for (const item of order.items) {
                sum += item.product.getPrice() * item.quantity;
            }
            order.total = sum;
        }

        //SUMMARY
        console.log(`\n[OrderManager] Order #${order.id} Summary:`);
        console.log(`  User: ${user.name}\n  Items:`);

        for (const item of order.items) {
            console.log(`   - ${item.product.getName()} (SKU: ${item.product.getSku()}) x${item.quantity} = $${item.product.getPrice() * item.quantity}`);
        }

        console.log(`  Total Amount: $${order.total}`);
        console.log(`  Delivery Partners: ${order.partners.map(p => p.name).join(", ")}`);

        this.orders.push(order); //save order
    }

    getAllOrders(): Order[] {
        return this.orders;
    }
}

class ZeptoHelper {
    static showAllItems(user: User) {
        console.log(`\n[Zepto] All Available products within 5 KM for ${user.name}:`);

        const stores = DarkstoreManager.getInstance()
            .getNearbyStores(user.x, user.y, 5);

        const skuToPrice = new Map<number, number>();
        const skuToName = new Map<number, string>();

        // Aggregate products from all nearby stores
        for (const store of stores) {
            const products = store.getAllProducts();

            for (const product of products) {
                const sku = product.getSku();

                if (!skuToPrice.has(sku)) {
                    skuToPrice.set(sku, product.getPrice());
                    skuToName.set(sku, product.getName());
                }
            }
        }

        //Show aggregated products
        for (const [sku, price] of skuToPrice.entries()) {
            console.log(`  SKU ${sku} - ${skuToName.get(sku)} @ ₹${price}`);
        }

    }

    static initialize(): void {
        const manager = DarkstoreManager.getInstance();

        const A = new Darkstore("DarkStoreA", 0, 0);
        A.addStock(101, 5);
        A.addStock(102, 2);

        const B = new Darkstore("DarkStoreB", 4, 1);
        B.addStock(101, 3);
        B.addStock(103, 10);

        const C = new Darkstore("DarkStoreC", 2, 3);
        C.addStock(102, 5);
        C.addStock(201, 7);

        manager.registerStore(A);
        manager.registerStore(B);
        manager.registerStore(C);
    }
}

function main() {
    ZeptoHelper.initialize();

    const user = new User("Aditya", 1, 1);
    console.log(`\nUser with name ${user.name} comes on platform`);

    ZeptoHelper.showAllItems(user);

    console.log("\nAdding items to cart");

    const cart = user.getCart();
    cart.addItem(101, 4);
    cart.addItem(102, 3);
    cart.addItem(103, 2);

    OrderManager.getInstance().placeOrder(user, cart);
}

main();
