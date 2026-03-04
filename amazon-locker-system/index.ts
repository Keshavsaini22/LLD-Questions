// ==========================
// ENUMS
// ==========================

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

// ==========================
// ENTITIES
// ==========================

class LockerSlot {

    private mutex = false;

    constructor(
        public id: string,
        public size: SlotSize,
        public status: SlotStatus = SlotStatus.AVAILABLE,
        public packageId?: string
    ) { }

    reserve(packageId: string): boolean {
        if (this.mutex) return false;
        this.mutex = true;

        try {
            if (this.status !== SlotStatus.AVAILABLE) return false;
            this.status = SlotStatus.RESERVED;
            this.packageId = packageId;
            return true;
        } finally {
            this.mutex = false;
        }
    }

    occupy() {
        this.status = SlotStatus.OCCUPIED;
    }

    release() {
        this.status = SlotStatus.AVAILABLE;
        this.packageId = undefined;
    }

    markExpired() {
        this.status = SlotStatus.EXPIRED;
    }
}

class AccessToken {
    constructor(
        public code: string,
        public packageId: string,
        public createdAt: Date = new Date(),
        public status: TokenStatus = TokenStatus.ACTIVE
    ) { }

    isExpired(): boolean {
        const expiryTime =
            this.createdAt.getTime() + 7 * 24 * 60 * 60 * 1000;
        return Date.now() > expiryTime;
    }
}

class Package {
    constructor(
        public id: string,
        public size: SlotSize,
        public zipcode: string,
        public status: PackageStatus = PackageStatus.CREATED,
        public slotId?: string,
        public token?: AccessToken
    ) { }
}

// ==========================
// STRATEGIES
// ==========================

interface SlotAssignmentStrategy {
    assign(slots: LockerSlot[], size: SlotSize): LockerSlot | null;
}

class FirstFitStrategy implements SlotAssignmentStrategy {
    assign(slots: LockerSlot[], size: SlotSize): LockerSlot | null {
        return (
            slots.find(
                s => s.size === size &&
                    s.status === SlotStatus.AVAILABLE
            ) || null
        );
    }
}

interface SlotOpenStrategy {
    validate(token: string, system: LockerSystem): boolean;
}

class OTPStrategy implements SlotOpenStrategy {
    validate(token: string, system: LockerSystem): boolean {
        return system.validateToken(token);
    }
}

class BarcodeStrategy implements SlotOpenStrategy {
    validate(token: string, system: LockerSystem): boolean {
        return system.validateToken(token);
    }
}

// ==========================
// MANAGERS (Singleton)
// ==========================

class PackageManager {

    private static instance: PackageManager;
    private packages = new Map<string, Package>();
    private tokens = new Map<string, AccessToken>();

    private constructor() { }

    static getInstance(): PackageManager {
        if (!this.instance) {
            this.instance = new PackageManager();
        }
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
            "TOKEN-" + Math.random().toString(36).substring(2),
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

        return this.getPackage(token.packageId);
    }
}

class SlotManager {

    constructor(
        private strategy: SlotAssignmentStrategy
    ) { }

    reserveSlot(
        slots: LockerSlot[],
        size: SlotSize,
        packageId: string
    ): LockerSlot {

        const slot = this.strategy.assign(slots, size);
        if (!slot) throw new Error("No slot available");

        if (!slot.reserve(packageId))
            throw new Error("Slot reservation failed");

        slot.occupy();
        return slot;
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

// ==========================
// LOCKER SYSTEM (Facade)
// ==========================

class LockerSystem {

    private openStrategy!: SlotOpenStrategy;

    constructor(
        public slots: LockerSlot[],
        public slotManager: SlotManager
    ) { }

    setOpenStrategy(strategy: SlotOpenStrategy) {
        this.openStrategy = strategy;
    }

    deposit(pkg: Package) {

        const slot = this.slotManager.reserveSlot(
            this.slots,
            pkg.size,
            pkg.id
        );

        pkg.slotId = slot.id;
        pkg.status = PackageStatus.DELIVERED_TO_LOCKER;

        const token =
            PackageManager.getInstance().generateToken(pkg);

        NotificationService.getInstance()
            .notifyUser(pkg.id, token.code);

        this.startAutoExpiry(pkg);
    }

    open(tokenCode: string) {

        if (!this.openStrategy)
            throw new Error("No open strategy set");

        if (!this.openStrategy.validate(tokenCode, this))
            throw new Error("Invalid credential");

        const pkg =
            PackageManager.getInstance()
                .validateToken(tokenCode);

        const slot =
            this.slots.find(s => s.id === pkg.slotId);

        if (!slot) throw new Error("Slot not found");

        slot.release();

        pkg.status = PackageStatus.PICKED_UP;
        pkg.token!.status = TokenStatus.USED;

        console.log("Package picked up successfully");
    }

    validateToken(token: string): boolean {
        try {
            PackageManager.getInstance().validateToken(token);
            return true;
        } catch {
            return false;
        }
    }

    private startAutoExpiry(pkg: Package) {

        setTimeout(() => {

            if (pkg.status === PackageStatus.DELIVERED_TO_LOCKER) {

                const slot =
                    this.slots.find(s => s.id === pkg.slotId);

                if (slot) slot.markExpired();

                pkg.status = PackageStatus.RETURNED;

                console.log(
                    "Package expired and returned"
                );
            }

        }, 5 * 60 * 1000); // 5 mins
    }
}

// ==========================
// MAIN
// ==========================

function main() {

    const slots = [
        new LockerSlot("S1", SlotSize.SMALL),
        new LockerSlot("S2", SlotSize.MEDIUM),
        new LockerSlot("S3", SlotSize.LARGE)
    ];

    const slotManager =
        new SlotManager(new FirstFitStrategy());

    const lockerSystem =
        new LockerSystem(slots, slotManager);

    const pkg =
        new Package("PKG1", SlotSize.SMALL, "560001");

    PackageManager.getInstance().addPackage(pkg);

    // DELIVERY
    lockerSystem.deposit(pkg);

    const token = pkg.token!.code;

    console.log("Generated Token:", token);

    // CUSTOMER PICKUP
    lockerSystem.setOpenStrategy(new OTPStrategy());
    lockerSystem.open(token);
}

main();