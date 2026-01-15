enum SplitType {
    EQUAL,
    EXACT,
    PERCENTAGE
}

class Split {
    userId: string;
    amount: number;

    constructor(userId: string, amount: number) {
        this.userId = userId;
        this.amount = amount;
    }
}

//Observer Pattern-Notification Interface
interface Observer {
    update(message: string): void;
}

//Strategy Pattern-Split Strategies
interface SplitStrategy {
    calculateSplit(totalAmount: number, userIds: string[], values?: number[]): Split[];
}

class EqualSplit implements SplitStrategy {
    calculateSplit(totalAmount: number, userIds: string[]): Split[] {
        const splits: Split[] = [];
        const splitAmount = totalAmount / userIds.length;

        userIds.forEach(userId => {
            splits.push(new Split(userId, splitAmount));
        });

        return splits;
    }
}

class ExactSplit implements SplitStrategy {
    calculateSplit(
        totalAmount: number,
        userIds: string[],
        values: number[] = []
    ): Split[] {
        const splits: Split[] = [];

        // validations can be added here

        for (let i = 0; i < userIds.length; i++) {
            splits.push(new Split(userIds[i], values[i]));
        }

        return splits;
    }
}

class PercentageSplit implements SplitStrategy {
    calculateSplit(
        totalAmount: number,
        userIds: string[],
        values: number[] = []
    ): Split[] {
        const splits: Split[] = [];

        // validations can be added here

        for (let i = 0; i < userIds.length; i++) {
            const amount = (totalAmount * values[i]) / 100;
            splits.push(new Split(userIds[i], amount));
        }

        return splits;
    }
}

//Factory for Split Strategies
class SplitFactory {
    static getSplitStrategy(type: SplitType): SplitStrategy {
        switch (type) {
            case SplitType.EQUAL:
                return new EqualSplit();
            case SplitType.EXACT:
                return new ExactSplit();
            case SplitType.PERCENTAGE:
                return new PercentageSplit();
            default:
                return new EqualSplit();
        }
    }
}

//User Class -> Concrete Observer
class User implements Observer {
    static nextUserId = 1;
    userId: string;
    name: string;
    email: string;
    balances: Map<string, number>; // userId -> amount (positive = they owe you, negative = you owe them)

    constructor(name: string, email: string) {
        this.userId = `U${User.nextUserId++}`;
        this.name = name;
        this.email = email;
        this.balances = new Map<string, number>();
    }

    update(message: string): void {
        console.log(`Notification for ${this.name}: ${message}`);
    }

    updateBalance(otherUserId: string, amount: number): void {
        const currentBalance = this.balances.get(otherUserId) || 0;
        this.balances.set(otherUserId, currentBalance + amount);

        if (this.balances.get(otherUserId) === 0) {
            this.balances.delete(otherUserId); // Clean up zero balances
        }
    }

    getTotalOwed(): number {
        let total = 0;
        this.balances.forEach(amount => {
            if (amount < 0) {
                total += -amount;
            }
        });
        return total;
    }

    getTotalOwing(): number {
        let total = 0;
        this.balances.forEach(amount => {
            if (amount > 0) {
                total += amount;
            }
        });
        return total;
    }
}

//Expense Model Class
class Expense {
    private static nextExpenseId = 0;

    public expenseId: string;
    public description: string;
    public totalAmount: number;
    public paidByUserId: string;
    public splits: Split[];
    public groupId: string;

    constructor(
        desc: string,
        amount: number,
        paidBy: string,
        splits: Split[],
        group: string = ""
    ) {
        Expense.nextExpenseId++;

        this.expenseId = `expense${Expense.nextExpenseId}`;
        this.description = desc;
        this.totalAmount = amount;
        this.paidByUserId = paidBy;
        this.splits = splits;
        this.groupId = group;
    }
}

class DebtSimplifier {
    static simplifyDebts(
        groupBalances: Map<string, Map<string, number>>
    ): Map<string, Map<string, number>> {

        // Calculate net amount for each person
        const netAmounts: Map<string, number> = new Map();

        // Initialize all users with 0
        for (const [userId] of groupBalances) {
            netAmounts.set(userId, 0);
        }

        // Calculate net amounts
        // If groupBalances[A][B] = 200, B owes A 200
        for (const [creditorId, balances] of groupBalances) {
            for (const [debtorId, amount] of balances) {

                // Only process positive amounts to avoid double counting
                if (amount > 0) {
                    netAmounts.set(
                        creditorId,
                        (netAmounts.get(creditorId) || 0) + amount
                    );
                    netAmounts.set(
                        debtorId,
                        (netAmounts.get(debtorId) || 0) - amount
                    );
                }
            }
        }

        // Divide users into creditors and debtors
        const creditors: Array<[string, number]> = [];
        const debtors: Array<[string, number]> = [];

        for (const [userId, amount] of netAmounts) {
            if (amount > 0.01) {
                creditors.push([userId, amount]);
            } else if (amount < -0.01) {
                debtors.push([userId, -amount]); // store positive
            }
        }

        // Sort largest first
        creditors.sort((a, b) => b[1] - a[1]);
        debtors.sort((a, b) => b[1] - a[1]);

        // Create new simplified balance map
        const simplifiedBalances: Map<string, Map<string, number>> = new Map();

        // Initialize empty maps for all users
        for (const [userId] of groupBalances) {
            simplifiedBalances.set(userId, new Map());
        }

        // Greedy settlement
        let i = 0;
        let j = 0;

        while (i < creditors.length && j < debtors.length) {
            const [creditorId, creditorAmount] = creditors[i];
            const [debtorId, debtorAmount] = debtors[j];

            const settleAmount = Math.min(creditorAmount, debtorAmount);

            // debtor owes creditor
            simplifiedBalances.get(creditorId)!.set(debtorId, settleAmount);
            simplifiedBalances.get(debtorId)!.set(creditorId, -settleAmount);

            // Update remaining
            creditors[i][1] -= settleAmount;
            debtors[j][1] -= settleAmount;

            if (creditors[i][1] < 0.01) i++;
            if (debtors[j][1] < 0.01) j++;
        }

        return simplifiedBalances;
    }
}

class Group {
    private static nextGroupId = 0;

    public groupId: string;
    public name: string;
    public members: User[] = [];
    public groupExpenses: Map<string, Expense> = new Map(); // Group's own expense book
    public groupBalances: Map<string, Map<string, number>> = new Map(); // userId -> (userId -> amount)

    constructor(name: string) {
        Group.nextGroupId++;
        this.groupId = `group${Group.nextGroupId}`;
        this.name = name;
    }

    // ---------------- Private Helpers ----------------

    private getUserByUserId(userId: string): User | null {
        for (const member of this.members) {
            if (member.userId === userId) return member;
        }
        return null;
    }

    // ---------------- Member Management ----------------

    addMember(user: User): void {
        this.members.push(user);
        this.groupBalances.set(user.userId, new Map());
        console.log(`${user.name} added to group ${this.name}`);
    }

    removeMember(userId: string): boolean {
        if (!this.canUserLeaveGroup(userId)) {
            console.log("User not allowed to leave group without clearing expenses");
            return false;
        }

        this.members = this.members.filter(u => u.userId !== userId);
        this.groupBalances.delete(userId);

        for (const [, balanceMap] of this.groupBalances) {
            balanceMap.delete(userId);
        }

        return true;
    }

    isMember(userId: string): boolean {
        return this.groupBalances.has(userId);
    }

    // ---------------- Observer ----------------

    notifyMembers(message: string): void {
        for (const member of this.members) {
            member.update(message);
        }
    }

    // ---------------- Balances ----------------

    updateGroupBalance(fromUserId: string, toUserId: string, amount: number): void {
        const fromMap = this.groupBalances.get(fromUserId)!;
        const toMap = this.groupBalances.get(toUserId)!;

        fromMap.set(toUserId, (fromMap.get(toUserId) || 0) + amount);
        toMap.set(fromUserId, (toMap.get(fromUserId) || 0) - amount);

        if (Math.abs(fromMap.get(toUserId)!) < 0.01) fromMap.delete(toUserId);
        if (Math.abs(toMap.get(fromUserId)!) < 0.01) toMap.delete(fromUserId);
    }

    canUserLeaveGroup(userId: string): boolean {
        if (!this.isMember(userId)) {
            throw new Error("user is not a part of this group");
        }

        const balanceSheet = this.groupBalances.get(userId)!;
        for (const [, amount] of balanceSheet) {
            if (Math.abs(amount) > 0.01) return false;
        }
        return true;
    }

    getUserGroupBalances(userId: string): Map<string, number> {
        if (!this.isMember(userId)) {
            throw new Error("user is not a part of this group");
        }
        return this.groupBalances.get(userId)!;
    }

    // ---------------- Expenses ----------------

    addExpense(
        description: string,
        amount: number,
        paidByUserId: string,
        involvedUsers: string[],
        splitType: SplitType,
        splitValues: number[] = []
    ): boolean {
        if (!this.isMember(paidByUserId)) {
            throw new Error("user is not a part of this group");
        }

        for (const userId of involvedUsers) {
            if (!this.isMember(userId)) {
                throw new Error("involvedUsers are not a part of this group");
            }
        }

        const splits = SplitFactory
            .getSplitStrategy(splitType)
            .calculateSplit(amount, involvedUsers, splitValues);

        const expense = new Expense(description, amount, paidByUserId, splits, this.groupId);
        this.groupExpenses.set(expense.expenseId, expense);

        for (const split of splits) {
            if (split.userId !== paidByUserId) {
                this.updateGroupBalance(paidByUserId, split.userId, split.amount);
            }
        }

        const paidByName = this.getUserByUserId(paidByUserId)?.name || paidByUserId;
        this.notifyMembers(`New expense added: ${description} (Rs ${amount})`);

        console.log(`Expense added to ${this.name}: ${description} paid by ${paidByName}`);
        return true;
    }

    // ---------------- Settlement ----------------

    settlePayment(fromUserId: string, toUserId: string, amount: number): boolean {
        if (!this.isMember(fromUserId) || !this.isMember(toUserId)) {
            console.log("user is not a part of this group");
            return false;
        }

        this.updateGroupBalance(fromUserId, toUserId, amount);

        const fromName = this.getUserByUserId(fromUserId)?.name || fromUserId;
        const toName = this.getUserByUserId(toUserId)?.name || toUserId;

        this.notifyMembers(`Settlement: ${fromName} paid ${toName} Rs ${amount}`);

        console.log(`Settlement in ${this.name}: ${fromName} paid ${toName} Rs ${amount}`);
        return true;
    }

    // ---------------- Display ----------------

    showGroupBalances(): void {
        console.log(`\n=== Group Balances for ${this.name} ===`);

        for (const [memberId, balances] of this.groupBalances) {
            const memberName = this.getUserByUserId(memberId)?.name || memberId;
            console.log(`${memberName}'s balances:`);

            if (balances.size === 0) {
                console.log("  No outstanding balances");
            } else {
                for (const [otherId, amount] of balances) {
                    const otherName = this.getUserByUserId(otherId)?.name || otherId;
                    if (amount > 0) {
                        console.log(`  ${otherName} owes: Rs ${amount.toFixed(2)}`);
                    } else {
                        console.log(`  Owes ${otherName}: Rs ${Math.abs(amount).toFixed(2)}`);
                    }
                }
            }
        }
    }

    // ---------------- Simplification ----------------

    simplifyGroupDebts(): void {
        this.groupBalances = DebtSimplifier.simplifyDebts(this.groupBalances);
        console.log(`Debts have been simplified for group: ${this.name}`);
    }
}

// Splitwise Main Class - Singleton and Facade
class Splitwise {
    private static instance: Splitwise | null = null;

    private users: Map<string, User> = new Map();
    private groups: Map<string, Group> = new Map();
    private expenses: Map<string, Expense> = new Map();

    private constructor() { }

    public static getInstance(): Splitwise {
        if (!Splitwise.instance) {
            Splitwise.instance = new Splitwise();
        }
        return Splitwise.instance;
    }

    // ---------------- User Management ----------------

    createUser(name: string, email: string): User {
        const user = new User(name, email);
        this.users.set(user.userId, user);
        console.log(`User created: ${name} (ID: ${user.userId})`);
        return user;
    }

    getUser(userId: string): User | null {
        return this.users.get(userId) || null;
    }

    // ---------------- Group Management ----------------

    createGroup(name: string): Group {
        const group = new Group(name);
        this.groups.set(group.groupId, group);
        console.log(`Group created: ${name} (ID: ${group.groupId})`);
        return group;
    }

    getGroup(groupId: string): Group | null {
        return this.groups.get(groupId) || null;
    }

    addUserToGroup(userId: string, groupId: string): void {
        const user = this.getUser(userId);
        const group = this.getGroup(groupId);

        if (user && group) {
            group.addMember(user);
        }
    }

    removeUserFromGroup(userId: string, groupId: string): boolean {
        const group = this.getGroup(groupId);
        if (!group) {
            console.log("Group not found!");
            return false;
        }

        const user = this.getUser(userId);
        if (!user) {
            console.log("User not found!");
            return false;
        }

        const userRemoved = group.removeMember(userId);
        if (userRemoved) {
            console.log(`${user.name} successfully left ${group.name}`);
        }

        return userRemoved;
    }

    // ---------------- Group Expenses ----------------

    addExpenseToGroup(
        groupId: string,
        description: string,
        amount: number,
        paidByUserId: string,
        involvedUsers: string[],
        splitType: SplitType,
        splitValues: number[] = []
    ): void {
        const group = this.getGroup(groupId);
        if (!group) {
            console.log("Group not found!");
            return;
        }

        group.addExpense(
            description,
            amount,
            paidByUserId,
            involvedUsers,
            splitType,
            splitValues
        );
    }

    settlePaymentInGroup(
        groupId: string,
        fromUserId: string,
        toUserId: string,
        amount: number
    ): void {
        const group = this.getGroup(groupId);
        if (!group) {
            console.log("Group not found!");
            return;
        }

        group.settlePayment(fromUserId, toUserId, amount);
    }

    // ---------------- Individual Settlement ----------------

    settleIndividualPayment(
        fromUserId: string,
        toUserId: string,
        amount: number
    ): void {
        const fromUser = this.getUser(fromUserId);
        const toUser = this.getUser(toUserId);

        if (fromUser && toUser) {
            fromUser.updateBalance(toUserId, amount);
            toUser.updateBalance(fromUserId, -amount);

            console.log(`${fromUser.name} settled Rs${amount} with ${toUser.name}`);
        }
    }

    addIndividualExpense(
        description: string,
        amount: number,
        paidByUserId: string,
        toUserId: string,
        splitType: SplitType,
        splitValues: number[] = []
    ): void {
        const strategy = SplitFactory.getSplitStrategy(splitType);
        const splits = strategy.calculateSplit(
            amount,
            [paidByUserId, toUserId],
            splitValues
        );

        const expense = new Expense(description, amount, paidByUserId, splits);
        this.expenses.set(expense.expenseId, expense);

        const paidByUser = this.getUser(paidByUserId)!;
        const toUser = this.getUser(toUserId)!;

        paidByUser.updateBalance(toUserId, amount);
        toUser.updateBalance(paidByUserId, -amount);

        console.log(
            `Individual expense added: ${description} (Rs ${amount}) paid by ${paidByUser.name} for ${toUser.name}`
        );
    }

    // ---------------- Display ----------------

    showUserBalance(userId: string): void {
        const user = this.getUser(userId);
        if (!user) return;

        console.log(`\n=========== Balance for ${user.name} ====================`);
        console.log(`Total you owe: Rs ${user.getTotalOwed().toFixed(2)}`);
        console.log(`Total others owe you: Rs ${user.getTotalOwing().toFixed(2)}`);

        console.log("Detailed balances:");
        for (const [otherUserId, balance] of user.balances) {
            const otherUser = this.getUser(otherUserId);
            if (!otherUser) continue;

            if (balance > 0) {
                console.log(`  ${otherUser.name} owes you: Rs${balance}`);
            } else {
                console.log(`  You owe ${otherUser.name}: Rs${Math.abs(balance)}`);
            }
        }
    }

    showGroupBalances(groupId: string): void {
        const group = this.getGroup(groupId);
        if (!group) return;

        group.showGroupBalances();
    }

    simplifyGroupDebts(groupId: string): void {
        const group = this.getGroup(groupId);
        if (!group) return;

        group.simplifyGroupDebts();
    }
}

//client
function main(): void {
    const manager = Splitwise.getInstance();

    console.log("\n=========== Creating Users ====================");
    const user1 = manager.createUser("Aditya", "aditya@gmail.com");
    const user2 = manager.createUser("Rohit", "rohit@gmail.com");
    const user3 = manager.createUser("Manish", "manish@gmail.com");
    const user4 = manager.createUser("Saurav", "saurav@gmail.com");

    console.log("\n=========== Creating Group and Adding Members ====================");
    const hostelGroup = manager.createGroup("Hostel Expenses");

    manager.addUserToGroup(user1.userId, hostelGroup.groupId);
    manager.addUserToGroup(user2.userId, hostelGroup.groupId);
    manager.addUserToGroup(user3.userId, hostelGroup.groupId);
    manager.addUserToGroup(user4.userId, hostelGroup.groupId);

    console.log("\n=========== Adding Expenses in group ====================");
    const groupMembers = [user1.userId, user2.userId, user3.userId, user4.userId];

    manager.addExpenseToGroup(
        hostelGroup.groupId,
        "Lunch",
        800.0,
        user1.userId,
        groupMembers,
        SplitType.EQUAL
    );

    const dinnerMembers = [user1.userId, user3.userId, user4.userId];
    const dinnerAmounts = [200.0, 300.0, 200.0];

    manager.addExpenseToGroup(
        hostelGroup.groupId,
        "Dinner",
        700.0,
        user3.userId,
        dinnerMembers,
        SplitType.EXACT,
        dinnerAmounts
    );

    console.log("\n=========== printing Group-Specific Balances ====================");
    manager.showGroupBalances(hostelGroup.groupId);

    console.log("\n=========== Debt Simplification ====================");
    manager.simplifyGroupDebts(hostelGroup.groupId);

    console.log("\n=========== printing Group-Specific Balances ====================");
    manager.showGroupBalances(hostelGroup.groupId);

    console.log("\n=========== Adding Individual Expense ====================");
    manager.addIndividualExpense(
        "Coffee",
        40.0,
        user2.userId,
        user4.userId,
        SplitType.EQUAL
    );

    console.log("\n=========== printing User Balances ====================");
    manager.showUserBalance(user1.userId);
    manager.showUserBalance(user2.userId);
    manager.showUserBalance(user3.userId);
    manager.showUserBalance(user4.userId);

    console.log("\n==========Attempting to remove Rohit from group==========");
    manager.removeUserFromGroup(user2.userId, hostelGroup.groupId);

    console.log("\n======== Making Settlement to Clear Rohit's Debt ==========");
    manager.settlePaymentInGroup(
        hostelGroup.groupId,
        user2.userId,
        user3.userId,
        200.0
    );

    console.log("\n======== Attempting to Remove Rohit Again ==========");
    manager.removeUserFromGroup(user2.userId, hostelGroup.groupId);

    console.log("\n=========== Updated Group Balances ====================");
    manager.showGroupBalances(hostelGroup.groupId);
}

// Run program
main();
