#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <ctime>
#include <stdexcept>
#include <algorithm>

using namespace std;

// ENUMS

enum class TransactionType {
    WITHDRAW,
    CHECK_BALANCE
};

enum class TransactionStatus {
    SUCCESS,
    FAILED
};

string toString(TransactionType type) {
    switch (type) {
        case TransactionType::WITHDRAW:
            return "WITHDRAW";
        case TransactionType::CHECK_BALANCE:
            return "CHECK_BALANCE";
    }
    return "";
}

string toString(TransactionStatus status) {
    switch (status) {
        case TransactionStatus::SUCCESS:
            return "SUCCESS";
        case TransactionStatus::FAILED:
            return "FAILED";
    }
    return "";
}

// CORE ENTITIES

class Transaction {
public:
    string id;
    time_t timestamp;
    TransactionType type;
    double amount;
    TransactionStatus status;

    Transaction(TransactionType type, double amount, TransactionStatus status)
        : type(type), amount(amount), status(status) {

        timestamp = time(nullptr);
        id = "TXN-" + to_string(timestamp) + "-" + to_string(rand() % 10000);
    }
};

class Account {
private:
    double balance;
    vector<Transaction> transactions;

public:
    string accountNumber;

    Account(string accNo, double bal)
        : accountNumber(accNo), balance(bal) {}

    void withdraw(double amount) {
        if (amount > balance) {
            throw runtime_error("Insufficient balance");
        }

        balance -= amount;
    }

    double getBalance() const {
        return balance;
    }

    void addTransaction(const Transaction& txn) {
        transactions.push_back(txn);
    }

    const vector<Transaction>& getTransactions() const {
        return transactions;
    }
};

class Card {
private:
    string pin;

public:
    string cardNumber;
    Account* account;

    Card(string cardNo, Account* acc, string pin)
        : cardNumber(cardNo), account(acc), pin(pin) {}

    bool validatePin(const string& inputPin) {
        return pin == inputPin;
    }
};

class CashInventory {
private:
    map<int, int, greater<int>> notes;

public:
    void addCash(int denomination, int count) {
        notes[denomination] += count;
    }

    bool hasSufficientCash(int amount) {
        int remaining = amount;

        for (auto& entry : notes) {
            int denom = entry.first;
            int count = entry.second;

            int used = min(remaining / denom, count);
            remaining -= used * denom;
        }

        return remaining == 0;
    }

    void deductCash(int amount) {
        if (!hasSufficientCash(amount)) {
            throw runtime_error("ATM has insufficient cash");
        }

        int remaining = amount;

        for (auto& entry : notes) {
            int denom = entry.first;
            int count = entry.second;

            int used = min(remaining / denom, count);

            notes[denom] -= used;
            remaining -= used * denom;
        }
    }
};

class CashDispenser {
private:
    CashInventory* inventory;

public:
    CashDispenser(CashInventory* inv) : inventory(inv) {}

    void dispense(int amount) {
        inventory->deductCash(amount);
        cout << "Dispensed ₹" << amount << endl;
    }
};

// FORWARD DECLARATION

class ATM;

// STATE INTERFACE

class ATMState {
public:
    virtual void insertCard(ATM* atm, Card* card) = 0;
    virtual void enterPin(ATM* atm, const string& pin) = 0;
    virtual void selectOperation(ATM* atm, TransactionType type) = 0;
    virtual void withdraw(ATM* atm, int amount) = 0;
    virtual void checkBalance(ATM* atm) = 0;
    virtual void ejectCard(ATM* atm) = 0;

    virtual ~ATMState() = default;
};

// CONTEXT

class ATM {
private:
    ATMState* state;
    Card* currentCard;

public:
    CashInventory* cashInventory;
    CashDispenser* dispenser;

    ATM(CashInventory* inventory, CashDispenser* dispenser);

    void setState(ATMState* newState) {
        state = newState;
    }

    void setCard(Card* card) {
        currentCard = card;
    }

    Card* getCard() {
        if (!currentCard) {
            throw runtime_error("No card inserted");
        }

        return currentCard;
    }

    void insertCard(Card* card) {
        state->insertCard(this, card);
    }

    void enterPin(const string& pin) {
        state->enterPin(this, pin);
    }

    void selectOperation(TransactionType type) {
        state->selectOperation(this, type);
    }

    void withdraw(int amount) {
        state->withdraw(this, amount);
    }

    void checkBalance() {
        state->checkBalance(this);
    }

    void ejectCard() {
        state->ejectCard(this);
    }

    void printTransactionHistory() {
        auto transactions = getCard()->account->getTransactions();

        cout << "\n----- Transaction History -----\n";

        for (const auto& txn : transactions) {
            cout << "ID: " << txn.id
                 << " | Type: " << toString(txn.type)
                 << " | Amount: ₹" << txn.amount
                 << " | Status: " << toString(txn.status)
                 << " | Time: " << ctime(&txn.timestamp);
        }
    }
};

// CONCRETE STATES

class IdleState : public ATMState {
public:
    void insertCard(ATM* atm, Card* card) override;

    void enterPin(ATM*, const string&) override {
        throw runtime_error("Insert card first");
    }

    void selectOperation(ATM*, TransactionType) override {
        throw runtime_error("Insert card first");
    }

    void withdraw(ATM*, int) override {
        throw runtime_error("Insert card first");
    }

    void checkBalance(ATM*) override {
        throw runtime_error("Insert card first");
    }

    void ejectCard(ATM*) override {
        throw runtime_error("No card to eject");
    }
};

class HasCardState : public ATMState {
public:
    void insertCard(ATM*, Card*) override {
        throw runtime_error("Card already inserted");
    }

    void enterPin(ATM* atm, const string& pin) override;

    void selectOperation(ATM*, TransactionType) override {
        throw runtime_error("Enter PIN first");
    }

    void withdraw(ATM*, int) override {
        throw runtime_error("Enter PIN first");
    }

    void checkBalance(ATM*) override {
        throw runtime_error("Enter PIN first");
    }

    void ejectCard(ATM* atm) override;
};

class SelectOperationState : public ATMState {
public:
    void insertCard(ATM*, Card*) override {
        throw runtime_error("Operation in progress");
    }

    void enterPin(ATM*, const string&) override {
        throw runtime_error("Already validated");
    }

    void selectOperation(ATM* atm, TransactionType type) override;

    void withdraw(ATM*, int) override {
        throw runtime_error("Select operation first");
    }

    void checkBalance(ATM*) override {
        throw runtime_error("Select operation first");
    }

    void ejectCard(ATM* atm) override;
};

class WithdrawState : public ATMState {
public:
    void insertCard(ATM*, Card*) override {
        throw runtime_error("Invalid state");
    }

    void enterPin(ATM*, const string&) override {
        throw runtime_error("Invalid state");
    }

    void selectOperation(ATM*, TransactionType) override {
        throw runtime_error("Invalid state");
    }

    void checkBalance(ATM*) override {
        throw runtime_error("Invalid state");
    }

    void withdraw(ATM* atm, int amount) override;

    void ejectCard(ATM* atm) override;
};

class CheckBalanceState : public ATMState {
public:
    void insertCard(ATM*, Card*) override {
        throw runtime_error("Invalid state");
    }

    void enterPin(ATM*, const string&) override {
        throw runtime_error("Invalid state");
    }

    void selectOperation(ATM*, TransactionType) override {
        throw runtime_error("Invalid state");
    }

    void withdraw(ATM*, int) override {
        throw runtime_error("Invalid state");
    }

    void checkBalance(ATM* atm) override;

    void ejectCard(ATM* atm) override;
};

// ATM IMPLEMENTATION

ATM::ATM(CashInventory* inventory, CashDispenser* dispenser)
    : cashInventory(inventory), dispenser(dispenser), currentCard(nullptr) {
    state = new IdleState();
}

// STATE METHODS

void IdleState::insertCard(ATM* atm, Card* card) {
    atm->setCard(card);
    cout << "Card inserted\n";
    atm->setState(new HasCardState());
}

void HasCardState::enterPin(ATM* atm, const string& pin) {
    Card* card = atm->getCard();

    if (!card->validatePin(pin)) {
        throw runtime_error("Invalid PIN");
    }

    cout << "PIN validated\n";
    atm->setState(new SelectOperationState());
}

void HasCardState::ejectCard(ATM* atm) {
    atm->setCard(nullptr);
    atm->setState(new IdleState());

    cout << "Card ejected\n";
}

void SelectOperationState::selectOperation(ATM* atm, TransactionType type) {
    if (type == TransactionType::WITHDRAW) {
        atm->setState(new WithdrawState());
    } else {
        atm->setState(new CheckBalanceState());
    }
}

void SelectOperationState::ejectCard(ATM* atm) {
    atm->setCard(nullptr);
    atm->setState(new IdleState());
}

void WithdrawState::withdraw(ATM* atm, int amount) {
    Card* card = atm->getCard();
    Account* account = card->account;

    try {
        if (!atm->cashInventory->hasSufficientCash(amount)) {
            throw runtime_error("ATM insufficient cash");
        }

        account->withdraw(amount);

        atm->dispenser->dispense(amount);

        Transaction txn(
            TransactionType::WITHDRAW,
            amount,
            TransactionStatus::SUCCESS
        );

        account->addTransaction(txn);

        cout << "Withdrawal successful\n";

    } catch (exception& e) {

        Transaction txn(
            TransactionType::WITHDRAW,
            amount,
            TransactionStatus::FAILED
        );

        account->addTransaction(txn);

        throw;
    }

    atm->setState(new SelectOperationState());
}

void WithdrawState::ejectCard(ATM* atm) {
    atm->setCard(nullptr);
    atm->setState(new IdleState());
}

void CheckBalanceState::checkBalance(ATM* atm) {
    Account* account = atm->getCard()->account;

    double balance = account->getBalance();

    Transaction txn(
        TransactionType::CHECK_BALANCE,
        0,
        TransactionStatus::SUCCESS
    );

    account->addTransaction(txn);

    cout << "Available Balance: ₹" << balance << endl;

    atm->setState(new SelectOperationState());
}

void CheckBalanceState::ejectCard(ATM* atm) {
    atm->setCard(nullptr);
    atm->setState(new IdleState());
}

// MAIN

int main() {

    Account account("ACC123", 10000);

    Card card("CARD123", &account, "1234");

    CashInventory inventory;

    inventory.addCash(2000, 10);
    inventory.addCash(500, 20);

    CashDispenser dispenser(&inventory);

    ATM atm(&inventory, &dispenser);

    try {

        atm.insertCard(&card);

        atm.enterPin("1234");

        atm.selectOperation(TransactionType::WITHDRAW);

        atm.withdraw(3000);

        atm.selectOperation(TransactionType::CHECK_BALANCE);

        atm.checkBalance();

        atm.printTransactionHistory();

        atm.ejectCard();

    } catch (exception& e) {

        cout << "Error: " << e.what() << endl;
    }

    return 0;
}