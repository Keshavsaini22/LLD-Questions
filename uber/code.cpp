// why driver is geting saved in ride request and also in ride? why duplicate data is being stored?
#include <memory>
#include <unordered_map>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <bits/stdc++.h>
using namespace std;

//======================================================
// ENUMS
//======================================================

enum class RideType
{
    GO,
    XL,
    PREMIER
};

enum class RideStatus
{
    DRIVER_ASSIGNED,
    DRIVER_ARRIVING,
    ONGOING,
    COMPLETED,
    CANCELLED
};

enum class RequestStatus
{
    PENDING,
    ACCEPTED,
    CANCELLED,
    EXPIRED
};

//======================================================
// LOCATION
//======================================================

class Location
{
    double latitude;
    double longitude;

public:
    Location(
        double latitude = 0,
        double longitude = 0)
        : latitude(latitude),
          longitude(longitude) {}

    double getLatitude() const
    {
        return latitude;
    }

    double getLongitude() const
    {
        return longitude;
    }

    double distanceTo(const Location &other) const // const in the starting of the function means that this function will not modify any member variables of the class.
    // It is a promise that the function will not change the state of the object.
    {

        double dx =
            latitude -
            other.latitude;

        double dy =
            longitude -
            other.longitude;

        return sqrt(dx * dx + dy * dy);
    }
};

//======================================================
// VEHICLE
//======================================================

class Vehicle
{

    string vehicleNumber;

    RideType rideType;

public:
    Vehicle(
        string vehicleNumber,
        RideType rideType)
        : vehicleNumber(vehicleNumber),
          rideType(rideType) {}

    RideType getRideType() const
    {
        return rideType;
    }

    string getVehicleNumber() const
    {
        return vehicleNumber;
    }
};

//======================================================
// RIDER
//======================================================

class Rider
{

    int riderId;

    string name;

public:
    Rider(
        int riderId,
        string name)
        : riderId(riderId),
          name(name) {}

    int getId() const
    {
        return riderId;
    }

    string getName() const
    {
        return name;
    }
};

//======================================================
// DRIVER
//======================================================

class Driver
{

    int driverId;

    string name;

    Location currentLocation;

    Vehicle vehicle;

    bool available;

public:
    Driver(
        int driverId,
        string name,
        Location currentLocation,
        Vehicle vehicle)
        : driverId(driverId),
          name(name),
          currentLocation(currentLocation),
          vehicle(vehicle),
          available(true) {}

    int getId() const
    {
        return driverId;
    }

    string getName() const
    {
        return name;
    }

    bool isAvailable() const
    {
        return available;
    }

    void setAvailability(bool value)
    {
        available = value;
    }

    Location getLocation() const
    {
        return currentLocation;
    }

    Vehicle getVehicle() const
    {
        return vehicle;
    }
};

//======================================================
// RIDE QUOTE
//======================================================

class RideQuote
{

    RideType rideType;

    double estimatedFare;

    int etaMinutes;

public:
    RideQuote(
        RideType rideType,
        double estimatedFare,
        int etaMinutes)
        : rideType(rideType),
          estimatedFare(estimatedFare),
          etaMinutes(etaMinutes)
    {
    }

    RideType getRideType() const
    {
        return rideType;
    }

    double getEstimatedFare() const
    {
        return estimatedFare;
    }

    int getEtaMinutes() const
    {
        return etaMinutes;
    }
};

//======================================================
// RIDE REQUEST
//======================================================

class RideRequest
{

    int requestId;

    shared_ptr<Rider> rider;

    Location source;

    Location destination;

    RideQuote selectedQuote;

    RequestStatus status;

    shared_ptr<Driver> assignedDriver;

    mutable mutex requestMutex;

public:
    RideRequest(
        int requestId,
        shared_ptr<Rider> rider,
        const Location &source,
        const Location &destination,
        const RideQuote &selectedQuote)
        : requestId(requestId),
          rider(rider),
          source(source),
          destination(destination),
          selectedQuote(selectedQuote),
          status(RequestStatus::PENDING)
    {
    }

    int getRequestId() const
    {
        return requestId;
    }

    shared_ptr<Rider> getRider() const
    {
        return rider;
    }

    const Location &getSource() const
    {
        return source;
    }

    const Location &getDestination() const
    {
        return destination;
    }

    RideType getRideType() const
    {
        return selectedQuote.getRideType();
    }

    double getEstimatedFare() const
    {
        return selectedQuote.getEstimatedFare();
    }

    RequestStatus getStatus() const
    {
        return status;
    }

    bool acceptDriver(
        shared_ptr<Driver> driver)
    {
        lock_guard<mutex> lock(requestMutex);

        if (status != RequestStatus::PENDING)
        {
            return false;
        }

        assignedDriver = driver;
        status = RequestStatus::ACCEPTED;

        return true;
    }

    void cancelRequest()
    {
        lock_guard<mutex> lock(requestMutex);

        if (status == RequestStatus::PENDING)
        {
            status = RequestStatus::CANCELLED;
        }
    }

    shared_ptr<Driver> getAssignedDriver() const
    {
        lock_guard<mutex> lock(requestMutex);
        return assignedDriver;
    }
};

//======================================================
// RIDE
//======================================================

class Ride
{

    int rideId;

    shared_ptr<Rider> rider;

    shared_ptr<Driver> driver;

    Location source;

    Location destination;

    RideType rideType;

    double fare;

    RideStatus status;

public:
    Ride(
        int rideId,
        shared_ptr<Rider> rider,
        shared_ptr<Driver> driver,
        const Location &source,
        const Location &destination,
        RideType rideType,
        double fare)
        : rideId(rideId),
          rider(rider),
          driver(driver),
          source(source),
          destination(destination),
          rideType(rideType),
          fare(fare),
          status(RideStatus::DRIVER_ASSIGNED)
    {
    }

    int getRideId() const
    {
        return rideId;
    }

    RideStatus getStatus() const
    {
        return status;
    }

    void startRide()
    {
        status = RideStatus::ONGOING;
    }

    void completeRide()
    {
        status = RideStatus::COMPLETED;
    }

    void cancelRide()
    {
        status = RideStatus::CANCELLED;
    }

    shared_ptr<Driver> getDriver() const
    {
        return driver;
    }

    shared_ptr<Rider> getRider() const
    {
        return rider;
    }

    double getFare() const
    {
        return fare;
    }
};

//======================================================
// FARE STRATEGY
//======================================================

class FareStrategy
{
public:
    virtual ~FareStrategy() = default;

    virtual vector<RideQuote> calculateFare(
        const Location &source,
        const Location &destination) = 0;
};

//======================================================
// NORMAL FARE STRATEGY
//======================================================

class NormalFareStrategy : public FareStrategy
{
public:
    vector<RideQuote> calculateFare(
        const Location &source,
        const Location &destination) override
    {
        double distance =
            source.distanceTo(destination);

        vector<RideQuote> quotes;

        quotes.emplace_back(
            RideType::GO,
            50 + distance * 10,
            3);

        quotes.emplace_back(
            RideType::XL,
            100 + distance * 15,
            5);

        quotes.emplace_back(
            RideType::PREMIER,
            200 + distance * 25,
            7);

        return quotes;
    }
};
//======================================================
// SURGE FARE STRATEGY
//======================================================

class SurgeFareStrategy : public FareStrategy
{
    double surgeMultiplier;

public:
    SurgeFareStrategy(
        double surgeMultiplier)
        : surgeMultiplier(surgeMultiplier)
    {
    }

    vector<RideQuote> calculateFare(
        const Location &source,
        const Location &destination) override
    {
        double distance =
            source.distanceTo(destination);

        vector<RideQuote> quotes;

        quotes.emplace_back(
            RideType::GO,
            (50 + distance * 10) * surgeMultiplier,
            3);

        quotes.emplace_back(
            RideType::XL,
            (100 + distance * 15) * surgeMultiplier,
            5);

        quotes.emplace_back(
            RideType::PREMIER,
            (200 + distance * 25) * surgeMultiplier,
            7);

        return quotes;
    }
};

//======================================================
// DRIVER MATCHING STRATEGY
//======================================================

class DriverMatchingStrategy
{

public:
    virtual ~DriverMatchingStrategy() = default;

    virtual vector<shared_ptr<Driver>> findDrivers(
        const RideRequest &request,
        const vector<
            shared_ptr<Driver>> &drivers) = 0;
};

//======================================================
// NEAREST DRIVER STRATEGY
//======================================================

class NearestDriverStrategy
    : public DriverMatchingStrategy
{

public:
    vector<
        shared_ptr<Driver>>
    findDrivers(
        const RideRequest &request,
        const vector<
            shared_ptr<Driver>> &drivers) override
    {
        vector<
            shared_ptr<Driver>>
            candidates;

        for (auto &driver : drivers)
        {
            if (
                !driver->isAvailable())
            {
                continue;
            }

            if (
                driver
                    ->getVehicle()
                    .getRideType() !=
                request.getRideType())
            {
                continue;
            }

            candidates.push_back(
                driver);
        }

        sort(
            candidates.begin(),
            candidates.end(),
            [&](auto &d1,
                auto &d2)
            {
                return d1->getLocation()
                           .distanceTo(
                               request.getSource()) <
                       d2->getLocation()
                           .distanceTo(
                               request.getSource());
            });

        return candidates;
    }
};

// Or HIGHEST RATED DRIVER STRATEGY by checking rating of driver stored in driver class and sorting drivers based on rating and returning the sorted list of drivers.

//======================================================
// RIDE CONFIGURATION SERVICE
//======================================================


