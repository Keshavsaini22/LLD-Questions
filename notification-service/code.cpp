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