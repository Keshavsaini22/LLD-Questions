#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <unordered_set>

using namespace std;

// =========================================
// PRODUCT,
// =========================================

class Product
{
private:
    int id;
    string name;
    double price;

public:
    Product(int id,
            string name,
            double price)
        : id(id),
          name(name),
          price(price) {}

    int getId() const
    {
        return id;
    }

    string getName() const
    {
        return name;
    }

    double getPrice() const
    {
        return price;
    }
};

// =========================================
// INVENTORY ITEM
// =========================================

class InventoryItem
{

private:
    shared_ptr<Product> product;

    int totalQuantity;

    int reservedQuantity;

    mutable mutex inventoryMutex;

public:
    InventoryItem(shared_ptr<Product> product,
                  int quantity)
        : product(product),
          totalQuantity(quantity),
          reservedQuantity(0) {}

    shared_ptr<Product> getProduct() const
    {
        return product;
    }

    int getAvailableQuantity() const
    {
        return totalQuantity - reservedQuantity;
    }

    int getTotalQuantity() const
    {
        return totalQuantity;
    }

    bool reserve(int qty)
    {

        lock_guard<mutex> lock(inventoryMutex);

        if (qty > getAvailableQuantity())
            return false;

        reservedQuantity += qty;
        return true;
    }

    void release(int qty)
    {

        lock_guard<mutex> lock(inventoryMutex);

        reservedQuantity -= qty;

        if (reservedQuantity < 0)
            reservedQuantity = 0;
    }

    bool confirmReservation(int qty)
    {

        lock_guard<mutex> lock(inventoryMutex);

        if (qty > reservedQuantity)
            return false;

        reservedQuantity -= qty;
        totalQuantity -= qty;

        return true;
    }

    void addStock(int qty)
    {

        lock_guard<mutex> lock(inventoryMutex);

        totalQuantity += qty;
    }
};

// =========================================
// INVENTORY STORE
// =========================================

class InventoryStore
{

public:
    virtual ~InventoryStore() = default;

    virtual void addProduct(
        shared_ptr<Product> product,
        int quantity) = 0;

    virtual shared_ptr<InventoryItem>
    getInventory(int productId) = 0;

    virtual vector<shared_ptr<InventoryItem>>
    getAllInventory() = 0;
};

// =========================================
// IN MEMORY INVENTORY STORE
// =========================================

class InMemoryInventoryStore
    : public InventoryStore
{

private:
    unordered_map<
        int,
        shared_ptr<InventoryItem>>
        inventoryMap;

public:
    void addProduct(
        shared_ptr<Product> product,
        int quantity) override
    {

        int id = product->getId();

        if (inventoryMap.count(id))
        {

            inventoryMap[id]->addStock(quantity);
        }
        else
        {

            inventoryMap[id] =
                make_shared<InventoryItem>(
                    product,
                    quantity);
        }
    }

    shared_ptr<InventoryItem>
    getInventory(
        int productId) override
    {

        if (!inventoryMap.count(productId))
            return nullptr;

        return inventoryMap[productId];
    }

    vector<shared_ptr<InventoryItem>>
    getAllInventory() override
    {

        vector<shared_ptr<InventoryItem>> result;

        for (auto &entry : inventoryMap)
            result.push_back(entry.second);

        return result;
    }
};

// =========================================
// REPLENISHMENT STRATEGY
// =========================================

class ReplenishmentStrategy
{

public:
    virtual ~ReplenishmentStrategy() = default;

    virtual int getReplenishmentQty(
        int currentStock) = 0;
};

class ThresholdReplenishmentStrategy
    : public ReplenishmentStrategy
{

private:
    int threshold;
    int refillAmount;

public:
    ThresholdReplenishmentStrategy(
        int threshold,
        int refillAmount)
        : threshold(threshold),
          refillAmount(refillAmount) {}

    int getReplenishmentQty(
        int currentStock) override
    {

        if (currentStock < threshold)
            return refillAmount;

        return 0;
    }
};

// =========================================
// INVENTORY MANAGER
// =========================================

class InventoryManager
{

private:
    shared_ptr<InventoryStore> inventoryStore;

public:
    InventoryManager(
        shared_ptr<InventoryStore> inventoryStore)
        : inventoryStore(inventoryStore) {}

    void addStock(
        shared_ptr<Product> product,
        int quantity)
    {

        inventoryStore->addProduct(
            product,
            quantity);
    }

    bool reserveProduct(
        int productId,
        int quantity)
    {

        auto inventory =
            inventoryStore->getInventory(
                productId);

        if (!inventory)
            return false;

        return inventory->reserve(quantity);
    }

    void releaseReservation(
        int productId,
        int quantity)
    {

        auto inventory =
            inventoryStore->getInventory(
                productId);

        if (!inventory)
            return;

        inventory->release(quantity);
    }

    bool confirmReservation(
        int productId,
        int quantity)
    {

        auto inventory =
            inventoryStore->getInventory(
                productId);

        if (!inventory)
            return false;

        return inventory->confirmReservation(
            quantity);
    }

    vector<shared_ptr<InventoryItem>>
    getAllInventory()
    {

        return inventoryStore->getAllInventory();
    }

    shared_ptr<InventoryItem>
    getInventory(int productId)
    {

        return inventoryStore->getInventory(
            productId);
    }
};

// =========================================
// STORE
// =========================================

class Store
{

private:
    int id;
    string name;

    double latitude;
    double longitude;

    shared_ptr<InventoryManager>
        inventoryManager;

    shared_ptr<ReplenishmentStrategy>
        replenishmentStrategy;

public:
    Store(
        int id,
        string name,
        double latitude,
        double longitude,
        shared_ptr<InventoryManager>
            inventoryManager,
        shared_ptr<ReplenishmentStrategy>
            replenishmentStrategy)
        : id(id),
          name(name),
          latitude(latitude),
          longitude(longitude),
          inventoryManager(
              inventoryManager),
          replenishmentStrategy(
              replenishmentStrategy) {}

    int getId() const
    {
        return id;
    }

    string getName() const
    {
        return name;
    }

    double getLatitude() const
    {
        return latitude;
    }

    double getLongitude() const
    {
        return longitude;
    }

    shared_ptr<InventoryManager>
    getInventoryManager()
    {

        return inventoryManager;
    }

    void replenishInventory()
    {

        auto items =
            inventoryManager
                ->getAllInventory();

        for (auto &item : items)
        {

            int qty =
                replenishmentStrategy
                    ->getReplenishmentQty(
                        item->getAvailableQuantity());

            if (qty > 0)
            {

                item->addStock(qty);

                cout
                    << "Store "
                    << name
                    << " replenished "
                    << item->getProduct()
                           ->getName()
                    << " by "
                    << qty
                    << endl;
            }
        }
    }
};

// =========================================
// STORE MANAGER
// =========================================

class StoreManager
{

private:
    vector<shared_ptr<Store>> stores;

public:
    void addStore(
        shared_ptr<Store> store)
    {

        stores.push_back(store);
    }

    vector<shared_ptr<Store>>
    getAllStores()
    {

        return stores;
    }

    double distance(
        double x1,
        double y1,
        double x2,
        double y2)
    {

        return sqrt(
            (x1 - x2) * (x1 - x2) +
            (y1 - y2) * (y1 - y2));
    }

    vector<shared_ptr<Store>>
    getNearbyStores(
        double latitude,
        double longitude,
        double radiusKm)
    {

        vector<shared_ptr<Store>>
            result;

        for (auto &store : stores)
        {

            double d =
                distance(
                    latitude,
                    longitude,
                    store->getLatitude(),
                    store->getLongitude());

            if (d <= radiusKm)
                result.push_back(store);
        }

        return result;
    }

    vector<shared_ptr<Product>>
    getProductsForCustomer(
        double latitude,
        double longitude,
        double radiusKm)
    {

        vector<shared_ptr<Product>>
            products;

        auto nearbyStores =
            getNearbyStores(
                latitude,
                longitude,
                radiusKm);

        unordered_set<int> visited;

        for (auto &store : nearbyStores)
        {

            auto inventoryList =
                store
                    ->getInventoryManager()
                    ->getAllInventory();

            for (auto &inventory :
                 inventoryList)
            {

                if (
                    inventory
                        ->getAvailableQuantity() <= 0)
                    continue;

                int productId =
                    inventory
                        ->getProduct()
                        ->getId();

                if (
                    visited.count(
                        productId))
                    continue;

                visited.insert(
                    productId);

                products.push_back(
                    inventory
                        ->getProduct());
            }
        }

        return products;
    }
};

// =========================================
// CART ITEM
// =========================================

class CartItem
{

private:
    shared_ptr<Product> product;
    int quantity;

public:
    CartItem(
        shared_ptr<Product> product,
        int quantity)
        : product(product),
          quantity(quantity) {}

    shared_ptr<Product>
    getProduct() const
    {

        return product;
    }

    int getQuantity() const
    {

        return quantity;
    }

    void setQuantity(int qty)
    {

        quantity = qty;
    }
};

// =========================================
// CART
// =========================================

class Cart
{

private:
    vector<CartItem> items;

public:
    void addItem(
        shared_ptr<Product> product,
        int quantity)
    {

        for (auto &item : items)
        {

            if (
                item.getProduct()
                    ->getId() ==
                product->getId())
            {

                item.setQuantity(
                    item.getQuantity() +
                    quantity);

                return;
            }
        }

        items.push_back(
            CartItem(
                product,
                quantity));
    }

    vector<CartItem> &
    getItems()
    {

        return items;
    }

    double getTotal()
    {

        double total = 0;

        for (auto &item : items)
        {

            total +=
                item.getProduct()
                    ->getPrice() *
                item.getQuantity();
        }

        return total;
    }

    void clear()
    {

        items.clear();
    }
};

// =========================================
// CUSTOMER
// =========================================

class Customer
{

private:
    int id;
    string name;

    double latitude;
    double longitude;

    Cart cart;

public:
    Customer(
        int id,
        string name,
        double latitude,
        double longitude)
        : id(id),
          name(name),
          latitude(latitude),
          longitude(longitude) {}

    int getId() const
    {

        return id;
    }

    string getName() const
    {

        return name;
    }

    double getLatitude() const
    {

        return latitude;
    }

    double getLongitude() const
    {

        return longitude;
    }

    Cart &getCart()
    {

        return cart;
    }
};

// =========================================
// ORDER STATUS
// =========================================

enum class OrderStatus
{

    CREATED,
    PAYMENT_PENDING,
    PAID,
    ASSIGNED,
    OUT_FOR_DELIVERY,
    DELIVERED,
    CANCELLED
};

// =========================================
// ORDER ITEM
// =========================================

class OrderItem
{

private:
    shared_ptr<Product> product;
    int quantity;
    double purchasePrice;

public:
    OrderItem(
        shared_ptr<Product> product,
        int quantity,
        double purchasePrice)
        : product(product),
          quantity(quantity),
          purchasePrice(purchasePrice) {}

    shared_ptr<Product> getProduct()
    {
        return product;
    }

    int getQuantity()
    {
        return quantity;
    }

    double getPurchasePrice()
    {
        return purchasePrice;
    }
};

// =========================================
// ORDER
// =========================================

class Order
{

private:
    int orderId;

    int customerId;

    vector<OrderItem> items;

    double totalAmount;

    OrderStatus status;

public:
    Order(
        int orderId,
        int customerId,
        vector<OrderItem> items,
        double totalAmount)
        : orderId(orderId),
          customerId(customerId),
          items(items),
          totalAmount(totalAmount),
          status(OrderStatus::CREATED) {}

    int getOrderId()
    {
        return orderId;
    }

    vector<OrderItem> &getItems()
    {
        return items;
    }

    double getTotalAmount()
    {
        return totalAmount;
    }

    OrderStatus getStatus()
    {
        return status;
    }

    void setStatus(
        OrderStatus newStatus)
    {

        status = newStatus;
    }
};

// =========================================
// PAYMENT STRATEGY
// =========================================

class PaymentStrategy
{

public:
    virtual ~PaymentStrategy() = default;

    virtual bool pay(
        double amount) = 0;
};

// =========================================
// UPI
// =========================================

class UPIPaymentStrategy
    : public PaymentStrategy
{

public:
    bool pay(
        double amount) override
    {

        cout
            << "UPI Payment Success : "
            << amount
            << endl;

        return true;
    }
};

// =========================================
// CARD
// =========================================

class CardPaymentStrategy
    : public PaymentStrategy
{

public:
    bool pay(
        double amount) override
    {

        cout
            << "Card Payment Success : "
            << amount
            << endl;

        return true;
    }
};

// =========================================
// DELIVERY PARTNER
// =========================================

class DeliveryPartner
{

private:
    int id;

    string name;

    bool available;

public:
    DeliveryPartner(
        int id,
        string name)
        : id(id),
          name(name),
          available(true) {}

    int getId()
    {
        return id;
    }

    string getName()
    {
        return name;
    }

    bool isAvailable()
    {
        return available;
    }

    void setAvailability(
        bool status)
    {

        available = status;
    }
};

// =========================================
// OBSERVER
// =========================================

class INotificationObserver
{

public:
    virtual ~INotificationObserver() = default;

    virtual void notify(
        string message) = 0;
};

// =========================================
// EMAIL
// =========================================

class EmailNotification
    : public INotificationObserver
{

public:
    void notify(
        string message) override
    {

        cout
            << "[EMAIL] "
            << message
            << endl;
    }
};

// =========================================
// SMS
// =========================================

class SMSNotification
    : public INotificationObserver
{

public:
    void notify(
        string message) override
    {

        cout
            << "[SMS] "
            << message
            << endl;
    }
};

// =========================================
// NOTIFICATION SERVICE
// =========================================

class NotificationService
{

private:
    vector<
        shared_ptr<
            INotificationObserver>>
        observers;

public:
    void addObserver(
        shared_ptr<
            INotificationObserver>
            observer)
    {

        observers.push_back(
            observer);
    }

    void sendNotification(
        string msg)
    {

        for (
            auto &observer : observers)
        {

            observer->notify(msg);
        }
    }
};

// =========================================
// DELIVERY MANAGER
// =========================================

class DeliveryManager
{

private:
    vector<
        shared_ptr<
            DeliveryPartner>>
        partners;

public:
    void addPartner(
        shared_ptr<
            DeliveryPartner>
            partner)
    {

        partners.push_back(
            partner);
    }

    shared_ptr<
        DeliveryPartner>
    assignPartner()
    {

        for (
            auto &partner : partners)
        {

            if (
                partner->isAvailable())
            {

                partner
                    ->setAvailability(
                        false);

                return partner;
            }
        }

        return nullptr;
    }

    void markDelivered(
        shared_ptr<
            DeliveryPartner>
            partner)
    {

        partner
            ->setAvailability(
                true);
    }
};

// =========================================
// ORDER MANAGER
// =========================================

class OrderManager
{

private:
    int nextOrderId = 1;

    vector<shared_ptr<Order>> orders;

public:
    shared_ptr<Order>
    createOrder(
        Customer &customer)
    {

        vector<OrderItem> orderItems;

        for (auto &cartItem :
             customer.getCart().getItems())
        {

            orderItems.emplace_back(
                cartItem.getProduct(),
                cartItem.getQuantity(),
                cartItem.getProduct()->getPrice());
        }

        auto order =
            make_shared<Order>(
                nextOrderId++,
                customer.getId(),
                orderItems,
                customer.getCart().getTotal());

        orders.push_back(order);

        return order;
    }

    vector<shared_ptr<Order>> &
    getOrders()
    {

        return orders;
    }
};

// =========================================
// ZEPTO FACADE
// =========================================

class ZeptoService
{

private:
    StoreManager storeManager;

    OrderManager orderManager;

    DeliveryManager deliveryManager;

    NotificationService notificationService;

public:
    StoreManager &
    getStoreManager()
    {

        return storeManager;
    }

    DeliveryManager &
    getDeliveryManager()
    {

        return deliveryManager;
    }

    NotificationService &
    getNotificationService()
    {

        return notificationService;
    }

    vector<shared_ptr<Product>>
    browseProducts(
        Customer &customer,
        double radiusKm)
    {

        return storeManager
            .getProductsForCustomer(
                customer.getLatitude(),
                customer.getLongitude(),
                radiusKm);
    }

    bool checkout(
        Customer &customer,
        shared_ptr<Store> store,
        shared_ptr<PaymentStrategy> paymentStrategy)
    {

        vector<pair<int, int>>
            reservedProducts;

        // -----------------------
        // Reserve Inventory
        // -----------------------

        for (auto &cartItem :
             customer.getCart().getItems())
        {

            bool success =
                store
                    ->getInventoryManager()
                    ->reserveProduct(
                        cartItem.getProduct()->getId(),
                        cartItem.getQuantity());

            if (!success)
            {

                cout
                    << "Reservation failed for "
                    << cartItem
                           .getProduct()
                           ->getName()
                    << endl;

                // rollback

                for (auto &r :
                     reservedProducts)
                {

                    store
                        ->getInventoryManager()
                        ->releaseReservation(
                            r.first,
                            r.second);
                }

                return false;
            }

            reservedProducts.push_back(
                {cartItem
                     .getProduct()
                     ->getId(),

                 cartItem
                     .getQuantity()});
        }

        // -----------------------
        // Payment
        // -----------------------

        double amount =
            customer
                .getCart()
                .getTotal();

        bool paymentSuccess =
            paymentStrategy
                ->pay(amount);

        if (!paymentSuccess)
        {

            for (auto &r :
                 reservedProducts)
            {

                store
                    ->getInventoryManager()
                    ->releaseReservation(
                        r.first,
                        r.second);
            }

            return false;
        }

        // -----------------------
        // Confirm Inventory
        // -----------------------

        for (auto &cartItem :
             customer.getCart().getItems())
        {

            store
                ->getInventoryManager()
                ->confirmReservation(
                    cartItem
                        .getProduct()
                        ->getId(),

                    cartItem
                        .getQuantity());
        }

        // -----------------------
        // Create Order
        // -----------------------

        auto order =
            orderManager
                .createOrder(
                    customer);

        order->setStatus(
            OrderStatus::PAID);

        // -----------------------
        // Delivery Assignment
        // -----------------------

        auto partner =
            deliveryManager
                .assignPartner();

        if (partner)
        {

            order->setStatus(
                OrderStatus::ASSIGNED);

            cout
                << "Delivery Partner Assigned : "
                << partner->getName()
                << endl;
        }

        // -----------------------
        // Notification
        // -----------------------

        notificationService
            .sendNotification(

                "Order #" +
                to_string(
                    order->getOrderId()) +
                " created successfully");

        customer
            .getCart()
            .clear();

        return true;
    }
};

int main()
{

    ZeptoService zepto;

    // =====================================
    // Notifications
    // =====================================

    zepto
        .getNotificationService()
        .addObserver(
            make_shared<
                EmailNotification>());

    zepto
        .getNotificationService()
        .addObserver(
            make_shared<
                SMSNotification>());

    // =====================================
    // Delivery Partners
    // =====================================

    zepto
        .getDeliveryManager()
        .addPartner(

            make_shared<
                DeliveryPartner>(
                1,
                "Rohit"));

    // =====================================
    // Products
    // =====================================

    auto milk =
        make_shared<Product>(
            1,
            "Milk",
            60);

    auto bread =
        make_shared<Product>(
            2,
            "Bread",
            40);

    auto eggs =
        make_shared<Product>(
            3,
            "Eggs",
            80);

    // =====================================
    // Store Inventory
    // =====================================

    auto inventoryStore =
        make_shared<
            InMemoryInventoryStore>();

    auto inventoryManager =
        make_shared<
            InventoryManager>(
            inventoryStore);

    inventoryManager
        ->addStock(
            milk,
            100);

    inventoryManager
        ->addStock(
            bread,
            50);

    inventoryManager
        ->addStock(
            eggs,
            30);

    // =====================================
    // Strategy
    // =====================================

    auto strategy =
        make_shared<
            ThresholdReplenishmentStrategy>(
            10,
            100);

    // =====================================
    // Store
    // =====================================

    auto store =
        make_shared<Store>(
            1,
            "Zepto Sector 22",
            10,
            10,
            inventoryManager,
            strategy);

    zepto
        .getStoreManager()
        .addStore(store);

    // =====================================
    // Customer
    // =====================================

    Customer customer(
        1,
        "Alpha",
        11,
        11);

    // =====================================
    // Browse Products
    // =====================================

    cout
        << "\nAvailable Products\n";

    auto products =
        zepto.browseProducts(
            customer,
            10);

    for (auto &p : products)
    {

        cout
            << p->getName()
            << " "
            << p->getPrice()
            << endl;
    }

    // =====================================
    // Cart
    // =====================================

    customer
        .getCart()
        .addItem(
            milk,
            2);

    customer
        .getCart()
        .addItem(
            bread,
            1);

    // =====================================
    // Payment
    // =====================================

    auto payment =
        make_shared<
            UPIPaymentStrategy>();

    zepto.checkout(
        customer,
        store,
        payment);

    return 0;
}
