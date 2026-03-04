// ============================================================
// ENUMS
// ============================================================

enum SlotSize {
    SMALL = "SMALL",
    MEDIUM = "MEDIUM",
    LARGE = "LARGE"
}

enum SlotStatus {
    AVAILABLE = "AVAILABLE",
    RESERVED = "RESERVED",
    OCCUPIED = "OCCUPIED",
    EXPIRED = "EXPIRED"
}

enum PackageStatus {
    CREATED = "CREATED",
    ASSIGNED_TO_AGENT = "ASSIGNED_TO_AGENT",
    DELIVERED_TO_LOCKER = "DELIVERED_TO_LOCKER",
    PICKED_UP = "PICKED_UP"
}

enum TokenStatus {
    ACTIVE = "ACTIVE",
    USED = "USED",
    EXPIRED = "EXPIRED"
}

// ============================================================
// SIMPLE ASYNC MUTEX (Machine-Level Lock)
// ============================================================

class Mutex {
    private locked = false;
    private waiting: Array<() => void> = [];

    async acquire(): Promise<void> {
        if (!this.locked) {
            this.locked = true;
            return;
        }

        return new Promise(resolve => {
            this.waiting.push(() => {
                this.locked = true;
                resolve();
            });
        });
    }

    release(): void {
        if (this.waiting.length > 0) {
            const next = this.waiting.shift();
            next && next();
        } else {
            this.locked = false;
        }
    }
}

// ============================================================
// CORE ENTITIES
// ============================================================

class LockerSlot {
    constructor(
        public id: string,
        public size: SlotSize,
        public status: SlotStatus = SlotStatus.AVAILABLE,
        public packageId?: string
    ) {}

    reserve(packageId: string): void {
        if (this.status !== SlotStatus.AVAILABLE)
            throw new Error("Slot not available");

        this.status = SlotStatus.RESERVED;
        this.packageId = packageId;
    }

    occupy(): void {
        this.status = SlotStatus.OCCUPIED;
    }

    release(): void {
        this.status = SlotStatus.AVAILABLE;
        this.packageId = undefined;
    }
}

class AccessToken {
    constructor(
        public code: string,
        public packageId: string,
        public createdAt: Date = new Date(),
        public status: TokenStatus = TokenStatus.ACTIVE
    ) {}

    isExpired(): boolean {
        const expiryMs = 7 * 24 * 60 * 60 * 1000;
        return Date.now() > this.createdAt.getTime() + expiryMs;
    }
}

class Package {
    constructor(
        public id: string,
        public size: SlotSize,
        public zipcode: string,
        public status: PackageStatus = PackageStatus.CREATED,
        public lockerId?: string,
        public slotId?: string,
        public token?: AccessToken
    ) {}
}

class DeliveryAgent {
    constructor(
        public id: string,
        public serviceableZipcodes: string[]
    ) {}

    canDeliver(zip: string): boolean {
        return this.serviceableZipcodes.includes(zip);
    }
}

// ============================================================
// STRATEGIES
// ============================================================

interface SlotAssignmentStrategy {
    assign(slots: LockerSlot[], size: SlotSize): LockerSlot | null;
}

class FirstFitStrategy implements SlotAssignmentStrategy {
    assign(slots: LockerSlot[], size: SlotSize): LockerSlot | null {
        return slots.find(
            s => s.size === size && s.status === SlotStatus.AVAILABLE
        ) || null;
    }
}

interface AgentAssignmentStrategy {
    assign(agents: DeliveryAgent[], zipcode: string): DeliveryAgent;
}

class RandomAgentStrategy implements AgentAssignmentStrategy {
    assign(agents: DeliveryAgent[], zipcode: string): DeliveryAgent {
        const eligible = agents.filter(a => a.canDeliver(zipcode));
        if (!eligible.length)
            throw new Error("No agent available");

        return eligible[Math.floor(Math.random() * eligible.length)];
    }
}

interface SlotOpenStrategy {
    validate(token: string, locker: LockerSystem): boolean;
}

class OTPStrategy implements SlotOpenStrategy {
    validate(token: string, locker: LockerSystem): boolean {
        return locker.validateToken(token);
    }
}

class BarcodeStrategy implements SlotOpenStrategy {
    validate(token: string, locker: LockerSystem): boolean {
        return locker.validateToken(token);
    }
}

// ============================================================
// MANAGERS
// ============================================================

class PackageManager {
    private static instance: PackageManager;
    private packages = new Map<string, Package>();
    private tokens = new Map<string, AccessToken>();

    private constructor() {}

    static getInstance(): PackageManager {
        if (!this.instance)
            this.instance = new PackageManager();
        return this.instance;
    }

    addPackage(pkg: Package) {
        this.packages.set(pkg.id, pkg);
    }

    getPackage(id: string): Package {
        const pkg = this.packages.get(id);
        if (!pkg) throw new Error("Package not found");
        return pkg;
    }

    generateToken(pkg: Package): AccessToken {
        const token = new AccessToken(
            "T-" + Math.random().toString(36).substring(2),
            pkg.id
        );
        this.tokens.set(token.code, token);
        pkg.token = token;
        return token;
    }

    validateAndUseToken(code: string): Package {
        const token = this.tokens.get(code);
        if (!token) throw new Error("Invalid token");

        if (token.status === TokenStatus.USED)
            throw new Error("Token already used");

        if (token.isExpired()) {
            token.status = TokenStatus.EXPIRED;
            throw new Error("Token expired");
        }

        token.status = TokenStatus.USED;
        return this.getPackage(token.packageId);
    }
}

class AgentManager {
    private agents: DeliveryAgent[] = [];

    addAgent(agent: DeliveryAgent) {
        this.agents.push(agent);
    }

    assignAgent(zipcode: string, strategy: AgentAssignmentStrategy) {
        return strategy.assign(this.agents, zipcode);
    }
}

// ============================================================
// LOCKER SYSTEM (Machine-Level Lock Enforced)
// ============================================================

class LockerSystem {

    private mutex = new Mutex();
    private openStrategy!: SlotOpenStrategy;

    constructor(
        public slots: LockerSlot[],
        private slotStrategy: SlotAssignmentStrategy
    ) {}

    setOpenStrategy(strategy: SlotOpenStrategy) {
        this.openStrategy = strategy;
    }

    async deposit(pkg: Package) {

        await this.mutex.acquire();  // 🔒 machine lock

        try {
            const slot = this.slotStrategy.assign(
                this.slots,
                pkg.size
            );

            if (!slot)
                throw new Error("No slot available");

            slot.reserve(pkg.id);
            slot.occupy();

            pkg.slotId = slot.id;
            pkg.status = PackageStatus.DELIVERED_TO_LOCKER;

            const token =
                PackageManager.getInstance().generateToken(pkg);

            console.log(
                `Notification sent: OTP ${token.code}`
            );

        } finally {
            this.mutex.release(); // 🔓 release machine
        }
    }

    async open(tokenCode: string) {

        await this.mutex.acquire();  // 🔒 only one operation

        try {
            if (!this.openStrategy)
                throw new Error("Open strategy not set");

            if (!this.openStrategy.validate(tokenCode, this))
                throw new Error("Invalid credential");

            const pkg =
                PackageManager.getInstance()
                    .validateAndUseToken(tokenCode);

            const slot = this.slots.find(
                s => s.id === pkg.slotId
            );

            if (!slot)
                throw new Error("Slot not found");

            slot.release();
            pkg.status = PackageStatus.PICKED_UP;

            console.log("Package picked up successfully");

        } finally {
            this.mutex.release();
        }
    }

    validateToken(code: string): boolean {
        try {
            PackageManager.getInstance()
                .validateAndUseToken(code);
            return true;
        } catch {
            return false;
        }
    }
}

class Locker {
    constructor(
        public id: string,
        public zipcode: string,
        public lockerSystem: LockerSystem
    ) {}
}

class LockerManager {
    private static instance: LockerManager;
    private lockers: Locker[] = [];

    private constructor() {}

    static getInstance(): LockerManager {
        if (!this.instance)
            this.instance = new LockerManager();
        return this.instance;
    }

    addLocker(locker: Locker) {
        this.lockers.push(locker);
    }

    findByZipcode(zip: string): Locker[] {
        return this.lockers.filter(l => l.zipcode === zip);
    }
}

// ============================================================
// MAIN FLOW
// ============================================================

async function main() {

    const locker = new Locker(
        "L1",
        "560001",
        new LockerSystem(
            [
                new LockerSlot("S1", SlotSize.SMALL),
                new LockerSlot("S2", SlotSize.MEDIUM)
            ],
            new FirstFitStrategy()
        )
    );

    LockerManager.getInstance().addLocker(locker);

    const agentManager = new AgentManager();
    agentManager.addAgent(
        new DeliveryAgent("A1", ["560001"])
    );

    const pkg = new Package("PKG1", SlotSize.SMALL, "560001");

    PackageManager.getInstance().addPackage(pkg);

    const agent = agentManager.assignAgent(
        "560001",
        new RandomAgentStrategy()
    );

    console.log("Agent assigned:", agent.id);

    locker.lockerSystem.setOpenStrategy(new BarcodeStrategy());
    await locker.lockerSystem.deposit(pkg);

    const token = pkg.token!.code;

    locker.lockerSystem.setOpenStrategy(new OTPStrategy());
    await locker.lockerSystem.open(token);
}

main();