#include <iostream>
#include <bits/stdc++.h>
using namespace std;

// Creation of notification which will be decorator pattern

class Notification
{
protected:
    string sender;
    string desc;
    string subject;

public:
    Notification(string sender, string desc, string subject)
    {
        this->sender = sender;
        this->desc = desc;
        this->subject = subject;
    }

    virtual ~Notification() = default;
    virtual string getDesc() = 0;
    virtual string getSender() = 0;
    virtual string getSubject() = 0;
};

class SimpleNotification : public Notification
{
public:
    SimpleNotification(string sender, string desc, string subject) : Notification(sender, desc, subject) {}

    string getDesc() override
    {
        return desc;
    }

    string getSender() override
    {
        return sender;
    }

    string getSubject() override
    {
        return subject;
    }
};

class ModifiedNotification : public Notification
{
protected:
    Notification *base;

public:
    ModifiedNotification(Notification *base) : Notification("", "", "")
    {
        this->base = base;
    }

    virtual ~ModifiedNotification()
    {
        delete base;
    }
};

class TimestampNotification : public ModifiedNotification
{
public:
    TimestampNotification(Notification *base) : ModifiedNotification(base) {}

    string getDesc() override
    {
        time_t now = time(0);
        string currentTime = ctime(&now);

        // remove trailing newline
        currentTime.pop_back();

        return currentTime + " -> " + base->getDesc();
    }

    string getSender() override
    {
        return base->getSender();
    }

    string getSubject() override
    {
        return base->getSubject();
    }
};

class SignatureNotification : public ModifiedNotification
{
    string signature;

public:
    SignatureNotification(Notification *base, string signature) : ModifiedNotification(base)
    {
        this->signature = signature;
    }

    string getDesc() override
    {
        return base->getDesc() + " " + signature;
    }

    string getSender() override
    {
        return base->getSender();
    }

    string getSubject() override
    {
        return base->getSubject();
    }
};

// Different Strategies

class ChannelStrategy
{
public:
    virtual ~ChannelStrategy() = default;
    virtual bool sendNotification(Notification *notification) = 0;
};

class EmailChannel : public ChannelStrategy
{
public:
    bool sendNotification(Notification *notification) override
    {
        cout << " Sending notification via Email to " << notification->getSender() << endl;
        return true;
    }
};

class PhoneChannel : public ChannelStrategy
{
public:
    bool sendNotification(Notification *notification) override
    {
        cout << " Sending notification via Phone to " << notification->getSender() << endl;
        return true;
    }
};

class SMSChannel : public ChannelStrategy
{
public:
    bool sendNotification(Notification *notification) override
    {
        cout << " Sending notification via SMS to " << notification->getSender() << endl;
        return true;
    }
};

// OBSERVER BECAUSE MULTIPLE THINGS NEED TO BE DONE WHILE SENDING NOTIFICATION

class IObserver
{
public:
    virtual ~IObserver() = default;
    virtual void update(Notification *notification, ChannelStrategy *strategy) = 0;
};

class Logger : public IObserver
{
public:
    void update(Notification *notification)
    {
        cout << "Logging the notification" << notification->getDesc() << endl;
    }
};

class NotificationEnginer : public IObserver
{
public:
    void update(Notification *notification, ChannelStrategy *strategy)
    {
        strategy->sendNotification(notification);
        cout << "Done" << endl;
    }
};

class Obserable
{
public:
    virtual ~Obserable() = default;
    virtual void add(IObserver *observer) = 0;
    virtual void removeObserver(IObserver *observer) = 0;
    virtual void notify(Notification *notification, ChannelStrategy *strategy) = 0;
};

class NotificationObserver : public Obserable
{
    vector<IObserver *> observers;

public:
    void add(IObserver *observer) override
    {
        observers.push_back(observer);
    }

    void removeObserver(IObserver *observer) override
    {
        for (auto &i : observers)
        {
            if (observer == i)
            {
                swap(i, observers[observers.size() - 1]);
                observers.pop_back();
                break;
            }
        }
    }

    void notify(Notification *notification, ChannelStrategy *strategy) override
    {
        for (auto &i : observers)
        {
            i->update(notification, strategy);
        }
    }
};

class NotificationService
{
    vector<Notification *> notifications;
    Obserable *observerable;

    NotificationService()
    {
        observerable = new NotificationObserver();
    }

public:
    static NotificationService &getInstance()
    {
        static NotificationService instance;
        return instance;
    }

    void sendNotification(Notification *notification, ChannelStrategy *strategy)
    {
        notifications.push_back(notification);
        observerable->notify(notification, strategy);
    }
};

int main()
{
    // Write C++ code here
    std::cout << "Start small. Ship something.";

    return 0;
}

// entities-> Notification->sub,message, sender, receiver,
// retry->  proxy pattern
// different channels-> strategy

//======================================================================================

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <ctime>

using namespace std;

// ======================= ENUMS =======================

enum class NotificationStatus {
    PENDING,
    SENT,
    FAILED
};

// ======================= NOTIFICATION =======================

class Notification {
protected:
    string sender;
    string receiver;
    string subject;
    string body;
    NotificationStatus status;

public:
    Notification(string sender,
                 string receiver,
                 string subject,
                 string body)
        : sender(sender),
          receiver(receiver),
          subject(subject),
          body(body),
          status(NotificationStatus::PENDING) {}

    virtual ~Notification() = default;

    virtual string getBody() const {
        return body;
    }

    virtual string getSender() const {
        return sender;
    }

    virtual string getReceiver() const {
        return receiver;
    }

    virtual string getSubject() const {
        return subject;
    }

    NotificationStatus getStatus() const {
        return status;
    }

    void setStatus(NotificationStatus newStatus) {
        status = newStatus;
    }
};

class SimpleNotification : public Notification {
public:
    SimpleNotification(string sender,
                       string receiver,
                       string subject,
                       string body)
        : Notification(sender, receiver, subject, body) {}
};

// ======================= DECORATOR =======================

class NotificationDecorator : public Notification {
protected:
    shared_ptr<Notification> wrappedNotification;

public:
    NotificationDecorator(shared_ptr<Notification> notification)
        : Notification("", "", "", ""),
          wrappedNotification(notification) {}

    string getSender() const override {
        return wrappedNotification->getSender();
    }

    string getReceiver() const override {
        return wrappedNotification->getReceiver();
    }

    string getSubject() const override {
        return wrappedNotification->getSubject();
    }
};

class TimestampDecorator : public NotificationDecorator {
public:
    TimestampDecorator(shared_ptr<Notification> notification)
        : NotificationDecorator(notification) {}

    string getBody() const override {
        time_t now = time(nullptr);
        string timestamp = ctime(&now);
        timestamp.pop_back(); // remove newline
        return "[" + timestamp + "] " + wrappedNotification->getBody();
    }
};

class SignatureDecorator : public NotificationDecorator {
    string signature;

public:
    SignatureDecorator(shared_ptr<Notification> notification,
                       string signature)
        : NotificationDecorator(notification),
          signature(signature) {}

    string getBody() const override {
        return wrappedNotification->getBody() + "\n-- " + signature;
    }
};

// ======================= STRATEGY =======================

class NotificationChannel {
public:
    virtual ~NotificationChannel() = default;
    virtual bool send(shared_ptr<Notification> notification) = 0;
};

class EmailChannel : public NotificationChannel {
public:
    bool send(shared_ptr<Notification> notification) override {
        cout << "Sending EMAIL to "
             << notification->getReceiver() << endl;
        return true;
    }
};

class SMSChannel : public NotificationChannel {
public:
    bool send(shared_ptr<Notification> notification) override {
        cout << "Sending SMS to "
             << notification->getReceiver() << endl;
        return true;
    }
};

class PushChannel : public NotificationChannel {
public:
    bool send(shared_ptr<Notification> notification) override {
        cout << "Sending PUSH notification to "
             << notification->getReceiver() << endl;
        return true;
    }
};

// ======================= OBSERVER =======================

class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void update(shared_ptr<Notification> notification) = 0;
};

class Logger : public IObserver {
public:
    void update(shared_ptr<Notification> notification) override {
        cout << "LOG: Notification triggered -> "
             << notification->getBody() << endl;
    }
};

class NotificationEngine : public IObserver {
    vector<shared_ptr<NotificationChannel>> channels;

public:
    void addChannel(shared_ptr<NotificationChannel> channel) {
        channels.push_back(channel);
    }

    void update(shared_ptr<Notification> notification) override {
        bool success = true;

        for (auto& channel : channels) {
            success &= channel->send(notification);
        }

        if (success) {
            notification->setStatus(NotificationStatus::SENT);
            cout << "All notifications sent successfully.\n";
        } else {
            notification->setStatus(NotificationStatus::FAILED);
            cout << "Notification sending failed.\n";
        }
    }
};

// ======================= OBSERVABLE =======================

class Observable {
    vector<shared_ptr<IObserver>> observers;

public:
    void addObserver(shared_ptr<IObserver> observer) {
        observers.push_back(observer);
    }

    void removeObserver(shared_ptr<IObserver> observer) {
        observers.erase(
            remove(observers.begin(), observers.end(), observer),
            observers.end()
        );
    }

    void notify(shared_ptr<Notification> notification) {
        for (auto& observer : observers) {
            observer->update(notification);
        }
    }
};

// ======================= SINGLETON SERVICE =======================

class NotificationService {
private:
    Observable observable;
    vector<shared_ptr<Notification>> notificationHistory;

    NotificationService() = default;

public:
    NotificationService(const NotificationService&) = delete;
    NotificationService& operator=(const NotificationService&) = delete;

    static NotificationService& getInstance() {
        static NotificationService instance;
        return instance;
    }

    void addObserver(shared_ptr<IObserver> observer) {
        observable.addObserver(observer);
    }

    void sendNotification(shared_ptr<Notification> notification) {
        notificationHistory.push_back(notification);
        observable.notify(notification);
    }

    void showHistory() {
        cout << "\n===== Notification History =====\n";
        for (auto& notification : notificationHistory) {
            cout << notification->getSubject()
                 << " -> "
                 << notification->getBody()
                 << endl;
        }
    }
};

// ======================= MAIN =======================

int main() {

    NotificationService& service =
        NotificationService::getInstance();

    // observers
    auto logger = make_shared<Logger>();

    auto engine = make_shared<NotificationEngine>();
    engine->addChannel(make_shared<EmailChannel>());
    engine->addChannel(make_shared<SMSChannel>());
    engine->addChannel(make_shared<PushChannel>());

    service.addObserver(logger);
    service.addObserver(engine);

    // create notification
    shared_ptr<Notification> notification =
        make_shared<SimpleNotification>(
            "Amazon",
            "user@gmail.com",
            "Order Update",
            "Your order has been shipped"
        );

    // decorators
    notification = make_shared<TimestampDecorator>(notification);
    notification = make_shared<SignatureDecorator>(
        notification,
        "Amazon Team"
    );

    service.sendNotification(notification);

    service.showHistory();

    return 0;
}


//================================================================================================================

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <ctime>
#include <unordered_map>

using namespace std;

// ======================= ENUMS =======================

enum class NotificationStatus {
    PENDING,
    SENT,
    FAILED
};

enum class ChannelType {
    EMAIL,
    SMS,
    PUSH
};

// ======================= NOTIFICATION =======================

class Notification {
protected:
    string sender;
    string receiver;
    string subject;
    string body;

    NotificationStatus status;

    vector<ChannelType> selectedChannels;

public:
    Notification(string sender,
                 string receiver,
                 string subject,
                 string body,
                 vector<ChannelType> channels)
        : sender(sender),
          receiver(receiver),
          subject(subject),
          body(body),
          selectedChannels(channels),
          status(NotificationStatus::PENDING) {}

    virtual ~Notification() = default;

    virtual string getBody() const {
        return body;
    }

    virtual string getSender() const {
        return sender;
    }

    virtual string getReceiver() const {
        return receiver;
    }

    virtual string getSubject() const {
        return subject;
    }

    vector<ChannelType> getChannels() const {
        return selectedChannels;
    }

    NotificationStatus getStatus() const {
        return status;
    }

    void setStatus(NotificationStatus newStatus) {
        status = newStatus;
    }
};

class SimpleNotification : public Notification {
public:
    SimpleNotification(string sender,
                       string receiver,
                       string subject,
                       string body,
                       vector<ChannelType> channels)
        : Notification(sender,
                       receiver,
                       subject,
                       body,
                       channels) {}
};

// ======================= DECORATOR =======================

class NotificationDecorator : public Notification {
protected:
    shared_ptr<Notification> wrappedNotification;

public:
    NotificationDecorator(shared_ptr<Notification> notification)
        : Notification("", "", "", "", {}),
          wrappedNotification(notification) {}

    string getSender() const override {
        return wrappedNotification->getSender();
    }

    string getReceiver() const override {
        return wrappedNotification->getReceiver();
    }

    string getSubject() const override {
        return wrappedNotification->getSubject();
    }

    vector<ChannelType> getChannels() const {
        return wrappedNotification->getChannels();
    }
};

class TimestampDecorator : public NotificationDecorator {
public:
    TimestampDecorator(shared_ptr<Notification> notification)
        : NotificationDecorator(notification) {}

    string getBody() const override {

        time_t now = time(nullptr);

        string timestamp = ctime(&now);

        timestamp.pop_back();

        return "[" + timestamp + "] "
               + wrappedNotification->getBody();
    }
};

class SignatureDecorator : public NotificationDecorator {

    string signature;

public:
    SignatureDecorator(shared_ptr<Notification> notification,
                       string signature)
        : NotificationDecorator(notification),
          signature(signature) {}

    string getBody() const override {

        return wrappedNotification->getBody()
               + "\n-- "
               + signature;
    }
};

// ======================= STRATEGY =======================

class NotificationChannel {
public:
    virtual ~NotificationChannel() = default;

    virtual bool send(shared_ptr<Notification> notification) = 0;
};

class EmailChannel : public NotificationChannel {
public:
    bool send(shared_ptr<Notification> notification) override {

        cout << "Sending EMAIL to "
             << notification->getReceiver()
             << endl;

        return true;
    }
};

class SMSChannel : public NotificationChannel {
public:
    bool send(shared_ptr<Notification> notification) override {

        cout << "Sending SMS to "
             << notification->getReceiver()
             << endl;

        return true;
    }
};

class PushChannel : public NotificationChannel {
public:
    bool send(shared_ptr<Notification> notification) override {

        cout << "Sending PUSH notification to "
             << notification->getReceiver()
             << endl;

        return true;
    }
};

// ======================= OBSERVER =======================

class IObserver {
public:
    virtual ~IObserver() = default;

    virtual void update(shared_ptr<Notification> notification) = 0;
};

class Logger : public IObserver {
public:
    void update(shared_ptr<Notification> notification) override {

        cout << "LOG: "
             << notification->getBody()
             << endl;
    }
};

// ======================= NOTIFICATION ENGINE =======================

class NotificationEngine : public IObserver {

    unordered_map<
        ChannelType,
        shared_ptr<NotificationChannel>
    > channelMap;

public:

    void registerChannel(ChannelType type,
                         shared_ptr<NotificationChannel> strategy) {

        channelMap[type] = strategy;
    }

    void update(shared_ptr<Notification> notification) override {

        bool success = true;

        vector<ChannelType> channels =
            notification->getChannels();

        for (auto channelType : channels) {

            if (channelMap.find(channelType)
                != channelMap.end()) {

                success &=
                    channelMap[channelType]
                        ->send(notification);
            }
        }

        if (success) {

            notification->setStatus(
                NotificationStatus::SENT
            );

            cout << "Notification sent successfully.\n";

        } else {

            notification->setStatus(
                NotificationStatus::FAILED
            );

            cout << "Notification failed.\n";
        }
    }
};

// ======================= OBSERVABLE =======================

class Observable {

    vector<shared_ptr<IObserver>> observers;

public:

    void addObserver(shared_ptr<IObserver> observer) {

        observers.push_back(observer);
    }

    void notify(shared_ptr<Notification> notification) {

        for (auto& observer : observers) {

            observer->update(notification);
        }
    }
};

// ======================= SINGLETON SERVICE =======================

class NotificationService {

private:

    Observable observable;

    vector<shared_ptr<Notification>>
        notificationHistory;

    NotificationService() = default;

public:

    NotificationService(
        const NotificationService&
    ) = delete;

    NotificationService& operator=(
        const NotificationService&
    ) = delete;

    static NotificationService&
    getInstance() {

        static NotificationService instance;

        return instance;
    }

    void addObserver(shared_ptr<IObserver> observer) {

        observable.addObserver(observer);
    }

    void sendNotification(
        shared_ptr<Notification> notification
    ) {

        notificationHistory.push_back(notification);

        observable.notify(notification);
    }

    void showHistory() {

        cout << "\n===== Notification History =====\n";

        for (auto& notification : notificationHistory) {

            cout << notification->getSubject()
                 << " -> "
                 << notification->getBody()
                 << endl;
        }
    }
};

// ======================= MAIN =======================

int main() {

    NotificationService& service =
        NotificationService::getInstance();

    // LOGGER

    auto logger = make_shared<Logger>();

    // ENGINE

    auto engine =
        make_shared<NotificationEngine>();

    engine->registerChannel(
        ChannelType::EMAIL,
        make_shared<EmailChannel>()
    );

    engine->registerChannel(
        ChannelType::SMS,
        make_shared<SMSChannel>()
    );

    engine->registerChannel(
        ChannelType::PUSH,
        make_shared<PushChannel>()
    );

    service.addObserver(logger);
    service.addObserver(engine);

    // USER SELECTS CHANNELS

    vector<ChannelType> selectedChannels = {
        ChannelType::EMAIL,
        ChannelType::SMS
    };

    // CREATE NOTIFICATION

    shared_ptr<Notification> notification =

        make_shared<SimpleNotification>(
            "Amazon",
            "user@gmail.com",
            "Order Update",
            "Your order has been shipped",
            selectedChannels
        );

    // DECORATORS

    notification =
        make_shared<TimestampDecorator>(
            notification
        );

    notification =
        make_shared<SignatureDecorator>(
            notification,
            "Amazon Team"
        );

    // SEND

    service.sendNotification(notification);

    service.showHistory();

    return 0;
}

// for selecting channels, we can have a UI where user can select the channels and then we can pass that to notification object. This way we can avoid having multiple notification objects for different channels.