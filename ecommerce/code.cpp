#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

using namespace std;

// =====================================================
// ENUMS
// =====================================================

enum class OrderStatus {
    CREATED,
    PAID,
    FAILED
};

// =====================================================
// PRODUCT
// =====================================================

class Product {
private:
    int id;
    string name;
    double price;

public:
    Product(int id, string name, double price)
        : id(id), name(name), price(price) {}

    int getId() const {
        return id;
    }

    string getName() const {
        return name;
    }

    double getPrice() const {
        return price;
    }
};

// =====================================================
// INVENTORY
// =====================================================

class Inventory {
private:
    Product* product;
    int stock;
    mutable mutex mtx;

public:
    Inventory(Product* product, int stock)
        : product(product), stock(stock) {}

    bool reserveStock(int qty) {
        lock_guard<mutex> lock(mtx);

        if (stock < qty)
            return false;

        stock -= qty;
        return true;
    }

    void releaseStock(int qty) {
        lock_guard<mutex> lock(mtx);
        stock += qty;
    }

    int getStock() const {
        return stock;
    }

    Product* getProduct() const {
        return product;
    }
};

// =====================================================
// USER
// =====================================================

class User {
protected:
    int id;
    string name;
    string email;

public:
    User(int id, string name, string email)
        : id(id), name(name), email(email) {}

    virtual ~User() = default;

    string getName() const {
        return name;
    }

    string getEmail() const {
        return email;
    }
};

class Customer : public User {
public:
    Customer(int id, string name, string email)
        : User(id, name, email) {}
};

// =====================================================
// CART
// =====================================================

class CartItem {
public:
    Product* product;
    int quantity;

    CartItem(Product* product, int quantity)
        : product(product), quantity(quantity) {}
};

class Cart {
private:
    Customer* customer;
    vector<CartItem> items;

public:
    Cart(Customer* customer)
        : customer(customer) {}

    void addItem(Product* product, int quantity) {
        items.emplace_back(product, quantity);
    }

    vector<CartItem>& getItems() {
        return items;
    }

    void clear() {
        items.clear();
    }
};

// =====================================================
// ORDER
// =====================================================

class OrderItem {
private:
    string productName;
    int quantity;
    double purchasePrice;

public:
    OrderItem(string productName,
              int quantity,
              double purchasePrice)
        : productName(productName),
          quantity(quantity),
          purchasePrice(purchasePrice) {}

    double getTotalPrice() const {
        return quantity * purchasePrice;
    }

    string getProductName() const {
        return productName;
    }
};

class Order {
private:
    int orderId;
    Customer* customer;
    vector<OrderItem> items;
    OrderStatus status;

public:
    Order(int orderId, Customer* customer)
        : orderId(orderId),
          customer(customer),
          status(OrderStatus::CREATED) {}

    void addItem(const OrderItem& item) {
        items.push_back(item);
    }

    double getTotalAmount() const {
        double total = 0;

        for (const auto& item : items)
            total += item.getTotalPrice();

        return total;
    }

    void setStatus(OrderStatus status) {
        this->status = status;
    }

    OrderStatus getStatus() const {
        return status;
    }

    int getOrderId() const {
        return orderId;
    }
};

// =====================================================
// PAYMENT STRATEGY
// =====================================================

class PaymentStrategy {
public:
    virtual ~PaymentStrategy() = default;

    virtual bool pay(double amount) = 0;
};

class UPIPayment : public PaymentStrategy {
public:
    bool pay(double amount) override {
        cout << "UPI Payment of Rs "
             << amount
             << " successful\n";
        return true;
    }
};

class CardPayment : public PaymentStrategy {
public:
    bool pay(double amount) override {
        cout << "Card Payment of Rs "
             << amount
             << " successful\n";
        return true;
    }
};

// =====================================================
// OBSERVER
// =====================================================

class INotificationObserver {
public:
    virtual ~INotificationObserver() = default;

    virtual void notify(const string& msg) = 0;
};

class EmailNotification : public INotificationObserver {
public:
    void notify(const string& msg) override {
        cout << "[EMAIL] " << msg << endl;
    }
};

class SMSNotification : public INotificationObserver {
public:
    void notify(const string& msg) override {
        cout << "[SMS] " << msg << endl;
    }
};

class NotificationService {
private:
    vector<shared_ptr<INotificationObserver>> observers;

public:
    void addObserver(shared_ptr<INotificationObserver> observer) {
        observers.push_back(observer);
    }

    void sendNotification(const string& msg) {
        for (auto& observer : observers)
            observer->notify(msg);
    }
};

// =====================================================
// INVENTORY MANAGER
// =====================================================

class InventoryManager {
private:
    unordered_map<int, shared_ptr<Inventory>> inventories;

public:
    void addInventory(shared_ptr<Inventory> inventory) {
        inventories[inventory->getProduct()->getId()] =
            inventory;
    }

    bool reserve(int productId, int qty) {
        auto it = inventories.find(productId);
        
        if (it == inventories.end())
            return false;

        return it->second->reserveStock(qty);
    }

    void release(int productId, int qty) {
        auto it = inventories.find(productId);

        if (it != inventories.end())
            it->second->releaseStock(qty);
    }
};

// =====================================================
// ORDER MANAGER
// =====================================================

class OrderManager {
private:
    int nextOrderId = 1;

public:
    shared_ptr<Order> createOrder(Customer* customer,
                                  Cart& cart) {
        auto order =
            make_shared<Order>(nextOrderId++, customer);

        for (auto& item : cart.getItems()) {
            order->addItem(
                OrderItem(
                    item.product->getName(),
                    item.quantity,
                    item.product->getPrice()
                )
            );
        }

        return order;
    }
};

// =====================================================
// FACADE
// =====================================================

class EcommerceService {
private:
    InventoryManager inventoryManager;
    OrderManager orderManager;
    NotificationService notificationService;

public:
    InventoryManager& getInventoryManager() {
        return inventoryManager;
    }

    NotificationService& getNotificationService() {
        return notificationService;
    }

    shared_ptr<Order> checkout(
        Customer* customer,
        Cart& cart,
        PaymentStrategy* paymentStrategy)
    {
        vector<pair<int, int>> reservedProducts;

        for (auto& item : cart.getItems()) {

            bool success =
                inventoryManager.reserve(
                    item.product->getId(),
                    item.quantity);

            if (!success) {

                for (auto& reserved : reservedProducts) {
                    inventoryManager.release(
                        reserved.first,
                        reserved.second);
                }

                cout << "Inventory unavailable\n";
                return nullptr;
            }

            reservedProducts.push_back(
                {
                    item.product->getId(),
                    item.quantity
                }
            );
        }

        auto order =
            orderManager.createOrder(customer, cart);

        double amount = order->getTotalAmount();

        bool paymentSuccess =
            paymentStrategy->pay(amount);

        if (!paymentSuccess) {

            for (auto& reserved : reservedProducts) {
                inventoryManager.release(
                    reserved.first,
                    reserved.second);
            }

            order->setStatus(OrderStatus::FAILED);

            return order;
        }

        order->setStatus(OrderStatus::PAID);

        notificationService.sendNotification(
            "Order "
            + to_string(order->getOrderId())
            + " placed successfully"
        );

        cart.clear();

        return order;
    }
};

// =====================================================
// MAIN
// =====================================================

int main() {

    Product iphone(1, "iPhone 16", 100000);
    Product airpods(2, "AirPods Pro", 25000);

    auto iphoneInventory =
        make_shared<Inventory>(&iphone, 10);

    auto airpodsInventory =
        make_shared<Inventory>(&airpods, 20);

    Customer customer(
        1,
        "Alpha",
        "alpha@gmail.com"
    );

    Cart cart(&customer);

    cart.addItem(&iphone, 1);
    cart.addItem(&airpods, 2);

    EcommerceService ecommerce;

    ecommerce.getInventoryManager()
        .addInventory(iphoneInventory);

    ecommerce.getInventoryManager()
        .addInventory(airpodsInventory);

    ecommerce.getNotificationService()
        .addObserver(
            make_shared<EmailNotification>());

    ecommerce.getNotificationService()
        .addObserver(
            make_shared<SMSNotification>());

    UPIPayment payment;

    auto order =
        ecommerce.checkout(
            &customer,
            cart,
            &payment);

    if (order &&
        order->getStatus() == OrderStatus::PAID) {

        cout << "\nOrder Created Successfully\n";
        cout << "Order Id: "
             << order->getOrderId()
             << endl;
    }

    return 0;
}