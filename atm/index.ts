enum TransactionType {
    WITHDRAW = "WITHDRAW",
    CHECK_BALANCE = "CHECK_BALANCE",
}

enum TransactionStatus {
    SUCCESS = "SUCCESS",
    FAILED = "FAILED",
}

//CORE ENTITIES

class Transaction {
    public readonly id: string;
    public readonly timestamp: Date;

    constructor(
        public type: TransactionType,
        public amount: number,
        public status: TransactionStatus
    ) {
        this.id = `TXN-${Date.now()}-${Math.random()}`;
        this.timestamp = new Date();
    }
}

class Account {
    private transactions: Transaction[] = [];

    constructor(
        public accountNumber: string,
        private balance: number
    ) { }

    withdraw(amount: number) {
        if (amount > this.balance) {
            throw new Error("Insufficient balance");
        }
        this.balance -= amount;
    }

    getBalance(): number {
        return this.balance;
    }

    addTransaction(transaction: Transaction) {
        this.transactions.push(transaction);
    }

    getTransactions(): Transaction[] {
        return this.transactions;
    }
}

class Card {
    constructor(
        public cardNumber: string,
        public account: Account,
        public pin: string
    ) { }

    validatePin(inputPin: string): boolean {
        return this.pin === inputPin;
    }
}

class CashInventory {
    private notes: Map<number, number> = new Map(); // denomination -> count

    addCash(denomination: number, count: number) {
        this.notes.set(
            denomination,
            (this.notes.get(denomination) || 0) + count
        );
    }

    hasSufficientCash(amount: number): boolean {
        let remaining = amount;
        const denominations = Array.from(this.notes.keys()).sort((a, b) => b - a);

        for (const denom of denominations) {
            const count = this.notes.get(denom)!;
            const used = Math.min(Math.floor(remaining / denom), count);
            remaining -= used * denom;
        }

        return remaining === 0;
    }

    deductCash(amount: number) {
        if (!this.hasSufficientCash(amount)) {
            throw new Error("ATM has insufficient cash");
        }

        let remaining = amount;
        const denominations = Array.from(this.notes.keys()).sort((a, b) => b - a);

        for (const denom of denominations) {
            const count = this.notes.get(denom)!;
            const used = Math.min(Math.floor(remaining / denom), count);
            this.notes.set(denom, count - used);
            remaining -= used * denom;
        }
    }
}

class CashDispenser {
    constructor(private inventory: CashInventory) { }

    dispense(amount: number) {
        this.inventory.deductCash(amount);
        console.log(`Dispensed ₹${amount}`);
    }
}



//STATE DESIGN PATTERN

//State interface
interface ATMState {
    insertCard(atm: ATM, card: Card): void;
    enterPin(atm: ATM, pin: string): void;
    selectOperation(atm: ATM, type: TransactionType): void;
    withdraw(atm: ATM, amount: number): void;
    checkBalance(atm: ATM): void;
    ejectCard(atm: ATM): void;
}

//Context
class ATM {
    private state: ATMState;
    private currentCard: Card | null = null;

    constructor(
        public cashInventory: CashInventory,
        public dispenser: CashDispenser
    ) {
        this.state = new IdleState();
    }

    setState(state: ATMState) {
        this.state = state;
    }

    setCard(card: Card | null) {
        this.currentCard = card;
    }

    getCard(): Card {
        if (!this.currentCard) throw new Error("No card inserted");
        return this.currentCard;
    }

    insertCard(card: Card) {
        this.state.insertCard(this, card);
    }

    enterPin(pin: string) {
        this.state.enterPin(this, pin);
    }

    selectOperation(type: TransactionType) {
        this.state.selectOperation(this, type);
    }

    withdraw(amount: number) {
        this.state.withdraw(this, amount);
    }

    checkBalance() {
        this.state.checkBalance(this);
    }

    ejectCard() {
        this.state.ejectCard(this);
    }

    printTransactionHistory() {
        const account = this.getCard().account;
        const transactions = account.getTransactions();

        console.log("----- Transaction History -----");

        transactions.forEach(txn => {
            console.log(
                `ID: ${txn.id} | Type: ${txn.type} | Amount: ₹${txn.amount} | Status: ${txn.status} | Time: ${txn.timestamp}`
            );
        });
    }
}

//Concrete States

class IdleState implements ATMState {
    insertCard(atm: ATM, card: Card): void {
        atm.setCard(card);
        console.log("Card inserted");
        atm.setState(new HasCardState());
    }

    enterPin(): void { throw new Error("Insert card first"); }
    selectOperation(): void { throw new Error("Insert card first"); }
    withdraw(): void { throw new Error("Insert card first"); }
    checkBalance(): void { throw new Error("Insert card first"); }
    ejectCard(): void { throw new Error("No card to eject"); }
}

class HasCardState implements ATMState {
    insertCard(): void { throw new Error("Card already inserted"); }

    enterPin(atm: ATM, pin: string): void {
        const card = atm.getCard();
        if (!card.validatePin(pin)) {
            throw new Error("Invalid PIN");
        }
        console.log("PIN validated");
        atm.setState(new SelectOperationState());
    }

    selectOperation(): void { throw new Error("Enter PIN first"); }
    withdraw(): void { throw new Error("Enter PIN first"); }
    checkBalance(): void { throw new Error("Enter PIN first"); }

    ejectCard(atm: ATM): void {
        atm.setCard(null);
        atm.setState(new IdleState());
        console.log("Card ejected");
    }
}

class SelectOperationState implements ATMState {
    insertCard(): void { throw new Error("Operation in progress"); }
    enterPin(): void { throw new Error("Already validated"); }

    selectOperation(atm: ATM, type: TransactionType): void {
        if (type === TransactionType.WITHDRAW) {
            atm.setState(new WithdrawState());
        } else {
            atm.setState(new CheckBalanceState());
        }
    }

    withdraw(): void { throw new Error("Select operation first"); }
    checkBalance(): void { throw new Error("Select operation first"); }

    ejectCard(atm: ATM): void {
        atm.setCard(null);
        atm.setState(new IdleState());
    }
}

class WithdrawState implements ATMState {

    withdraw(atm: ATM, amount: number): void {
        const card = atm.getCard();
        const account = card.account;

        try {
            if (!atm.cashInventory.hasSufficientCash(amount)) {
                throw new Error("ATM insufficient cash");
            }

            account.withdraw(amount);
            atm.dispenser.dispense(amount);

            const txn = new Transaction(
                TransactionType.WITHDRAW,
                amount,
                TransactionStatus.SUCCESS
            );
            account.addTransaction(txn);

            console.log("Withdrawal successful");

        } catch (error) {
            const txn = new Transaction(
                TransactionType.WITHDRAW,
                amount,
                TransactionStatus.FAILED
            );
            account.addTransaction(txn);

            throw error;
        }

        atm.setState(new SelectOperationState());
    }

    insertCard(): void { throw new Error("Invalid state"); }
    enterPin(): void { throw new Error("Invalid state"); }
    selectOperation(): void { throw new Error("Invalid state"); }
    checkBalance(): void { throw new Error("Invalid state"); }

    ejectCard(atm: ATM): void {
        atm.setCard(null);
        atm.setState(new IdleState());
    }
}

class CheckBalanceState implements ATMState {

    checkBalance(atm: ATM): void {
        const account = atm.getCard().account;
        const balance = account.getBalance();

        const txn = new Transaction(
            TransactionType.CHECK_BALANCE,
            0,
            TransactionStatus.SUCCESS
        );
        account.addTransaction(txn);

        console.log("Available Balance:", balance);

        atm.setState(new SelectOperationState());
    }

    insertCard(): void { throw new Error("Invalid state"); }
    enterPin(): void { throw new Error("Invalid state"); }
    selectOperation(): void { throw new Error("Invalid state"); }
    withdraw(): void { throw new Error("Invalid state"); }

    ejectCard(atm: ATM): void {
        atm.setCard(null);
        atm.setState(new IdleState());
    }
}

function main() {
    const account = new Account("ACC123", 10000);
    const card = new Card("CARD123", account, "1234");

    const inventory = new CashInventory();
    inventory.addCash(2000, 10);
    inventory.addCash(500, 20);

    const dispenser = new CashDispenser(inventory);
    const atm = new ATM(inventory, dispenser);

    atm.insertCard(card);
    atm.enterPin("1234");

    atm.selectOperation(TransactionType.WITHDRAW);
    atm.withdraw(3000);

    atm.selectOperation(TransactionType.CHECK_BALANCE);
    atm.checkBalance();

    atm.printTransactionHistory();

    atm.ejectCard();
}

main();