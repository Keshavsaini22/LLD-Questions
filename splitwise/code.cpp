#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <queue>
#include <string>
#include <stdexcept>
#include <cmath>

using namespace std;

class User
{
private:
    int id;
    string name;

public:
    User(int id, const string &name)
        : id(id), name(name)
    {
    }

    int getId() const
    {
        return id;
    }

    string getName() const
    {
        return name;
    }
};

class Split  //Kisda kina hissa in expense
{
private:
    shared_ptr<User> user;
    double shareAmount;

public:
    Split(shared_ptr<User> user, double shareAmount)
        : user(user), shareAmount(shareAmount)
    {
    }

    shared_ptr<User> getUser() const
    {
        return user;
    }

    double getShareAmount() const
    {
        return shareAmount;
    }
};

class Expense
{
private:
    int expenseId;
    string description;
    double amount;
    shared_ptr<User> paidBy;
    vector<Split> splits;

public:
    Expense(int expenseId, const string &description, double amount, shared_ptr<User> paidBy, const vector<Split> &splits)
        : expenseId(expenseId), description(description), amount(amount), paidBy(paidBy), splits(splits)
    {
    }

    int getExpenseId() const
    {
        return expenseId;
    }

    double getAmount() const
    {
        return amount;
    }

    const string &getDescription() const
    {
        return description;
    }

    shared_ptr<User> getPaidBy() const
    {
        return paidBy;
    }

    const vector<Split> &getSplits() const
    {
        return splits;
    }
};

class Payment
{
private:
    int paymentId;
    shared_ptr<User> fromUser;
    shared_ptr<User> toUser;
    double amount;

public:
    Payment(int paymentId, shared_ptr<User> fromUser, shared_ptr<User> toUser, double amount)
        : paymentId(paymentId), fromUser(fromUser), toUser(toUser), amount(amount)
    {
    }

    shared_ptr<User> getFromUser() const
    {
        return fromUser;
    }

    shared_ptr<User> getToUser() const
    {
        return toUser;
    }

    double getAmount() const
    {
        return amount;
    }
};

//======================================================
// SPLIT STRATEGY
//======================================================

class ExpenseSplitStrategy
{
public:
    virtual vector<Split> calculateSplits(double amount, const vector<shared_ptr<User>> &users, const vector<double> &metadata = {}) = 0;
    virtual ~ExpenseSplitStrategy() = default;
};

class EqualSplitStrategy : public ExpenseSplitStrategy
{
public:
    vector<Split> calculateSplits(double amount, const vector<shared_ptr<User>> &users, const vector<double> &metadata = {}) override
    {
        vector<Split> splits;

        if (users.empty())
        {
            throw runtime_error("No users provided");
        }

        double share = amount / users.size();

        for (const auto &user : users)
        {
            splits.emplace_back(user, share);
        }

        return splits;
    }
};

class ExactSplitStrategy : public ExpenseSplitStrategy
{
public:
    vector<Split> calculateSplits(double amount, const vector<shared_ptr<User>> &users, const vector<double> &amounts) override
    {
        if (users.size() != amounts.size())
        {
            throw runtime_error("Invalid exact split");
        }

        double total = 0;
        for (double val : amounts)
        {
            total += val;
        }

        if (abs(total - amount) > 0.001)
        {
            throw runtime_error("Exact split mismatch");
        }

        vector<Split> splits;
        for (size_t i = 0; i < users.size(); i++)
        {
            splits.emplace_back(users[i], amounts[i]);
        }

        return splits;
    }
};

class PercentageSplitStrategy : public ExpenseSplitStrategy
{
public:
    vector<Split> calculateSplits(double amount, const vector<shared_ptr<User>> &users, const vector<double> &percentages) override
    {
        if (users.size() != percentages.size())
        {
            throw runtime_error("Invalid percentage split");
        }

        double totalPercent = 0;
        for (double percentage : percentages)
        {
            totalPercent += percentage;
        }

        if (abs(totalPercent - 100.0) > 0.001)
        {
            throw runtime_error("Percentage must sum to 100");
        }

        vector<Split> splits;
        for (size_t i = 0; i < users.size(); i++)
        {
            splits.emplace_back(users[i], amount * percentages[i] / 100.0);
        }

        return splits;
    }
};

//======================================================
// GROUP
//======================================================

class Group
{
private:
    int groupId;
    string name;
    vector<shared_ptr<User>> users;
    vector<shared_ptr<Expense>> expenses;
    vector<shared_ptr<Payment>> payments;

    // balances[A][B] = x
    // B owes A x
    unordered_map<int, unordered_map<int, double>> balances;

public:
    Group(int groupId, const string &name)
        : groupId(groupId), name(name)
    {
    }

    int getGroupId() const
    {
        return groupId;
    }

    string getName() const
    {
        return name;
    }

    const vector<shared_ptr<User>> &getUsers() const
    {
        return users;
    }

    const vector<shared_ptr<Expense>> &getExpenses() const
    {
        return expenses;
    }

    void addUser(shared_ptr<User> user)
    {
        users.push_back(user);
    }

    const unordered_map<int, unordered_map<int, double>> &getBalances() const
    {
        return balances;
    }

    void addExpense(shared_ptr<Expense> expense) //I can also get raw data in this function and make the expense inside it and send strategy also
    {
        expenses.push_back(expense);

        auto paidBy = expense->getPaidBy();
        int payerId = paidBy->getId();

        for (const auto &split : expense->getSplits())
        {
            int userId = split.getUser()->getId();
            double share = split.getShareAmount();

            if (userId == payerId)
            {
                continue;
            }

            balances[payerId][userId] += share;
        }
    }

    void recordPayment(shared_ptr<User> fromUser, shared_ptr<User> toUser, double amount) //when user A pays to B
    {
        int fromId = fromUser->getId();
        int toId = toUser->getId();

        double debt = balances[toId][fromId];
        double actualPayment = min(debt, amount);

        balances[toId][fromId] -= actualPayment;

        if (abs(balances[toId][fromId]) < 0.001)
        {
            balances[toId].erase(fromId);
        }

        payments.push_back(make_shared<Payment>(payments.size() + 1, fromUser, toUser, actualPayment));
    }

    void showBalances() const
    {
        cout << "\n===== BALANCES =====\n";

        for (const auto &[creditor, debtors] : balances)
        {
            for (const auto &[debtor, amount] : debtors)
            {
                if (amount <= 0)
                {
                    continue;
                }

                cout << debtor << " owes " << creditor << " : " << amount << endl;
            }
        }
    }
};

//======================================================
// NOTIFICATION CHANNEL
//======================================================

class INotificationChannel
{
public:
    virtual ~INotificationChannel() = default;
    virtual void send(const User &user, const string &message) = 0;
};

class EmailNotification : public INotificationChannel
{
public:
    void send(const User &user, const string &message) override
    {
        cout << "[EMAIL] " << user.getName() << " : " << message << endl;
    }
};

class SMSNotification : public INotificationChannel
{
public:
    void send(const User &user, const string &message) override
    {
        cout << "[SMS] " << user.getName() << " : " << message << endl;
    }
};

//======================================================
// NOTIFICATION SERVICE
//======================================================

class NotificationService
{
private:
    vector<shared_ptr<INotificationChannel>> channels;

public:
    void addChannel(shared_ptr<INotificationChannel> channel)
    {
        channels.push_back(channel);
    }

    void notifyUser(const User &user, const string &message)
    {
        for (auto &channel : channels)
        {
            channel->send(user, message);
        }
    }

    void notifyUsers(const vector<shared_ptr<User>> &users, const string &message)
    {
        for (auto &user : users)
        {
            notifyUser(*user, message);
        }
    }
};

//======================================================
// TRANSACTION
//======================================================

class Transaction
{
private:
    shared_ptr<User> from;
    shared_ptr<User> to;
    double amount;

public:
    Transaction(shared_ptr<User> from, shared_ptr<User> to, double amount)
        : from(from), to(to), amount(amount)
    {
    }

    shared_ptr<User> getFrom() const
    {
        return from;
    }

    shared_ptr<User> getTo() const
    {
        return to;
    }

    double getAmount() const
    {
        return amount;
    }
};

class SettlementService
{
public:
    vector<Transaction> settle(const Group &group)
    {
        unordered_map<int, double> netBalance;
        unordered_map<int, shared_ptr<User>> userMap;

        // Initialize users
        for (const auto &user : group.getUsers())
        {
            netBalance[user->getId()] = 0;
            userMap[user->getId()] = user;
        }

        // Build net balances
        // balances[A][B] = X
        // means B owes A X
        const auto &balances = group.getBalances();
        for (const auto &[creditorId, debtors] : balances)
        {
            for (const auto &[debtorId, amount] : debtors)
            {
                netBalance[creditorId] += amount;
                netBalance[debtorId] -= amount;
            }
        }

        priority_queue<pair<double, int>> creditors;
        priority_queue<pair<double, int>> debtors;

        // Create creditor and debtor heaps
        for (const auto &[userId, balance] : netBalance)
        {
            if (balance > 0.001)
            {
                creditors.push({balance, userId});
            }
            else if (balance < -0.001)
            {
                debtors.push({-balance, userId});
            }
        }

        vector<Transaction> transactions;

        while (!creditors.empty() && !debtors.empty())
        {
            auto [creditAmount, creditorId] = creditors.top();
            creditors.pop();

            auto [debtAmount, debtorId] = debtors.top();
            debtors.pop();

            double settlementAmount = min(creditAmount, debtAmount);
            transactions.emplace_back(userMap[debtorId], userMap[creditorId], settlementAmount);

            creditAmount -= settlementAmount;
            debtAmount -= settlementAmount;

            if (creditAmount > 0.001)
            {
                creditors.push({creditAmount, creditorId});
            }

            if (debtAmount > 0.001)
            {
                debtors.push({debtAmount, debtorId});
            }
        }

        return transactions;
    }
};

//======================================================
// SPLITWISE FACADE
//======================================================

class SplitwiseFacade
{
private:
    NotificationService &notificationService;
    SettlementService &settlementService;
    unordered_map<int, shared_ptr<Group>> groups;

public:
    SplitwiseFacade(NotificationService &notificationService, SettlementService &settlementService)
        : notificationService(notificationService), settlementService(settlementService)
    {
    }

    void addGroup(shared_ptr<Group> group)
    {
        groups[group->getGroupId()] = group;
    }

    shared_ptr<Group> getGroup(int groupId)
    {
        if (!groups.count(groupId))
        {
            throw runtime_error("Group not found");
        }

        return groups[groupId];
    }

    void addUserToGroup(int groupId, shared_ptr<User> user)
    {
        auto group = getGroup(groupId);
        group->addUser(user);

        notificationService.notifyUser(*user, "Added to group : " + group->getName());
    }

    void addExpense(int groupId, shared_ptr<Expense> expense)
    {
        auto group = getGroup(groupId);
        group->addExpense(expense);

        notificationService.notifyUsers(group->getUsers(), "New expense added : " + expense->getDescription());
    }

    void recordPayment(int groupId, shared_ptr<User> fromUser, shared_ptr<User> toUser, double amount)
    {
        auto group = getGroup(groupId);
        group->recordPayment(fromUser, toUser, amount);

        notificationService.notifyUser(*fromUser, "Payment recorded");
        notificationService.notifyUser(*toUser, "Payment received");
    }

    void showBalances(int groupId)
    {
        auto group = getGroup(groupId);
        group->showBalances();
    }

    void settleGroup(int groupId)
    {
        auto group = getGroup(groupId);
        auto settlements = settlementService.settle(*group);

        cout << "\n===== SETTLEMENT PLAN =====\n";
        for (auto &transaction : settlements)
        {
            cout << transaction.getFrom()->getName() << " pays " << transaction.getTo()->getName() << " : " << transaction.getAmount() << endl;
        }
    }
};

int main()
{
    // ==========================================
    // Notification Setup
    // ==========================================
    NotificationService notificationService;
    notificationService.addChannel(make_shared<EmailNotification>());
    notificationService.addChannel(make_shared<SMSNotification>());

    // ==========================================
    // Settlement Service
    // ==========================================
    SettlementService settlementService;

    // ==========================================
    // Facade
    // ==========================================
    SplitwiseFacade splitwise(notificationService, settlementService);

    // ==========================================
    // Users
    // ==========================================
    auto alpha = make_shared<User>(1, "Alpha");
    auto rahul = make_shared<User>(2, "Rahul");
    auto aman = make_shared<User>(3, "Aman");

    // ==========================================
    // Group
    // ==========================================
    auto tripGroup = make_shared<Group>(1, "Goa Trip");
    splitwise.addGroup(tripGroup);

    splitwise.addUserToGroup(1, alpha);
    splitwise.addUserToGroup(1, rahul);
    splitwise.addUserToGroup(1, aman);

    // ==========================================
    // Expense 1
    // Alpha paid 1200
    // Equal split
    // ==========================================
    EqualSplitStrategy equalStrategy;
    vector<shared_ptr<User>> users = {alpha, rahul, aman};

    auto splits1 = equalStrategy.calculateSplits(1200, users);
    auto expense1 = make_shared<Expense>(1, "Dinner", 1200, alpha, splits1);
    splitwise.addExpense(1, expense1);

    // ==========================================
    // Expense 2
    // Rahul paid 600
    // Equal split
    // ==========================================
    auto splits2 = equalStrategy.calculateSplits(600, users);
    auto expense2 = make_shared<Expense>(2, "Scooter Rent", 600, rahul, splits2);
    splitwise.addExpense(1, expense2);

    // ==========================================
    // Show Balances
    // ==========================================
    cout << "\n";
    splitwise.showBalances(1);

    // ==========================================
    // Settlement Plan
    // ==========================================
    cout << "\n";
    splitwise.settleGroup(1);

    // ==========================================
    // Aman pays Alpha 200
    // ==========================================
    splitwise.recordPayment(1, aman, alpha, 200);

    // ==========================================
    // Updated Balances
    // ==========================================
    cout << "\nAfter Payment\n";
    splitwise.showBalances(1);

    return 0;
}