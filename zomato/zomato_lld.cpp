#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <ctime>
using namespace std;

// ===================== ENTITIES =====================

class MenuItem
{
    int id;
    string name;
    int price;

public:
    MenuItem(int id, string name, int price)
        : id(id), name(name), price(price) {}

    int getId() const { return id; }
    string getName() const { return name; }
    int getPrice() const { return price; }
};

class Restaurant
{
    int id;
    string name;
    string city;
    vector<MenuItem *> menuItems;

public:
    Restaurant(int id, string name, string city)
        : id(id), name(name), city(city) {}

    int getId() const { return id; }
    string getName() const { return name; }

    void addMenuItem(MenuItem *item)
    {
        menuItems.push_back(item);
    }

    vector<MenuItem *> &getMenu()
    {
        return menuItems;
    }
};

class User
{
    int id;
    string name;

public:
    User(int id, string name)
        : id(id), name(name) {}

    int getId() const { return id; }
    string getName() const { return name; }
};

class CartItem
{
public:
    MenuItem *item;
    int quantity;

    CartItem(MenuItem *item, int quantity)
        : item(item), quantity(quantity) {}
};

class Cart
{
    int userId;
    int restaurantId;
    vector<CartItem> items;

public:
    Cart(int userId, int restaurantId)
        : userId(userId), restaurantId(restaurantId) {}

    void addItem(MenuItem *item, int qty)
    {
        items.emplace_back(item, qty);
    }

    vector<CartItem> &getItems()
    {
        return items;
    }

    int getTotal() const
    {
        int total = 0;
        for (auto &x : items)
            total += x.item->getPrice() * x.quantity;
        return total;
    }

    int getUserId() const { return userId; }
    int getRestaurantId() const { return restaurantId; }
};

enum class OrderStatus
{
    CREATED,
    ACCEPTED,
    PREPARING,
    OUT_FOR_DELIVERY,
    DELIVERED
};

class DeliveryPartner
{
    int id;
    string name;
    bool available;

public:
    DeliveryPartner(int id, string name)
        : id(id), name(name), available(true) {}

    bool isAvailable() const { return available; }
    void assign() { available = false; }
    void release() { available = true; }
    string getName() const { return name; }
};

class Order
{
protected:
    int id;
    int userId;
    int restaurantId;
    vector<CartItem> items;
    int amount;
    OrderStatus status;

public:
    Order(int id, int userId, int restaurantId,
          vector<CartItem> items, int amount)
        : id(id), userId(userId),
          restaurantId(restaurantId),
          items(items), amount(amount),
          status(OrderStatus::CREATED) {}

    virtual ~Order() = default;

    virtual void showDetails() = 0;

    int getId() const { return id; }
    int getAmount() const { return amount; }
};

class DeliveryOrder : public Order
{
public:
    DeliveryOrder(int id, int userId, int restaurantId,
                  vector<CartItem> items, int amount)
        : Order(id, userId, restaurantId, items, amount) {}

    void showDetails() override
    {
        cout << "Delivery Order Id : " << id << "\n";
    }
};

class ScheduledOrder : public Order
{
    time_t scheduledTime;

public:
    ScheduledOrder(int id, int userId, int restaurantId,
                   vector<CartItem> items, int amount,
                   time_t scheduledTime)
        : Order(id, userId, restaurantId, items, amount),
          scheduledTime(scheduledTime) {}

    void showDetails() override
    {
        cout << "Scheduled Order Id : " << id << "\n";
    }
};

enum class OrderType
{
    DELIVERY_NOW,
    SCHEDULED
};

class OrderFactory
{
public:
    static unique_ptr<Order> createOrder(
        OrderType type,
        int orderId,
        Cart *cart,
        time_t scheduledTime = 0)
    {

        if (type == OrderType::DELIVERY_NOW)
        {
            return make_unique<DeliveryOrder>(
                orderId,
                cart->getUserId(),
                cart->getRestaurantId(),
                cart->getItems(),
                cart->getTotal());
        }

        return make_unique<ScheduledOrder>(
            orderId,
            cart->getUserId(),
            cart->getRestaurantId(),
            cart->getItems(),
            cart->getTotal(),
            scheduledTime);
    }
};

// ===================== STRATEGY =====================

class PaymentStrategy
{
public:
    virtual ~PaymentStrategy() = default;
    virtual bool pay(int amount) = 0;
};

class UpiPayment : public PaymentStrategy
{
public:
    bool pay(int amount) override
    {
        cout << "UPI Payment of " << amount << "\n";
        return true;
    }
};

class CardPayment : public PaymentStrategy
{
public:
    bool pay(int amount) override
    {
        cout << "Card Payment of " << amount << "\n";
        return true;
    }
};

// ===================== OBSERVER =====================

class INotification
{
public:
    virtual ~INotification() = default;
    virtual void notify(string msg) = 0;
};

class EmailNotification : public INotification
{
public:
    void notify(string msg) override
    {
        cout << "[EMAIL] " << msg << "\n";
    }
};

class SMSNotification : public INotification
{
public:
    void notify(string msg) override
    {
        cout << "[SMS] " << msg << "\n";
    }
};

class NotificationManager
{
    vector<INotification *> observers;

public:
    void addObserver(INotification *obs)
    {
        observers.push_back(obs);
    }

    void notifyAll(string msg)
    {
        for (auto obs : observers)
            obs->notify(msg);
    }
};

// ===================== MANAGERS =====================

class RestaurantManager
{
    unordered_map<int, Restaurant *> restaurants;

public:
    void addRestaurant(Restaurant *r)
    {
        restaurants[r->getId()] = r;
    }

    Restaurant *getRestaurant(int id)
    {
        return restaurants[id];
    }
};

class CartManager
{
    unordered_map<int, unique_ptr<Cart>> carts;

public:
    Cart *createCart(int userId, int restaurantId)
    {
        carts[userId] = make_unique<Cart>(userId, restaurantId);
        return carts[userId].get();
    }

    Cart *getCart(int userId)
    {
        return carts[userId].get();
    }
};

class OrderManager
{
    unordered_map<int, unique_ptr<Order>> orders;
    int nextId = 1;

public:
    Order *createOrder(OrderType type, Cart *cart)
    {
        auto order = OrderFactory::createOrder(
            type,
            nextId++,
            cart);

        int id = order->getId();
        orders[id] = move(order);

        return orders[id].get();
    }
};

class PaymentManager
{
public:
    bool processPayment(
        int amount,
        PaymentStrategy *strategy)
    {

        return strategy->pay(amount);
    }
};

class DeliveryManager
{
    vector<DeliveryPartner *> partners;

public:
    void addPartner(DeliveryPartner *partner)
    {
        partners.push_back(partner);
    }

    DeliveryPartner *assignPartner()
    {
        for (auto partner : partners)
        {
            if (partner->isAvailable())
            {
                partner->assign();
                return partner;
            }
        }
        return nullptr;
    }
};

// ===================== FACADE =====================

class ZomatoService
{

    RestaurantManager restaurantManager;
    CartManager cartManager;
    OrderManager orderManager;
    PaymentManager paymentManager;
    DeliveryManager deliveryManager;
    NotificationManager notificationManager;

public:
    RestaurantManager &getRestaurantManager()
    {
        return restaurantManager;
    }

    CartManager &getCartManager()
    {
        return cartManager;
    }

    DeliveryManager &getDeliveryManager()
    {
        return deliveryManager;
    }

    NotificationManager &getNotificationManager()
    {
        return notificationManager;
    }

    void placeOrder(
        User *user,
        Cart *cart,
        PaymentStrategy *payment,
        OrderType type)
    {

        Order *order =
            orderManager.createOrder(type, cart);

        if (!paymentManager.processPayment(
                cart->getTotal(),
                payment))
        {

            cout << "Payment Failed\n";
            return;
        }

        DeliveryPartner *partner =
            deliveryManager.assignPartner();

        order->showDetails();

        if (partner)
            cout << "Assigned Partner : "
                 << partner->getName() << "\n";

        notificationManager.notifyAll(
            "Order Placed Successfully");
    }
};

// ===================== MAIN =====================

int main()
{

    ZomatoService zomato;

    auto *email = new EmailNotification();
    auto *sms = new SMSNotification();

    zomato.getNotificationManager().addObserver(email);
    zomato.getNotificationManager().addObserver(sms);

    auto *burger = new MenuItem(1, "Burger", 200);
    auto *pizza = new MenuItem(2, "Pizza", 300);

    auto *restaurant =
        new Restaurant(1, "Burger King", "Delhi");

    restaurant->addMenuItem(burger);
    restaurant->addMenuItem(pizza);

    zomato.getRestaurantManager()
        .addRestaurant(restaurant);

    User user(1, "Alpha");

    Cart *cart =
        zomato.getCartManager()
            .createCart(user.getId(),
                        restaurant->getId());

    cart->addItem(burger, 2);
    cart->addItem(pizza, 1);

    auto *rider =
        new DeliveryPartner(1, "Rider-A");

    zomato.getDeliveryManager()
        .addPartner(rider);

    UpiPayment payment;

    zomato.placeOrder(
        &user,
        cart,
        &payment,
        OrderType::DELIVERY_NOW);

    return 0;
}
