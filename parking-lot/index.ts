enum VehicleType {
    CAR = "CAR",
    BIKE = "BIKE",
    TRUCK = "TRUCK"
}

enum SpotType {
    SMALL = "SMALL",
    MEDIUM = "MEDIUM",
    LARGE = "LARGE",
}

enum TicketStatus {
    ACTIVE = "ACTIVE",
    PAID = "PAID",
}



//VECHILE CREATION - FACTORY DESIGN PATTERN
abstract class Vehicle {
    constructor(public licensePlate: string, public type: VehicleType) { }
}

class Car extends Vehicle {
    constructor(licensePlate: string) {
        super(licensePlate, VehicleType.CAR);
    }
}
class Bike extends Vehicle {
    constructor(licensePlate: string) {
        super(licensePlate, VehicleType.BIKE);
    }
}
class Truck extends Vehicle {
    constructor(licensePlate: string) {
        super(licensePlate, VehicleType.TRUCK);
    }
}

class VehicleFactory {
    static createVehicle(type: VehicleType, plate: string): Vehicle {
        switch (type) {
            case VehicleType.BIKE:
                return new Bike(plate);
            case VehicleType.CAR:
                return new Car(plate);
            case VehicleType.TRUCK:
                return new Truck(plate);
            default:
                throw new Error("Invalid vehicle type");
        }
    }
}



//PARKING SPOT
class ParkingSpot {
    private vechile: Vehicle | null = null;

    constructor(public id: number, public type: SpotType) { }

    isAvailable(): boolean {
        return this.vechile === null;
    }

    //what we can also do is one type of parking spot can fit vechile of same size not 
    // small size but here we are doing that small can fit bike and medium can fit bike 
    // and car and large can fit all. Depends on interview
    canFit(vehicle: Vehicle): boolean {
        if (this.type === SpotType.SMALL) {
            return vehicle.type === VehicleType.BIKE;
        } else if (this.type === SpotType.MEDIUM) {
            return vehicle.type === VehicleType.CAR || vehicle.type === VehicleType.BIKE;
        } else if (this.type === SpotType.LARGE) {
            return true; // Can fit any vehicle
        }
        return false;
    }

    park(vehicle: Vehicle) {
        if (!this.isAvailable()) {
            throw new Error("Spot is already occupied");
        }

        if (!this.canFit(vehicle)) {
            throw new Error("Vehicle cannot fit in this spot");
        }
        this.vechile = vehicle;
    }

    unpark() {
        if (this.isAvailable()) {
            throw new Error("Spot is already empty");
        }
        this.vechile = null;
    }

}

class ParkingFloor {
    constructor(
        public level: number,
        private spots: ParkingSpot[]
    ) { }

    findAvailableSpot(vehicle: Vehicle): ParkingSpot | null {
        for (const spot of this.spots) {
            if (spot.isAvailable() && spot.canFit(vehicle)) {
                return spot;
            }
        }
        return null;
    }
}

class Ticket {
    public exitTime?: Date;
    public status: TicketStatus = TicketStatus.ACTIVE;

    constructor(
        public id: string,
        public vehicle: Vehicle,
        public spot: ParkingSpot,
        public entryTime: Date
    ) { }

    closeTicket() {
        this.exitTime = new Date();
        this.status = TicketStatus.PAID;
    }

    getDurationInHours(): number {
        if (!this.exitTime) throw new Error("Exit time not set");
        const diffMs = this.exitTime.getTime() - this.entryTime.getTime();
        return Math.ceil(diffMs / (1000 * 60 * 60));
    }
}


//PRICING STRATEGY - STRATEGY DESIGN PATTERN
interface PricingStrategy {
    calculate(ticket: Ticket): number;
}
// For simplicity, we are using a flat hourly rate based on vehicle type
class HourlyPricingStrategy implements PricingStrategy {
    private rates = {
        [VehicleType.BIKE]: 5,
        [VehicleType.CAR]: 10,
        [VehicleType.TRUCK]: 15,
    };

    calculate(ticket: Ticket): number {
        const hours = ticket.getDurationInHours();
        return hours * this.rates[ticket.vehicle.type];
    }
}

//PAYMENT STRATEGY - STRATEGY DESIGN PATTERN
interface PaymentStrategy {
    pay(amount: number): void;
}

class CashPayment implements PaymentStrategy {
    pay(amount: number): void {
        console.log(`Paid ₹${amount} using Cash`);
    }
}

class UPIPayment implements PaymentStrategy {
    pay(amount: number): void {
        console.log(`Paid ₹${amount} using UPI`);
    }
}

class CardPayment implements PaymentStrategy {
    pay(amount: number): void {
        console.log(`Paid ₹${amount} using Card`);
    }
}



//PARKING LOT - SINGLETON DESIGN PATTERN and FACADE DESIGN PATTERN

class ParkingLot {
    private static instance: ParkingLot;
    private floors: ParkingFloor[] = [];
    private activeTickets: Map<string, Ticket> = new Map();
    private pricingStrategy: PricingStrategy;

    private constructor(pricingStrategy: PricingStrategy) {
        this.pricingStrategy = pricingStrategy;
    }

    static getInstance(pricingStrategy: PricingStrategy): ParkingLot {
        if (!ParkingLot.instance) {
            ParkingLot.instance = new ParkingLot(pricingStrategy);
        }
        return ParkingLot.instance;
    }

    addFloor(floor: ParkingFloor) {
        this.floors.push(floor);
    }

    parkVehicle(vehicle: Vehicle): Ticket {
        for (const floor of this.floors) {
            const spot = floor.findAvailableSpot(vehicle);
            if (spot) {
                spot.park(vehicle);
                const ticket = new Ticket(
                    `TICKET-${Date.now()}`,
                    vehicle,
                    spot,
                    new Date()
                );
                this.activeTickets.set(ticket.id, ticket);
                return ticket;
            }
        }
        throw new Error("No available spots");
    }

    exitVehicle(ticketId: string, paymentStrategy: PaymentStrategy): number {
        const ticket = this.activeTickets.get(ticketId);
        if (!ticket) throw new Error("Invalid ticket");

        ticket.closeTicket();
        const amount = this.pricingStrategy.calculate(ticket);
        paymentStrategy.pay(amount);

        ticket.spot.unpark();
        this.activeTickets.delete(ticketId);

        return amount;
    }
}

function main() {
  const pricingStrategy = new HourlyPricingStrategy();
  const parkingLot = ParkingLot.getInstance(pricingStrategy);

  const floor1 = new ParkingFloor(1, [
    new ParkingSpot(1, SpotType.SMALL),
    new ParkingSpot(2, SpotType.MEDIUM),
    new ParkingSpot(3, SpotType.LARGE),
  ]);

  parkingLot.addFloor(floor1);

  const vehicle = VehicleFactory.createVehicle(VehicleType.CAR, "DL01AB1234");

  const ticket = parkingLot.parkVehicle(vehicle);
  console.log("Vehicle Parked. Ticket:", ticket.id);

  // simulate exit
  setTimeout(() => {
    const amount = parkingLot.exitVehicle(ticket.id, new UPIPayment());
    console.log("Total Paid:", amount);
  }, 2000);
}

main();