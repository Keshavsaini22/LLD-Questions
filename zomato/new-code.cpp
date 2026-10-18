
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <optional>
#include <atomic>
#include <stdexcept>

using namespace std;

// ============================================================
// 1. DOMAIN MODELS
// ============================================================

struct MenuItem
{
    int id;
    string name;
    double price;
    bool available;

    MenuItem(int id, string name, double price,
             bool available = true)
        : id(id), name(std::move(name)),
          price(price), available(available) {}
};

class Customer
{
private:
    int id;
    string name;

public:
    Customer(int id, string name)
        : id(id), name(std::move(name)) {}

    int getId() const { return id; }
    string getName() const { return name; }
};

class Restaurant
{
private:
    int id;
    string name;
    unordered_map<int, MenuItem> menu;
    mutable mutex mtx;

public:
    Restaurant(int id, string name)
        : id(id), name(std::move(name)) {}

    int getId() const { return id; }
    string getName() const { return name; }

    void addMenuItem(const MenuItem &item)
    {
        lock_guard<mutex> lock(mtx);
        menu[item.id] = item;
    }

    optional<MenuItem> getMenuItem(int itemId) const
    {
        lock_guard<mutex> lock(mtx);

        auto it = menu.find(itemId);

        if (it == menu.end())
        {
            return nullopt;
        }

        return it->second;
    }
};

struct CartItem
{
    int itemId;
    int quantity;
};

class Cart
{
private:
    int restaurantId = -1;
    unordered_map<int, CartItem> items;
    mutable mutex mtx;

public:
    bool addItem(int restaurantId, int itemId, int quantity)
    {
        if (quantity <= 0)
            return false;

        lock_guard<mutex> lock(mtx);

        if (this->restaurantId != -1 &&
            this->restaurantId != restaurantId)
        {
            return false;
        }

        this->restaurantId = restaurantId;

        auto it = items.find(itemId);

        if (it == items.end())
        {
            items[itemId] = {itemId, quantity};
        }
        else
        {
            it->second.quantity += quantity;
        }

        return true;
    }

    int getRestaurantId() const
    {
        lock_guard<mutex> lock(mtx);
        return restaurantId;
    }

    vector<CartItem> getItems() const
    {
        lock_guard<mutex> lock(mtx);

        vector<CartItem> result;

        for (const auto &[id, item] : items)
        {
            result.push_back(item);
        }

        return result;
    }

    bool empty() const
    {
        lock_guard<mutex> lock(mtx);
        return items.empty();
    }

    void clear()
    {
        lock_guard<mutex> lock(mtx);
        items.clear();
        restaurantId = -1;
    }
};

// ============================================================
// 2. ORDER AND ORDER STATE
// ============================================================

enum class OrderStatus
{
    PENDING_PAYMENT,
    CONFIRMED,
    PREPARING,
    READY_FOR_PICKUP,
    OUT_FOR_DELIVERY,
    DELIVERED,
    PAYMENT_FAILED,
    CANCELLED
};

string toString(OrderStatus status)
{
    switch (status)
    {
    case OrderStatus::PENDING_PAYMENT:
        return "PENDING_PAYMENT";
    case OrderStatus::CONFIRMED:
        return "CONFIRMED";
    case OrderStatus::PREPARING:
        return "PREPARING";
    case OrderStatus::READY_FOR_PICKUP:
        return "READY_FOR_PICKUP";
    case OrderStatus::OUT_FOR_DELIVERY:
        return "OUT_FOR_DELIVERY";
    case OrderStatus::DELIVERED:
        return "DELIVERED";
    case OrderStatus::PAYMENT_FAILED:
        return "PAYMENT_FAILED";
    case OrderStatus::CANCELLED:
        return "CANCELLED";
    }

    return "UNKNOWN";
}

struct OrderItem
{
    int itemId;
    string name;
    double unitPrice;
    int quantity;
};

class Order
{
private:
    int id;
    int customerId;
    int restaurantId;
    vector<OrderItem> items;
    double totalAmount = 0;
    OrderStatus status = OrderStatus::PENDING_PAYMENT;
    mutable mutex mtx;

    bool validTransition(OrderStatus next) const
    {
        switch (status)
        {
        case OrderStatus::PENDING_PAYMENT:
            return next == OrderStatus::CONFIRMED ||
                   next == OrderStatus::PAYMENT_FAILED ||
                   next == OrderStatus::CANCELLED;

        case OrderStatus::CONFIRMED:
            return next == OrderStatus::PREPARING ||
                   next == OrderStatus::CANCELLED;

        case OrderStatus::PREPARING:
            return next == OrderStatus::READY_FOR_PICKUP ||
                   next == OrderStatus::CANCELLED;

        case OrderStatus::READY_FOR_PICKUP:
            return next == OrderStatus::OUT_FOR_DELIVERY ||
                   next == OrderStatus::CANCELLED;

        case OrderStatus::OUT_FOR_DELIVERY:
            return next == OrderStatus::DELIVERED;

        case OrderStatus::DELIVERED:
        case OrderStatus::PAYMENT_FAILED:
        case OrderStatus::CANCELLED:
            return false;
        }

        return false;
    }

public:
    Order(int id, int customerId, int restaurantId,
          vector<OrderItem> items)
        : id(id), customerId(customerId),
          restaurantId(restaurantId), items(std::move(items))
    {

        for (const auto &item : this->items)
        {
            totalAmount += item.unitPrice * item.quantity;
        }
    }

    int getId() const { return id; }
    int getCustomerId() const { return customerId; }
    int getRestaurantId() const { return restaurantId; }
    double getTotalAmount() const { return totalAmount; }

    OrderStatus getStatus() const
    {
        lock_guard<mutex> lock(mtx);
        return status;
    }

    bool transitionTo(OrderStatus next)
    {
        lock_guard<mutex> lock(mtx);

        if (!validTransition(next))
        {
            return false;
        }

        status = next;
        return true;
    }

    void print() const
    {
        cout << "\nOrder #" << id
             << " | Customer: " << customerId
             << " | Restaurant: " << restaurantId << '\n';

        for (const auto &item : items)
        {
            cout << item.name << " x" << item.quantity
                 << " = " << item.unitPrice * item.quantity
                 << '\n';
        }

        cout << "Total: " << totalAmount
             << " | Status: " << toString(getStatus())
             << '\n';
    }
};

// ============================================================
// 3. OBSERVER PATTERN
// ============================================================

enum class OrderEventType
{
    ORDER_PLACED,
    ORDER_READY_FOR_PICKUP
};

struct OrderEvent
{
    OrderEventType type;
    shared_ptr<Order> order;
};

class IOrderObserver
{
public:
    virtual void onOrderEvent(const OrderEvent &event) = 0;
    virtual ~IOrderObserver() = default;
};

class OrderEventPublisher
{
private:
    vector<shared_ptr<IOrderObserver>> observers;
    mutex mtx;

public:
    void subscribe(shared_ptr<IOrderObserver> observer)
    {
        lock_guard<mutex> lock(mtx);
        observers.push_back(std::move(observer));
    }

    void publish(const OrderEvent &event)
    {
        vector<shared_ptr<IOrderObserver>> snapshot;

        {
            lock_guard<mutex> lock(mtx);
            snapshot = observers;
        }

        // Do not hold the publisher mutex while executing callbacks.
        for (const auto &observer : snapshot)
        {
            observer->onOrderEvent(event);
        }
    }
};

// ============================================================
// 4. PAYMENT STRATEGY
// ============================================================

class IPaymentStrategy
{
public:
    virtual bool pay(int orderId, double amount) = 0;
    virtual ~IPaymentStrategy() = default;
};

class UpiPayment : public IPaymentStrategy
{
public:
    bool pay(int orderId, double amount) override
    {
        cout << "UPI payment: order #" << orderId
             << ", amount = " << amount << '\n';
        return true; // Simulated provider response
    }
};

class CardPayment : public IPaymentStrategy
{
public:
    bool pay(int orderId, double amount) override
    {
        cout << "Card payment: order #" << orderId
             << ", amount = " << amount << '\n';
        return true; // Simulated provider response
    }
};

// ============================================================
// 5. ORDER SERVICE
// ============================================================

class OrderService
{
private:
    atomic<int> nextId{1};
    unordered_map<int, shared_ptr<Order>> orders;
    mutable mutex mtx;

public:
    shared_ptr<Order> createOrder(
        const Customer &customer,
        const Restaurant &restaurant,
        Cart &cart,
        IPaymentStrategy &payment)
    {

        if (cart.empty() ||
            cart.getRestaurantId() != restaurant.getId())
        {
            return nullptr;
        }

        vector<OrderItem> orderItems;

        // Validate current menu availability and use checkout prices.
        for (const auto &cartItem : cart.getItems())
        {
            auto menuItem = restaurant.getMenuItem(cartItem.itemId);

            if (!menuItem || !menuItem->available)
            {
                cout << "Checkout failed: item unavailable\n";
                return nullptr;
            }

            orderItems.push_back({menuItem->id,
                                  menuItem->name,
                                  menuItem->price,
                                  cartItem.quantity});
        }

        int id = nextId.fetch_add(1);

        auto order = make_shared<Order>(
            id, customer.getId(), restaurant.getId(),
            std::move(orderItems));

        {
            lock_guard<mutex> lock(mtx);
            orders[id] = order;
        }

        bool paid = false;

        try
        {
            paid = payment.pay(id, order->getTotalAmount());
        }
        catch (...)
        {
            order->transitionTo(OrderStatus::PAYMENT_FAILED);
            return order;
        }

        if (!paid)
        {
            order->transitionTo(OrderStatus::PAYMENT_FAILED);
            return order;
        }

        order->transitionTo(OrderStatus::CONFIRMED);
        cart.clear();

        return order;
    }

    shared_ptr<Order> getOrder(int id) const
    {
        lock_guard<mutex> lock(mtx);

        auto it = orders.find(id);
        return it == orders.end() ? nullptr : it->second;
    }
};

// ============================================================
// 6. DELIVERY ASSIGNMENT SERVICE
// ============================================================

class DeliveryPartner
{
private:
    int id;
    string name;
    bool available = true;
    int currentOrderId = -1;
    mutex mtx;

public:
    DeliveryPartner(int id, string name)
        : id(id), name(std::move(name)) {}

    string getName() const { return name; }

    bool tryAssign(int orderId)
    {
        lock_guard<mutex> lock(mtx);

        if (!available)
            return false;

        available = false;
        currentOrderId = orderId;
        return true;
    }

    bool completeDelivery(int orderId)
    {
        lock_guard<mutex> lock(mtx);

        if (available || currentOrderId != orderId)
        {
            return false;
        }

        available = true;
        currentOrderId = -1;
        return true;
    }
};

class DeliveryAssignmentService
{
private:
    vector<shared_ptr<DeliveryPartner>> partners;
    unordered_map<int, shared_ptr<DeliveryPartner>> assignments;
    mutex mtx;

public:
    void addPartner(shared_ptr<DeliveryPartner> partner)
    {
        lock_guard<mutex> lock(mtx);
        partners.push_back(std::move(partner));
    }

    shared_ptr<DeliveryPartner> assignPartner(int orderId)
    {
        lock_guard<mutex> lock(mtx);

        // Make repeated assignment requests idempotent.
        auto existing = assignments.find(orderId);

        if (existing != assignments.end())
        {
            return existing->second;
        }

        for (const auto &partner : partners)
        {
            if (partner->tryAssign(orderId))
            {
                assignments[orderId] = partner;

                cout << "Delivery partner assigned: "
                     << partner->getName()
                     << " for order #" << orderId << '\n';

                return partner;
            }
        }

        cout << "No delivery partner available for order #"
             << orderId << '\n';

        return nullptr;
    }

    bool completeDelivery(int orderId)
    {
        lock_guard<mutex> lock(mtx);

        auto it = assignments.find(orderId);

        if (it == assignments.end())
        {
            return false;
        }

        if (!it->second->completeDelivery(orderId))
        {
            return false;
        }

        assignments.erase(it);
        return true;
    }
};

// ============================================================
// 7. OBSERVERS
// ============================================================

class RestaurantNotificationObserver : public IOrderObserver
{
public:
    void onOrderEvent(const OrderEvent &event) override
    {
        if (event.type != OrderEventType::ORDER_PLACED)
        {
            return;
        }

        cout << "[Restaurant notification] Restaurant #"
             << event.order->getRestaurantId()
             << ": new order #" << event.order->getId()
             << " received\n";
    }
};

class DeliveryAssignmentObserver : public IOrderObserver
{
private:
    DeliveryAssignmentService &deliveryService;

public:
    explicit DeliveryAssignmentObserver(
        DeliveryAssignmentService &deliveryService)
        : deliveryService(deliveryService) {}

    void onOrderEvent(const OrderEvent &event) override
    {
        if (event.type != OrderEventType::ORDER_READY_FOR_PICKUP)
        {
            return;
        }

        cout << "[Delivery observer] Order #"
             << event.order->getId()
             << " is ready; assigning a partner\n";

        auto partner =
            deliveryService.assignPartner(event.order->getId());

        if (partner)
        {
            // A real system would persist this assignment and
            // coordinate the order transition transactionally.
            if (!event.order->transitionTo(
                    OrderStatus::OUT_FOR_DELIVERY))
            {
                deliveryService.completeDelivery(
                    event.order->getId());
            }
        }
    }
};

// ============================================================
// 8. ZOMATO FACADE
// ============================================================

class ZomatoFacade
{
private:
    OrderService orderService;
    DeliveryAssignmentService deliveryService;
    OrderEventPublisher eventPublisher;

    shared_ptr<RestaurantNotificationObserver> restaurantObserver;
    shared_ptr<DeliveryAssignmentObserver> deliveryObserver;

public:
    ZomatoFacade()
    {
        restaurantObserver =
            make_shared<RestaurantNotificationObserver>();

        deliveryObserver =
            make_shared<DeliveryAssignmentObserver>(deliveryService);

        eventPublisher.subscribe(restaurantObserver);
        eventPublisher.subscribe(deliveryObserver);
    }

    void addDeliveryPartner(shared_ptr<DeliveryPartner> partner)
    {
        deliveryService.addPartner(std::move(partner));
    }

    shared_ptr<Order> placeOrder(
        const Customer &customer,
        const Restaurant &restaurant,
        Cart &cart,
        IPaymentStrategy &payment)
    {

        auto order = orderService.createOrder(
            customer, restaurant, cart, payment);

        if (!order)
        {
            cout << "Could not create order\n";
            return nullptr;
        }

        if (order->getStatus() != OrderStatus::CONFIRMED)
        {
            cout << "Order #" << order->getId()
                 << " was not confirmed\n";
            return order;
        }

        // Notify all interested components about the new order.
        eventPublisher.publish({OrderEventType::ORDER_PLACED, order});

        return order;
    }

    bool markPreparing(int orderId)
    {
        auto order = orderService.getOrder(orderId);

        return order &&
               order->transitionTo(OrderStatus::PREPARING);
    }

    bool markReadyForPickup(int orderId)
    {
        auto order = orderService.getOrder(orderId);

        if (!order ||
            !order->transitionTo(OrderStatus::READY_FOR_PICKUP))
        {
            return false;
        }

        // Delivery assignment is triggered by an event.
        eventPublisher.publish({OrderEventType::ORDER_READY_FOR_PICKUP, order});

        return true;
    }

    bool markDelivered(int orderId)
    {
        auto order = orderService.getOrder(orderId);

        if (!order ||
            order->getStatus() != OrderStatus::OUT_FOR_DELIVERY)
        {
            return false;
        }

        // In production, delivery completion and partner release
        // must be coordinated with durable state changes.
        if (!deliveryService.completeDelivery(orderId))
        {
            return false;
        }

        return order->transitionTo(OrderStatus::DELIVERED);
    }

    shared_ptr<Order> trackOrder(int orderId) const
    {
        return orderService.getOrder(orderId);
    }
};

// ============================================================
// 9. CLIENT
// ============================================================

int main()
{
    Restaurant restaurant(1, "Punjabi Tadka");

    restaurant.addMenuItem(MenuItem(101, "Paneer Tikka", 250));
    restaurant.addMenuItem(MenuItem(102, "Dal Makhani", 200));
    restaurant.addMenuItem(MenuItem(103, "Butter Naan", 40));

    Customer customer(1, "Amit");
    Cart cart;

    cart.addItem(restaurant.getId(), 101, 2);
    cart.addItem(restaurant.getId(), 102, 1);
    cart.addItem(restaurant.getId(), 103, 3);

    UpiPayment payment;
    ZomatoFacade zomato;

    zomato.addDeliveryPartner(
        make_shared<DeliveryPartner>(1, "Rahul"));

    zomato.addDeliveryPartner(
        make_shared<DeliveryPartner>(2, "Vikram"));

    // Client calls the facade; it does not assign partners itself.
    auto order = zomato.placeOrder(
        customer, restaurant, cart, payment);

    if (!order)
        return 0;

    order->print();

    // Simulate restaurant actions through the facade.
    zomato.markPreparing(order->getId());
    zomato.markReadyForPickup(order->getId());

    auto trackedOrder = zomato.trackOrder(order->getId());

    if (trackedOrder)
    {
        trackedOrder->print();
    }

    zomato.markDelivered(order->getId());

    cout << "\nFinal order state:\n";
    zomato.trackOrder(order->getId())->print();

    return 0;
}
