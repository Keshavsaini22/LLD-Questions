// why driver is geting saved in ride request and also in ride? why duplicate data is being stored?
// Use normal values for small types, const T& for large objects, unique_ptr for ownership, and shared_ptr only when ownership truly needs to be shared
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
    Location(double latitude = 0, double longitude = 0)
        : latitude(latitude), longitude(longitude) {}

    double getLatitude() const { return latitude; }
    double getLongitude() const { return longitude; }

    double distanceTo(const Location &other) const
    {
        // const in the starting of the function means that this function will not modify any member variables of the class.
        // It is a promise that the function will not change the state of the object.
        double dx = latitude - other.latitude;
        double dy = longitude - other.longitude;
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
    Vehicle(string vehicleNumber, RideType rideType)
        : vehicleNumber(vehicleNumber), rideType(rideType) {}

    RideType getRideType() const { return rideType; }
    string getVehicleNumber() const { return vehicleNumber; }
};

//======================================================
// RIDER
//======================================================

class Rider
{
    int riderId;
    string name;

public:
    Rider(int riderId, string name) : riderId(riderId), name(name) {}

    int getId() const { return riderId; }
    string getName() const { return name; }
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
    Driver(int driverId, string name, Location currentLocation, Vehicle vehicle)
        : driverId(driverId), name(name), currentLocation(currentLocation), vehicle(vehicle), available(true) {}

    int getId() const { return driverId; }
    string getName() const { return name; }
    bool isAvailable() const { return available; }
    void setAvailability(bool value) { available = value; }
    Location getLocation() const { return currentLocation; }
    Vehicle getVehicle() const { return vehicle; }
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
    RideQuote(RideType rideType, double estimatedFare, int etaMinutes)
        : rideType(rideType), estimatedFare(estimatedFare), etaMinutes(etaMinutes) {}

    RideType getRideType() const { return rideType; }
    double getEstimatedFare() const { return estimatedFare; }
    int getEtaMinutes() const { return etaMinutes; }
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
    RideRequest(int requestId, shared_ptr<Rider> rider, const Location &source, const Location &destination, const RideQuote &selectedQuote)
        : requestId(requestId), rider(rider), source(source), destination(destination), selectedQuote(selectedQuote), status(RequestStatus::PENDING) {}

    int getRequestId() const { return requestId; }
    shared_ptr<Rider> getRider() const { return rider; }
    const Location &getSource() const { return source; }
    const Location &getDestination() const { return destination; }
    RideType getRideType() const { return selectedQuote.getRideType(); }
    double getEstimatedFare() const { return selectedQuote.getEstimatedFare(); }
    RequestStatus getStatus() const { return status; }

    bool acceptDriver(shared_ptr<Driver> driver)
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
    Ride(int rideId, shared_ptr<Rider> rider, shared_ptr<Driver> driver, const Location &source, const Location &destination, RideType rideType, double fare)
        : rideId(rideId), rider(rider), driver(driver), source(source), destination(destination), rideType(rideType), fare(fare), status(RideStatus::DRIVER_ASSIGNED) {}

    int getRideId() const { return rideId; }
    RideStatus getStatus() const { return status; }
    void startRide() { status = RideStatus::ONGOING; }
    void completeRide() { status = RideStatus::COMPLETED; }
    void cancelRide() { status = RideStatus::CANCELLED; }
    shared_ptr<Driver> getDriver() const { return driver; }
    shared_ptr<Rider> getRider() const { return rider; }
    double getFare() const { return fare; }
};

//======================================================
// FARE STRATEGY
//======================================================

class FareStrategy
{
public:
    virtual ~FareStrategy() = default;
    virtual vector<RideQuote> calculateFare(const Location &source, const Location &destination) = 0;
};

//======================================================
// NORMAL FARE STRATEGY
//======================================================

class NormalFareStrategy : public FareStrategy
{
public:
    vector<RideQuote> calculateFare(const Location &source, const Location &destination) override
    {
        double distance = source.distanceTo(destination);
        vector<RideQuote> quotes;
        quotes.emplace_back(RideType::GO, 50 + distance * 10, 3);
        quotes.emplace_back(RideType::XL, 100 + distance * 15, 5);
        quotes.emplace_back(RideType::PREMIER, 200 + distance * 25, 7);
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
    SurgeFareStrategy(double surgeMultiplier) : surgeMultiplier(surgeMultiplier) {}

    vector<RideQuote> calculateFare(const Location &source, const Location &destination) override
    {
        double distance = source.distanceTo(destination);
        vector<RideQuote> quotes;
        quotes.emplace_back(RideType::GO, (50 + distance * 10) * surgeMultiplier, 3);
        quotes.emplace_back(RideType::XL, (100 + distance * 15) * surgeMultiplier, 5);
        quotes.emplace_back(RideType::PREMIER, (200 + distance * 25) * surgeMultiplier, 7);
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
    virtual vector<shared_ptr<Driver>> findDrivers(const RideRequest &request, const vector<shared_ptr<Driver>> &drivers) = 0;
};

//======================================================
// NEAREST DRIVER STRATEGY
//======================================================

class NearestDriverStrategy : public DriverMatchingStrategy
{
public:
    vector<shared_ptr<Driver>> findDrivers(const RideRequest &request, const vector<shared_ptr<Driver>> &drivers) override
    {
        vector<shared_ptr<Driver>> candidates;
        for (auto &driver : drivers)
        {
            if (driver->getVehicle().getRideType() != request.getRideType())
            {
                continue;
            }
            candidates.push_back(driver);
        }
        sort(candidates.begin(), candidates.end(), [&](auto &d1, auto &d2)
             { return d1->getLocation().distanceTo(request.getSource()) < d2->getLocation().distanceTo(request.getSource()); });
        return candidates;
    }
};

class StrategySelector
{
public:
    virtual shared_ptr<FareStrategy> selectFareStrategy() = 0;
    virtual shared_ptr<DriverMatchingStrategy> selectDriverMatchingStrategy(const RideRequest &request) = 0;
    virtual ~StrategySelector() = default;
};

class DefaultStrategySelector : public StrategySelector
{
public:
    shared_ptr<FareStrategy> selectFareStrategy() override
    {
        return make_shared<NormalFareStrategy>();
    }

    shared_ptr<DriverMatchingStrategy> selectDriverMatchingStrategy(const RideRequest &request) override
    {
        return make_shared<NearestDriverStrategy>();
    }
};

class DriverManager
{
private:
    unordered_map<int, shared_ptr<Driver>> drivers;

public:
    void registerDriver(shared_ptr<Driver> driver)
    {
        drivers[driver->getId()] = driver;
    }

    shared_ptr<Driver> getDriver(int id)
    {
        return drivers[id];
    }

    vector<shared_ptr<Driver>> getAvailableDrivers()
    {
        vector<shared_ptr<Driver>> result;
        for (auto &[id, driver] : drivers)
        {
            if (driver->isAvailable())
            {
                result.push_back(driver);
            }
        }
        return result;
    }
};

class DriverMatchingService
{
private:
    DriverManager &driverManager;
    StrategySelector &strategySelector;

public:
    DriverMatchingService(DriverManager &driverManager, StrategySelector &strategySelector)
        : driverManager(driverManager), strategySelector(strategySelector) {}

    vector<shared_ptr<Driver>> findCandidateDrivers(const RideRequest &request)
    {
        auto strategy = strategySelector.selectDriverMatchingStrategy(request);
        auto drivers = driverManager.getAvailableDrivers();
        return strategy->findDrivers(request, drivers);
    }
};

class Notification
{
public:
    virtual ~Notification() = default;
    virtual void send(const string &recipient, const string &message) = 0;
};

class EmailNotification : public Notification
{
public:
    void send(const string &recipient, const string &message) override
    {
        cout << "Email to " << recipient << " : " << message << endl;
    }
};

class SMSNotification : public Notification
{
public:
    void send(const string &recipient, const string &message) override
    {
        cout << "SMS to " << recipient << " : " << message << endl;
    }
};

class NotificationService
{
    vector<shared_ptr<Notification>> channels;

public:
    void addObserver(shared_ptr<Notification> observer)
    {
        channels.push_back(observer);
    }

    void notifyRider(shared_ptr<Rider> rider, const string &message)
    {
        for (auto &channel : channels)
        {
            channel->send(rider->getName(), message);
        }
    }

    void notifyDriver(shared_ptr<Driver> driver, const string &message)
    {
        for (auto &channel : channels)
        {
            channel->send(driver->getName(), message);
        }
    }
};

class RideRequestManager
{
    unordered_map<int, shared_ptr<RideRequest>> requests;
    atomic<int> nextRequestId{1};

public:
    shared_ptr<RideRequest> createRequest(shared_ptr<Rider> rider, const Location &source, const Location &destination, const RideQuote &selectedQuote)
    {
        int requestId = nextRequestId++;
        auto request = make_shared<RideRequest>(requestId, rider, source, destination, selectedQuote);
        requests[requestId] = request;
        return request;
    }

    shared_ptr<RideRequest> getRequest(int requestId)
    {
        auto it = requests.find(requestId);
        if (it == requests.end())
        {
            return nullptr;
        }
        return it->second;
    }

    void cancelRequest(int requestId)
    {
        auto request = getRequest(requestId);
        if (request)
        {
            request->cancelRequest();
        }
    }
};

class RideManager
{
    unordered_map<int, shared_ptr<Ride>> rides;
    atomic<int> nextRideId{1};

public:
    shared_ptr<Ride> createRide(shared_ptr<RideRequest> request)
    {
        int rideId = nextRideId++;
        auto ride = make_shared<Ride>(rideId, request->getRider(), request->getAssignedDriver(), request->getSource(), request->getDestination(), request->getRideType(), request->getEstimatedFare());
        rides[rideId] = ride;
        return ride;
    }

    shared_ptr<Ride> getRide(int rideId)
    {
        auto it = rides.find(rideId);
        if (it == rides.end())
        {
            return nullptr;
        }
        return it->second;
    }

    void startRide(int rideId)
    {
        auto ride = getRide(rideId);
        if (ride)
        {
            ride->startRide();
        }
    }

    void completeRide(int rideId)
    {
        auto ride = getRide(rideId);
        if (!ride)
            return;
        ride->completeRide();
        ride->getDriver()->setAvailability(true);
    }

    void cancelRide(int rideId)
    {
        auto ride = getRide(rideId);
        if (!ride)
            return;
        ride->cancelRide();
        ride->getDriver()->setAvailability(true);
    }
};

class DriverResponseService
{
    RideRequestManager &requestManager;
    RideManager &rideManager;
    NotificationService &notificationService;

public:
    DriverResponseService(RideRequestManager &requestManager, RideManager &rideManager, NotificationService &notificationService)
        : requestManager(requestManager), rideManager(rideManager), notificationService(notificationService) {}

    shared_ptr<Ride> acceptRideRequest(int requestId, shared_ptr<Driver> driver)
    {
        auto request = requestManager.getRequest(requestId);
        if (!request)
        {
            return nullptr;
        }
        bool success = request->acceptDriver(driver);
        if (!success)
        {
            return nullptr;
        }
        driver->setAvailability(false);
        auto ride = rideManager.createRide(request);
        notificationService.notifyRider(request->getRider(), "Driver assigned");
        return ride;
    }

    void rejectRideRequest(int requestId, shared_ptr<Driver> driver)
    {
        cout << "Driver " << driver->getId() << " rejected request " << requestId << endl;
    }
};

class UberService
{
private:
    DriverManager &driverManager;
    RideRequestManager &requestManager;
    RideManager &rideManager;
    DriverMatchingService &matchingService;
    DriverResponseService &responseService;
    NotificationService &notificationService;
    StrategySelector &strategySelector;

public:
    UberService(DriverManager &driverManager, RideRequestManager &requestManager, RideManager &rideManager, DriverMatchingService &matchingService, DriverResponseService &responseService, NotificationService &notificationService, StrategySelector &strategySelector)
        : driverManager(driverManager), requestManager(requestManager), rideManager(rideManager), matchingService(matchingService), responseService(responseService), notificationService(notificationService), strategySelector(strategySelector) {}

    // Step 1
    // Rider asks for fare estimation
    vector<RideQuote> getRideQuote(const Location &source, const Location &destination)
    {
        auto fareStrategy = strategySelector.selectFareStrategy(); // I should pass something to this function and in return i will get strategy accordingly
        return fareStrategy->calculateFare(source, destination);
    }

    // Step 2
    // User selects quote and requests ride
    shared_ptr<RideRequest> requestRide(shared_ptr<Rider> rider, const Location &source, const Location &destination, const RideQuote &selectedQuote)
    {
        auto request = requestManager.createRequest(rider, source, destination, selectedQuote);
        auto candidateDrivers = matchingService.findCandidateDrivers(*request);
        cout << "Found " << candidateDrivers.size() << " candidate drivers\n";
        for (auto &driver : candidateDrivers)
        {
            notificationService.notifyDriver(driver, "Ride is requested");
        }
        return request;
    }

    // Driver accepts request
    shared_ptr<Ride> acceptRide(int requestId, shared_ptr<Driver> driver)
    {
        return responseService.acceptRideRequest(requestId, driver);
    }

    // Driver rejects request
    void rejectRide(int requestId, shared_ptr<Driver> driver)
    {
        responseService.rejectRideRequest(requestId, driver);
    }

    void cancelRideRequest(int requestId)
    {
        requestManager.cancelRequest(requestId);
    }

    void startRide(int rideId)
    {
        auto ride = rideManager.getRide(rideId);
        if (!ride)
            return;
        rideManager.startRide(rideId);
        notificationService.notifyRider(ride->getRider(), "Ride Started");
        notificationService.notifyDriver(ride->getDriver(), "Ride Started");
    }

    void completeRide(int rideId)
    {
        auto ride = rideManager.getRide(rideId);
        if (!ride)
            return;
        rideManager.completeRide(rideId);
        notificationService.notifyRider(ride->getRider(), "Ride Completed");
        notificationService.notifyDriver(ride->getDriver(), "Ride Completed");
    }

    void cancelRide(int rideId)
    {
        auto ride = rideManager.getRide(rideId);
        if (!ride)
            return;
        rideManager.cancelRide(rideId);
        notificationService.notifyRider(ride->getRider(), "Ride Cancelled");
        notificationService.notifyDriver(ride->getDriver(), "Ride Cancelled");
    }
};

int main()
{
    auto rider = make_shared<Rider>(1, "Rahul");
    auto driver1 = make_shared<Driver>(101, "Amit", Location(11, 11), Vehicle("DL01AB1234", RideType::GO));
    auto driver2 = make_shared<Driver>(102, "Rohit", Location(12, 12), Vehicle("DL01AB5678", RideType::GO));

    DriverManager driverManager;
    driverManager.registerDriver(driver1);
    driverManager.registerDriver(driver2);

    DefaultStrategySelector strategySelector;
    RideRequestManager requestManager;
    RideManager rideManager;
    NotificationService notificationService;

    auto emailObserver = make_shared<EmailNotification>();
    auto smsObserver = make_shared<SMSNotification>();

    notificationService.addObserver(emailObserver);
    notificationService.addObserver(smsObserver);

    DriverMatchingService matchingService(driverManager, strategySelector);
    DriverResponseService responseService(requestManager, rideManager, notificationService);
    UberService uberService(driverManager, requestManager, rideManager, matchingService, responseService, notificationService, strategySelector);

    cout << "====== FARE ESTIMATION ======\n";
    auto quotes = uberService.getRideQuote(Location(10, 10), Location(20, 20));

    RideQuote selectedQuote(RideType::GO, 0, 0);
    bool quoteFound = false;

    cout << "Available Quotes:\n";
    for (const auto &q : quotes)
    {
        string typeStr = "";
        if (q.getRideType() == RideType::GO)
            typeStr = "GO";
        else if (q.getRideType() == RideType::XL)
            typeStr = "XL";
        else if (q.getRideType() == RideType::PREMIER)
            typeStr = "PREMIER";

        cout << "Ride Type: " << typeStr
             << " | Estimated Fare: " << q.getEstimatedFare()
             << " | ETA: " << q.getEtaMinutes() << " mins\n";

        if (q.getRideType() == RideType::GO)
        {
            selectedQuote = q;
            quoteFound = true;
        }
    }

    if (!quoteFound)
    {
        throw runtime_error("Selected quote type not found");
    }

    cout << "\n====== REQUEST RIDE ======\n";
    auto request = uberService.requestRide(rider, Location(10, 10), Location(20, 20), selectedQuote);
    cout << "Request Id : " << request->getRequestId() << endl;

    cout << "\n====== DRIVER ACCEPTS ======\n";
    auto ride = uberService.acceptRide(request->getRequestId(), driver1);
    if (ride)
    {
        cout << "Ride Created : " << ride->getRideId() << endl;
    }

    cout << "\n====== START RIDE ======\n";
    uberService.startRide(ride->getRideId());

    cout << "\n====== COMPLETE RIDE ======\n";
    uberService.completeRide(ride->getRideId());

    return 0;
}