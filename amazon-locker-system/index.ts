// =====================================================
// ENUMS
// =====================================================

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
    DELIVERED_TO_LOCKER = "DELIVERED_TO_LOCKER",
    PICKED_UP = "PICKED_UP",
    RETURNED = "RETURNED"
}

enum TokenStatus {
    ACTIVE = "ACTIVE",
    EXPIRED = "EXPIRED",
    USED = "USED"
}

// =====================================================
// MACHINE LEVEL MUTEX
// =====================================================

class MachineLock {
    private locked = false;

    acquire() {
        if (this.locked)
            throw new Error("Locker busy. Try later.");
        this.locked = true;
    }

    release() {
        this.locked = false;
    }
}

// =====================================================
// SLOT
// =====================================================

class LockerSlot {

    private slotLock = false;

    constructor(
        public id: string,
        public size: SlotSize,
        public status: SlotStatus = SlotStatus.AVAILABLE,
        public packageId?: string
    ) { }

    reserve(packageId: string): void {
        if (this.slotLock)
            throw new Error("Slot currently locked");

        this.slotLock = true;

        if (this.status !== SlotStatus.AVAILABLE) {
            this.slotLock = false;
            throw new Error("Slot not available");
        }

        this.status = SlotStatus.RESERVED;
        this.packageId = packageId;
        this.slotLock = false;
    }

    occupy(): void {
        if (this.status !== SlotStatus.RESERVED)
            throw new Error("Invalid transition");

        this.status = SlotStatus.OCCUPIED;
    }

    release(): void {
        this.status = SlotStatus.AVAILABLE;
        this.packageId = undefined;
    }

    markExpired(): void {
        this.status = SlotStatus.EXPIRED;
    }

    markOutOfService(): void {
        this.status = SlotStatus.OUT_OF_SERVICE;
    }
}

// =====================================================
// PACKAGE + TOKEN
// =====================================================

class AccessToken {

    constructor(
        public code: string,
        public packageId: string,
        public createdAt: Date = new Date(),
        public status: TokenStatus = TokenStatus.ACTIVE
    ) { }

    isExpired(): boolean {
        const expiry =
            this.createdAt.getTime() +
            7 * 24 * 60 * 60 * 1000; // 7 days
        return Date.now() > expiry;
    }
}

class Package {

    constructor(
        public id: string,
        public size: SlotSize,
        public status: PackageStatus = PackageStatus.CREATED,
        public slotId?: string,
        public token?: AccessToken
    ) { }
}

// =====================================================
// STRATEGIES
// =====================================================

interface SlotAssignmentStrategy {
    assign(slots: LockerSlot[], size: SlotSize): LockerSlot | null;
}

class FirstFitStrategy implements SlotAssignmentStrategy {
    assign(slots: LockerSlot[], size: SlotSize): LockerSlot | null {
        return (
            slots.find(s =>
                s.size === size &&
                s.status === SlotStatus.AVAILABLE
            ) || null
        );
    }
}

interface SlotOpenStrategy {
    validate(token: string): Package;
}

class OTPStrategy implements SlotOpenStrategy {
    validate(token: string): Package {
        return PackageManager
            .getInstance()
            .validateToken(token);
    }
}

class BarcodeStrategy implements SlotOpenStrategy {
    validate(token: string): Package {
        return PackageManager
            .getInstance()
            .validateToken(token);
    }
}

// =====================================================
// SINGLETON MANAGERS
// =====================================================

class PackageManager {

    private static instance: PackageManager;
    private packages = new Map<string, Package>();
    private tokens = new Map<string, AccessToken>();

    private constructor() { }

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
        if (!token)
            throw new Error("Invalid token");

        if (token.status === TokenStatus.USED)
            throw new Error("Token already used");

        if (token.isExpired()) {
            token.status = TokenStatus.EXPIRED;
            throw new Error("Token expired");
        }

        const pkg = this.packages.get(token.packageId);
        if (!pkg)
            throw new Error("Package not found");

        return pkg;
    }
}

class NotificationService {

    private static instance: NotificationService;

    private constructor() { }

    static getInstance(): NotificationService {
        if (!this.instance)
            this.instance = new NotificationService();
        return this.instance;
    }

    notifyUser(pkgId: string, token: string) {
        console.log(
            `Notification -> Package ${pkgId}, Access Code: ${token}`
        );
    }
}

// =====================================================
// STATE PATTERN
// =====================================================

interface LockerState {
    deposit(locker: LockerSystem, pkg: Package): void;
    open(locker: LockerSystem, token: string): void;
    name(): string;
}

class IdleState implements LockerState {

    deposit(locker: LockerSystem, pkg: Package): void {
        locker.setState(new ServingAgentState());
        locker.depositInternal(pkg);
        locker.setState(new IdleState());
    }

    open(locker: LockerSystem, token: string): void {
        locker.setState(new ServingCustomerState());
        locker.openInternal(token);
        locker.setState(new IdleState());
    }

    name() { return "IDLE"; }
}

class ServingAgentState implements LockerState {
    deposit() { throw new Error("Locker busy"); }
    open() { throw new Error("Agent operation active"); }
    name() { return "SERVING_AGENT"; }
}

class ServingCustomerState implements LockerState {
    deposit() { throw new Error("Customer operation active"); }
    open() { throw new Error("Locker busy"); }
    name() { return "SERVING_CUSTOMER"; }
}

class OutOfServiceState implements LockerState {
    deposit() { throw new Error("Locker out of service"); }
    open() { throw new Error("Locker out of service"); }
    name() { return "OUT_OF_SERVICE"; }
}

// =====================================================
// LOCKER SYSTEM (Facade)
// =====================================================

class LockerSystem {

    private state: LockerState = new IdleState();
    private machineLock = new MachineLock();
    private openStrategy!: SlotOpenStrategy;

    constructor(
        private slots: LockerSlot[],
        private strategy: SlotAssignmentStrategy
    ) { }

    setState(state: LockerState) {
        this.state = state;
    }

    setOpenStrategy(strategy: SlotOpenStrategy) {
        this.openStrategy = strategy;
    }

    deposit(pkg: Package) {
        this.machineLock.acquire();
        try {
            this.state.deposit(this, pkg);
        } finally {
            this.machineLock.release();
        }
    }

    open(token: string) {
        this.machineLock.acquire();
        try {
            this.state.open(this, token);
        } finally {
            this.machineLock.release();
        }
    }

    depositInternal(pkg: Package) {

        const slot =
            this.strategy.assign(this.slots, pkg.size);

        if (!slot)
            throw new Error("No slot available");

        slot.reserve(pkg.id);
        slot.occupy();

        pkg.slotId = slot.id;
        pkg.status = PackageStatus.DELIVERED_TO_LOCKER;

        const token =
            PackageManager.getInstance().generateToken(pkg);

        NotificationService
            .getInstance()
            .notifyUser(pkg.id, token.code);
    }

    openInternal(tokenCode: string) {

        if (!this.openStrategy)
            throw new Error("Open strategy not set");

        const pkg =
            this.openStrategy.validate(tokenCode);

        const slot =
            this.slots.find(s => s.id === pkg.slotId);

        if (!slot)
            throw new Error("Slot not found");

        slot.release();

        pkg.status = PackageStatus.PICKED_UP;
        pkg.token!.status = TokenStatus.USED;

        console.log("Package picked up successfully");
    }

    markOutOfService() {
        this.setState(new OutOfServiceState());
    }
}

// =====================================================
// MAIN
// =====================================================

function main() {

    const slots = [
        new LockerSlot("S1", SlotSize.SMALL),
        new LockerSlot("S2", SlotSize.MEDIUM),
        new LockerSlot("S3", SlotSize.LARGE)
    ];

    const locker =
        new LockerSystem(slots, new FirstFitStrategy());

    const pkg =
        new Package("PKG1", SlotSize.SMALL);

    PackageManager.getInstance().addPackage(pkg);

    console.log("---- DELIVERY ----");
    locker.deposit(pkg);

    const token = pkg.token!.code;
    console.log("Generated Token:", token);

    console.log("---- CUSTOMER PICKUP ----");
    locker.setOpenStrategy(new OTPStrategy());
    locker.open(token);
}

main();