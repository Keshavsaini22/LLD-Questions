class PaymentRequest {
    constructor(
        public sender: string,
        public receiver: string,
        public amount: number,
        public currency: string,
    ) { }
}


// Banking System interface and implementations (Strategy for actual payment logic)
interface BankingSystem {
    processPayment(amount: number): boolean;
}

class PaytmBankingSystem implements BankingSystem {
    processPayment(amount: number): boolean {
        console.log(`Processing payment of ${amount} via Paytm Banking System.`);
        // 80% success
        return Math.floor(Math.random() * 100) < 80;
    }
}

class RazorpayBankingSystem implements BankingSystem {
    processPayment(amount: number): boolean {
        console.log(`[BankingSystem-Razorpay] Processing payment of ${amount}...`);
        // 90% success
        return Math.floor(Math.random() * 100) < 90;
    }
}


// Abstract base class for Payment Gateway (Template Method Pattern)
abstract class PaymentGateway {
    protected bankingSystem!: BankingSystem;

    processPayment(request: PaymentRequest): boolean {
        if (!this.validatePayment(request)) {
            console.log("Payment request validation failed.");
            return false;
        }

        if (!this.initiatePayment(request)) {
            console.log("Payment initiation failed.");
            return false;
        }

        if (!this.confirmPayment(request)) {
            console.log("Payment confirmation failed.");
            return false;
        }

        console.log("Payment processed successfully.");
        return true;
    }

    protected abstract validatePayment(request: PaymentRequest): boolean;
    protected abstract initiatePayment(request: PaymentRequest): boolean;
    protected abstract confirmPayment(request: PaymentRequest): boolean;
}

// Concrete Payment Gateway implementations
// Paytm Gateway

class PaytmGateway extends PaymentGateway {
    constructor() {
        super();
        this.bankingSystem = new PaytmBankingSystem();
    }

    protected validatePayment(request: PaymentRequest): boolean {
        console.log("Validating Paytm payment request...");
        return request.amount > 0 && request.currency === "INR";
    }

    protected initiatePayment(request: PaymentRequest): boolean {
        console.log("Initiating Paytm payment...");
        return this.bankingSystem.processPayment(request.amount);
    }

    protected confirmPayment(request: PaymentRequest): boolean {
        console.log("Confirming Paytm payment...");
        return true;
    }

}

// Razorpay Gateway
class RazorpayGateway extends PaymentGateway {
    constructor() {
        super();
        this.bankingSystem = new RazorpayBankingSystem();
    }

    protected validatePayment(request: PaymentRequest): boolean {
        console.log("Validating Razorpay payment request...");
        return request.amount > 0;
    }

    protected initiatePayment(request: PaymentRequest): boolean {
        console.log("Initiating Razorpay payment...");
        return this.bankingSystem.processPayment(request.amount);
    }

    protected confirmPayment(request: PaymentRequest): boolean {
        console.log("Confirming Razorpay payment...");
        return true;
    }
}

//Proxy (Retry Logic)

class PaymentGatewayProxy extends PaymentGateway {
    constructor(private realGateway: PaymentGateway, private retries: number = 3) {
        super();
    }

    processPayment(request: PaymentRequest): boolean {
        for (let attempt = 0; attempt < this.retries; attempt++) {
            if (attempt > 0) {
                console.log(
                    `[Proxy] Retrying payment (attempt ${attempt + 1}) for ${request.sender}.`
                );
            }
            if (this.realGateway.processPayment(request)) {
                return true;
            }
        }

        console.log(
            `[Proxy] Payment failed after ${this.retries} attempts for ${request.sender}.`
        );
        return false;
    }

    protected validatePayment(req: PaymentRequest): boolean {
        return this.realGateway["validatePayment"](req);
    }

    protected initiatePayment(req: PaymentRequest): boolean {
        return this.realGateway["initiatePayment"](req);
    }

    protected confirmPayment(req: PaymentRequest): boolean {
        return this.realGateway["confirmPayment"](req);
    }
}


//GATEWAY FACTORY SINGLETON
enum GatewayType {
    Paytm,
    Razorpay
}

class GatewayFactory {
    private static instance: GatewayFactory = new GatewayFactory();
    private constructor() { }
    public static getInstance(): GatewayFactory {
        return GatewayFactory.instance;
    }

    getGateway(type: GatewayType): PaymentGateway {
        switch (type) {
            case GatewayType.Paytm:
                return new PaymentGatewayProxy(new PaytmGateway());
            case GatewayType.Razorpay:
                return new PaymentGatewayProxy(new RazorpayGateway());
            default:
                throw new Error("Unsupported gateway type");
        }
    }
}

//Payment Service Singleton
class PaymentService {
    private static instance: PaymentService = new PaymentService();
    private gateway?: PaymentGateway;

    private constructor() { }
    public static getInstance(): PaymentService {
        return PaymentService.instance;
    }

    setGateway(gateway: PaymentGateway) {
        this.gateway = gateway
    }

    processPayment(request: PaymentRequest): boolean {
        if (!this.gateway) {
            throw new Error("Payment gateway not set.");
        }

        return this.gateway.processPayment(request);
    }
}

//Controller Singleton
class PaymentController {
    private static instance: PaymentController = new PaymentController()
    private constructor() { }

    public static getInstance(): PaymentController {
        return PaymentController.instance;
    }

    handlePayment(request: PaymentRequest, gatewayType: GatewayType): boolean {
        const gateway = GatewayFactory.getInstance().getGateway(gatewayType);
        const paymentService = PaymentService.getInstance();
        paymentService.setGateway(gateway);
        return paymentService.processPayment(request);
    }
}

//Client Code
const req1 = new PaymentRequest("Aditya", "Shubham", 1000, "INR");

console.log("Processing via Paytm");
console.log("------------------------------");
const res1 = PaymentController.getInstance().handlePayment(
    req1,
    GatewayType.Paytm,
);
console.log("Result:", res1 ? "SUCCESS" : "FAIL");
console.log("------------------------------\n");

const req2 = new PaymentRequest("Shubham", "Aditya", 500, "USD");

console.log("Processing via Razorpay");
console.log("------------------------------");
const res2 = PaymentController.getInstance().handlePayment(
    req2,
    GatewayType.Razorpay,
);
console.log("Result:", res2 ? "SUCCESS" : "FAIL");
console.log("------------------------------");
