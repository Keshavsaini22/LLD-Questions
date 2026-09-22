#include <bits/stdc++.h>
using namespace std;

// ================= ENUMS =================

enum class VehicleType {
    SMALL,
    MEDIUM,
    LARGE
};

enum class SpotType {
    SMALL,
    MEDIUM,
    LARGE
};

enum class TicketStatus {
    ACTIVE,
    CLOSED
};

enum class PaymentStatus {
    SUCCESS,
    FAILED
};

// ================= VEHICLE =================

class Vehicle {
protected:
    string licensePlate;
    VehicleType type;

public:
    Vehicle(string licensePlate, VehicleType type)
        : licensePlate(move(licensePlate)), type(type) {}

    virtual ~Vehicle() = default;

    VehicleType getType() const {
        return type;
    }

    string getLicensePlate() const {
        return licensePlate;
    }
};

class Bike : public Vehicle {
public:
    explicit Bike(string plate)
        : Vehicle(move(plate), VehicleType::SMALL) {}
};

class Car : public Vehicle {
public:
    explicit Car(string plate)
        : Vehicle(move(plate), VehicleType::MEDIUM) {}
};

class Truck : public Vehicle {
public:
    explicit Truck(string plate)
        : Vehicle(move(plate), VehicleType::LARGE) {}
};

// ================= VEHICLE FACTORY =================

class VehicleFactory {
public:
    static unique_ptr<Vehicle> createVehicle(
        VehicleType type,
        const string& plate
    ) {
        switch (type) {
            case VehicleType::SMALL:
                return make_unique<Bike>(plate);

            case VehicleType::MEDIUM:
                return make_unique<Car>(plate);

            case VehicleType::LARGE:
                return make_unique<Truck>(plate);
        }

        throw invalid_argument("Invalid vehicle type");
    }
};

// ================= PARKING SPOT =================

class ParkingSpot {
private:
    int id;
    SpotType type;
    Vehicle* vehicle;
    mutable mutex mtx;

public:
    ParkingSpot(int id, SpotType type)
        : id(id), type(type), vehicle(nullptr) {}

    bool canFit(const Vehicle& v) const {
        if (type == SpotType::SMALL)
            return v.getType() == VehicleType::SMALL;

        if (type == SpotType::MEDIUM)
            return v.getType() == VehicleType::SMALL ||
                   v.getType() == VehicleType::MEDIUM;

        return true;
    }

    bool tryPark(Vehicle* v) {
        lock_guard<mutex> lock(mtx);

        if (vehicle != nullptr)
            return false;

        if (!canFit(*v))
            return false;

        vehicle = v;
        return true;
    }

    void unpark() {
        lock_guard<mutex> lock(mtx);

        if (vehicle == nullptr)
            throw runtime_error("Spot is already empty");

        vehicle = nullptr;
    }

    bool isAvailable() const {
        lock_guard<mutex> lock(mtx);
        return vehicle == nullptr;
    }

    int getId() const {
        return id;
    }

    SpotType getType() const {
        return type;
    }
};

// ================= PARKING FLOOR =================

class ParkingFloor {
private:
    int floorNumber;
    vector<unique_ptr<ParkingSpot>> spots;

public:
    explicit ParkingFloor(int floorNumber)
        : floorNumber(floorNumber) {}

    void addSpot(unique_ptr<ParkingSpot> spot) {
        spots.push_back(move(spot));
    }

    ParkingSpot* findAndReserveSpot(Vehicle* vehicle) {
        for (auto& spot : spots) {
            if (spot->tryPark(vehicle))
                return spot.get();
        }

        return nullptr;
    }

    int getFloorNumber() const {
        return floorNumber;
    }
};

// ================= TICKET =================

class Ticket {
private:
    string id;
    Vehicle* vehicle;
    ParkingSpot* spot;

    time_t entryTime;
    time_t exitTime;

    TicketStatus status;

public:
    Ticket(
        string id,
        Vehicle* vehicle,
        ParkingSpot* spot
    )
        : id(move(id)),
          vehicle(vehicle),
          spot(spot),
          entryTime(time(nullptr)),
          exitTime(0),
          status(TicketStatus::ACTIVE) {}

    void close() {
        if (status == TicketStatus::CLOSED)
            throw runtime_error("Ticket already closed");

        exitTime = time(nullptr);
        status = TicketStatus::CLOSED;
    }

    int getDurationHours() const {
        if (status != TicketStatus::CLOSED)
            throw runtime_error("Ticket is still active");

        double seconds = difftime(exitTime, entryTime);

        return max(
            1,
            static_cast<int>(ceil(seconds / 3600.0))
        );
    }

    const string& getId() const {
        return id;
    }

    Vehicle* getVehicle() const {
        return vehicle;
    }

    ParkingSpot* getSpot() const {
        return spot;
    }

    TicketStatus getStatus() const {
        return status;
    }
};

// ================= TICKET SERVICE =================

class TicketService {
private:
    atomic<long long> counter{0};

public:
    unique_ptr<Ticket> createTicket(
        Vehicle* vehicle,
        ParkingSpot* spot
    ) {
        string id = "TICKET-" + to_string(++counter);

        return make_unique<Ticket>(
            id,
            vehicle,
            spot
        );
    }
};

// ================= PRICING =================

class PricingStrategy {
public:
    virtual ~PricingStrategy() = default;

    virtual double calculate(
        const Ticket& ticket
    ) const = 0;
};

class HourlyPricingStrategy : public PricingStrategy {
public:
    double calculate(const Ticket& ticket) const override {
        int hours = ticket.getDurationHours();

        switch (ticket.getVehicle()->getType()) {
            case VehicleType::SMALL:
                return hours * 5;

            case VehicleType::MEDIUM:
                return hours * 10;

            case VehicleType::LARGE:
                return hours * 15;
        }

        throw runtime_error("Invalid vehicle type");
    }
};

// ================= PAYMENT =================

class PaymentStrategy {
public:
    virtual ~PaymentStrategy() = default;

    virtual PaymentStatus pay(double amount) = 0;
};

class UPIPayment : public PaymentStrategy {
public:
    PaymentStatus pay(double amount) override {
        cout << "Paid Rs." << amount << " using UPI\n";
        return PaymentStatus::SUCCESS;
    }
};

class CashPayment : public PaymentStrategy {
public:
    PaymentStatus pay(double amount) override {
        cout << "Paid Rs." << amount << " using Cash\n";
        return PaymentStatus::SUCCESS;
    }
};

// ================= SPOT MANAGER =================

class SpotManager {
private:
    vector<unique_ptr<ParkingFloor>> floors;

public:
    void addFloor(unique_ptr<ParkingFloor> floor) {
        floors.push_back(move(floor));
    }

    ParkingSpot* reserveSpot(Vehicle* vehicle) {
        for (auto& floor : floors) {
            ParkingSpot* spot =
                floor->findAndReserveSpot(vehicle);

            if (spot)
                return spot;
        }

        return nullptr;
    }
};

// ================= PARKING LOT =================

class ParkingLot {
private:
    SpotManager spotManager;
    TicketService ticketService;

    unique_ptr<PricingStrategy> pricingStrategy;

    unordered_map<string, unique_ptr<Ticket>> activeTickets;

    mutex ticketMutex;

public:
    explicit ParkingLot(
        unique_ptr<PricingStrategy> pricingStrategy
    )
        : pricingStrategy(move(pricingStrategy)) {}

    void addFloor(unique_ptr<ParkingFloor> floor) {
        spotManager.addFloor(move(floor));
    }

    Ticket* enter(Vehicle* vehicle) {

        ParkingSpot* spot =
            spotManager.reserveSpot(vehicle);

        if (!spot)
            throw runtime_error("No parking spot available");

        auto ticket =
            ticketService.createTicket(vehicle, spot);

        Ticket* ticketPtr = ticket.get();

        {
            lock_guard<mutex> lock(ticketMutex);

            activeTickets[ticketPtr->getId()] =
                move(ticket);
        }

        return ticketPtr;
    }

    double exit(
        const string& ticketId,
        PaymentStrategy& paymentStrategy
    ) {
        Ticket* ticket = nullptr;

        {
            lock_guard<mutex> lock(ticketMutex);

            auto it = activeTickets.find(ticketId);

            if (it == activeTickets.end())
                throw runtime_error("Invalid ticket");

            ticket = it->second.get();
        }

        ticket->close();

        double amount =
            pricingStrategy->calculate(*ticket);

        PaymentStatus status =
            paymentStrategy.pay(amount);

        if (status == PaymentStatus::FAILED) {
            throw runtime_error("Payment failed");
        }

        ParkingSpot* spot = ticket->getSpot();

        spot->unpark();

        {
            lock_guard<mutex> lock(ticketMutex);

            activeTickets.erase(ticketId);
        }

        return amount;
    }
};

// ================= MAIN =================

int main() {

    ParkingLot parkingLot(
        make_unique<HourlyPricingStrategy>()
    );

    auto floor1 =
        make_unique<ParkingFloor>(1);

    floor1->addSpot(
        make_unique<ParkingSpot>(
            1,
            SpotType::SMALL
        )
    );

    floor1->addSpot(
        make_unique<ParkingSpot>(
            2,
            SpotType::MEDIUM
        )
    );

    floor1->addSpot(
        make_unique<ParkingSpot>(
            3,
            SpotType::LARGE
        )
    );

    parkingLot.addFloor(move(floor1));

    auto car =
        VehicleFactory::createVehicle(
            VehicleType::MEDIUM,
            "PB10AB1234"
        );

    Ticket* ticket =
        parkingLot.enter(car.get());

    cout << "Ticket: "
         << ticket->getId()
         << endl;

    this_thread::sleep_for(
        chrono::seconds(2)
    );

    UPIPayment payment;

    double amount =
        parkingLot.exit(
            ticket->getId(),
            payment
        );

    cout << "Total paid: Rs."
         << amount
         << endl;

    return 0;
}

