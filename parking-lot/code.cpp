#include <iostream>
#include <bits/stdc++.h>
using namespace std;

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
    PAID
};

// ---------------- VEHICLE ----------------
class Vehicle {
public:
    string licensePlate;
    VehicleType type;

    Vehicle(string plate, VehicleType type) {
        this->licensePlate = plate;
        this->type = type;
    }

    virtual ~Vehicle() {}
};

class Bike : public Vehicle {
public:
    Bike(string plate) : Vehicle(plate, VehicleType::SMALL) {}
};

class Car : public Vehicle {
public:
    Car(string plate) : Vehicle(plate, VehicleType::MEDIUM) {}
};

class Truck : public Vehicle {
public:
    Truck(string plate) : Vehicle(plate, VehicleType::LARGE) {}
};

// Factory Pattern
class VehicleFactory {
public:
    static Vehicle* createVehicle(VehicleType type, string plate) {
        switch (type) {
            case VehicleType::SMALL:
                return new Bike(plate);
            case VehicleType::MEDIUM:
                return new Car(plate);
            case VehicleType::LARGE:
                return new Truck(plate);
            default:
                throw runtime_error("Invalid vehicle type");
        }
    }
};

// ---------------- PARKING SPOT ----------------
class ParkingSpot {
private:
    Vehicle* vehicle = nullptr;

public:
    int id;
    SpotType type;

    ParkingSpot(int id, SpotType type) {
        this->id = id;
        this->type = type;
    }

    bool isAvailable() {
        return vehicle == nullptr;
    }

    bool canFit(Vehicle* v) {
        if (type == SpotType::SMALL) {
            return v->type == VehicleType::SMALL;
        } else if (type == SpotType::MEDIUM) {
            return v->type == VehicleType::SMALL ||
                   v->type == VehicleType::MEDIUM;
        } else {
            return true; // LARGE fits all
        }
    }

    void park(Vehicle* v) {
        if (!isAvailable()) throw runtime_error("Spot occupied");
        if (!canFit(v)) throw runtime_error("Vehicle can't fit");
        vehicle = v;
    }

    void unpark() {
        if (isAvailable()) throw runtime_error("Already empty");
        vehicle = nullptr;
    }
};

// ---------------- FLOOR ----------------
class ParkingFloor {
public:
    int level;
    vector<ParkingSpot*> spots;

    ParkingFloor(int level, vector<ParkingSpot*> spots) {
        this->level = level;
        this->spots = spots;
    }

    ParkingSpot* findAvailableSpot(Vehicle* vehicle) {
        for (auto spot : spots) {
            if (spot->isAvailable() && spot->canFit(vehicle)) {
                return spot;
            }
        }
        return nullptr;
    }
};

// ---------------- TICKET ----------------
class Ticket {
public:
    string id;
    Vehicle* vehicle;
    ParkingSpot* spot;
    time_t entryTime;
    time_t exitTime;
    TicketStatus status;

    Ticket(string id, Vehicle* vehicle, ParkingSpot* spot) {
        this->id = id;
        this->vehicle = vehicle;
        this->spot = spot;
        this->entryTime = time(0);
        this->status = TicketStatus::ACTIVE;
    }

    void closeTicket() {
        exitTime = time(0);
        status = TicketStatus::PAID;
    }

    int getDurationHours() {
        double seconds = difftime(exitTime, entryTime);
        return max(1, (int)ceil(seconds / 3600.0));
    }
};

// ---------------- PRICING STRATEGY ----------------
class PricingStrategy {
public:
    virtual int calculate(Ticket* ticket) = 0;
};

class HourlyPricingStrategy : public PricingStrategy {
public:
    int calculate(Ticket* ticket) override {
        int hours = ticket->getDurationHours();

        if (ticket->vehicle->type == VehicleType::SMALL) return hours * 5;
        if (ticket->vehicle->type == VehicleType::MEDIUM) return hours * 10;
        return hours * 15;
    }
};

// ---------------- PAYMENT STRATEGY ----------------
class PaymentStrategy {
public:
    virtual void pay(int amount) = 0;
};

class UPIPayment : public PaymentStrategy {
public:
    void pay(int amount) override {
        cout << "Paid Rs." << amount << " using UPI\n";
    }
};

class CashPayment : public PaymentStrategy {
public:
    void pay(int amount) override {
        cout << "Paid Rs." << amount << " using Cash\n";
    }
};

// ---------------- PARKING LOT SINGLETON ----------------
class ParkingLot {
private:
    static ParkingLot* instance;
    vector<ParkingFloor*> floors;
    unordered_map<string, Ticket*> activeTickets;
    PricingStrategy* pricingStrategy;

    ParkingLot(PricingStrategy* strategy) {
        pricingStrategy = strategy;
    }

public:
    static ParkingLot* getInstance(PricingStrategy* strategy) {
        if (!instance) {
            instance = new ParkingLot(strategy);
        }
        return instance;
    }

    void addFloor(ParkingFloor* floor) {
        floors.push_back(floor);
    }

    Ticket* parkVehicle(Vehicle* vehicle) {
        for (auto floor : floors) {
            ParkingSpot* spot = floor->findAvailableSpot(vehicle);
            if (spot) {
                spot->park(vehicle);

                string ticketId = "TICKET-" + to_string(time(0));
                Ticket* ticket = new Ticket(ticketId, vehicle, spot);
                activeTickets[ticketId] = ticket;

                return ticket;
            }
        }
        throw runtime_error("No spots available");
    }

    int exitVehicle(string ticketId, PaymentStrategy* paymentStrategy) {
        if (activeTickets.find(ticketId) == activeTickets.end()) {
            throw runtime_error("Invalid ticket");
        }

        Ticket* ticket = activeTickets[ticketId];
        ticket->closeTicket();

        int amount = pricingStrategy->calculate(ticket);
        paymentStrategy->pay(amount);

        ticket->spot->unpark();
        activeTickets.erase(ticketId);

        return amount;
    }
};

ParkingLot* ParkingLot::instance = nullptr;

// ---------------- MAIN ----------------
int main() {
    PricingStrategy* pricing = new HourlyPricingStrategy();
    ParkingLot* parkingLot = ParkingLot::getInstance(pricing);

    ParkingFloor* floor1 = new ParkingFloor(1, {
        new ParkingSpot(1, SpotType::SMALL),
        new ParkingSpot(2, SpotType::MEDIUM),
        new ParkingSpot(3, SpotType::LARGE)
    });

    parkingLot->addFloor(floor1);

    Vehicle* vehicle = VehicleFactory::createVehicle(
        VehicleType::MEDIUM,
        "PB10AB1234"
    );

    Ticket* ticket = parkingLot->parkVehicle(vehicle);
    cout << "Vehicle parked. Ticket: " << ticket->id << endl;

    // simulate exit
    this_thread::sleep_for(chrono::seconds(2));

    int amount = parkingLot->exitVehicle(ticket->id, new UPIPayment());
    cout << "Total paid: Rs." << amount << endl;

    return 0;
}