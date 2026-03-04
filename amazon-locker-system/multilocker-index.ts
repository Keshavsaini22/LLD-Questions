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
    EXPIRED = "EXPIRED",
    OUT_OF_SERVICE = "OUT_OF_SERVICE"
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
// MUTEX (Machine-Level)
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
// SLOT
// ============================================================

class LockerSlot {

    constructor(
        public id: string,
        public size: SlotSize,
        public status: SlotStatus = SlotStatus.AVAILABLE,
        public packageId?: string
    ) {}

    reserve(packageId: string) {
        if (this.status !== SlotStatus.AVAILABLE)
            throw new Error("Slot not available");

        this.status = SlotStatus.RESERVED;
        this.packageId = packageId;
    }

    occupy() {
        if (this.status !== SlotStatus.RESERVED)
            throw new Error("Invalid transition");

        this.status = SlotStatus.OCCUPIED;
    }

    release() {
        this.status = SlotStatus.AVAILABLE;
        this.packageId = undefined;
    }
}

// ============================================================
// PACKAGE + TOKEN
// ============================================================

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

// ============================================================
// STRATEGIES
// ============================================================

interface SlotAssignmentStrategy {
    assign(slots: LockerSlot[], size: SlotSize): LockerSlot | null;
}

class FirstFitStrategy implements SlotAssignmentStrategy {
    assign(slots: LockerSlot[], size: SlotSize): LockerSlot | null {
        return slots.find(
            s => s.size === size &&
                 s.status === SlotStatus.AVAILABLE
        ) || null;
    }
}

interface SlotOpenStrategy {
    validate(token: string): Package;
}

class OTPStrategy implements SlotOpenStrategy {
    validate(token: string): Package {
        return PackageManager.getInstance()
            .validateToken(token);
    }
}

class BarcodeStrategy implements SlotOpenStrategy {
    validate(token: string): Package {
        return PackageManager.getInstance()
            .validateToken(token);
    }
}

// ============================================================
// MANAGERS (Singleton)
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

    generateToken(pkg: Package): AccessToken {
        const token = new AccessToken(
            "T-" + Math.random().toString(36).substring(2),
            pkg.id
        );
        this.tokens.set(token.code, token);
        pkg.token = token;
        return token;
    }

    validateToken(code: string): Package {
        const token = this.tokens.get(code);
        if (!token) throw new Error("Invalid token");

        if (token.status === TokenStatus.USED)
            throw new Error("Token already used");

        if (token.isExpired()) {
            token.status = TokenStatus.EXPIRED;
            throw new Error("Token expired");
        }

        return this.packages.get(token.packageId)!;
    }

    consumeToken(code: string) {
        const token = this.tokens.get(code);
        if (token) token.status = TokenStatus.USED;
    }
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
// STATE PATTERN (Machine-Level)
// ============================================================

interface LockerState {
    deposit(locker: LockerSystem, pkg: Package): Promise<void>;
    open(locker: LockerSystem, token: string): Promise<void>;
}

class IdleState implements LockerState {

    async deposit(locker: LockerSystem, pkg: Package) {
        locker.setState(new ServingAgentState());
        await locker.depositInternal(pkg);
        locker.setState(new IdleState());
    }

    async open(locker: LockerSystem, token: string) {
        locker.setState(new ServingCustomerState());
        await locker.openInternal(token);
        locker.setState(new IdleState());
    }
}

class ServingAgentState implements LockerState {
    async deposit() { throw new Error("Machine busy"); }
    async open() { throw new Error("Agent operation active"); }
}

class ServingCustomerState implements LockerState {
    async deposit() { throw new Error("Customer operation active"); }
    async open() { throw new Error("Machine busy"); }
}

// ============================================================
// LOCKER SYSTEM
// ============================================================

class LockerSystem {

    private mutex = new Mutex();
    private state: LockerState = new IdleState();
    private openStrategy!: SlotOpenStrategy;

    constructor(
        public id: string,
        public slots: LockerSlot[],
        private slotStrategy: SlotAssignmentStrategy
    ) {}

    setState(state: LockerState) {
        this.state = state;
    }

    setOpenStrategy(strategy: SlotOpenStrategy) {
        this.openStrategy = strategy;
    }

    async deposit(pkg: Package) {

        await this.mutex.acquire();
        try {
            await this.state.deposit(this, pkg);
        } finally {
            this.mutex.release();
        }
    }

    async open(token: string) {

        await this.mutex.acquire();
        try {
            await this.state.open(this, token);
        } finally {
            this.mutex.release();
        }
    }

    async depositInternal(pkg: Package) {

        const slot =
            this.slotStrategy.assign(this.slots, pkg.size);

        if (!slot)
            throw new Error("No slot available");

        slot.reserve(pkg.id);
        slot.occupy();

        pkg.slotId = slot.id;
        pkg.lockerId = this.id;
        pkg.status = PackageStatus.DELIVERED_TO_LOCKER;

        const token =
            PackageManager.getInstance()
                .generateToken(pkg);

        console.log(
            `Locker ${this.id} OTP: ${token.code}`
        );
    }

    async openInternal(tokenCode: string) {

        if (!this.openStrategy)
            throw new Error("Open strategy not set");

        const pkg =
            this.openStrategy.validate(tokenCode);

        if (pkg.lockerId !== this.id)
            throw new Error("Wrong locker");

        const slot =
            this.slots.find(s => s.id === pkg.slotId);

        if (!slot)
            throw new Error("Slot not found");

        slot.release();
        pkg.status = PackageStatus.PICKED_UP;

        PackageManager
            .getInstance()
            .consumeToken(tokenCode);

        console.log("Package picked up");
    }
}

class Locker {
    constructor(
        public id: string,
        public zipcode: string,
        public system: LockerSystem
    ) {}
}

// ============================================================
// MAIN
// ============================================================

async function main() {

    const locker1 = new Locker(
        "L1",
        "560001",
        new LockerSystem(
            "L1",
            [
                new LockerSlot("S1", SlotSize.SMALL),
                new LockerSlot("S2", SlotSize.MEDIUM)
            ],
            new FirstFitStrategy()
        )
    );

    LockerManager.getInstance().addLocker(locker1);

    const pkg = new Package(
        "PKG1",
        SlotSize.SMALL,
        "560001"
    );

    PackageManager.getInstance().addPackage(pkg);

    const lockers =
        LockerManager.getInstance()
            .findByZipcode("560001");

    const selectedLocker = lockers[0];

    await selectedLocker.system.deposit(pkg);

    const token = pkg.token!.code;

    selectedLocker.system
        .setOpenStrategy(new OTPStrategy());

    await selectedLocker.system.open(token);
}

main();