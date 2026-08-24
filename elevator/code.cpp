#include <iostream>
#include <vector>
#include <set>
#include <memory>
#include <mutex>
#include <algorithm>
#include <climits>

using namespace std;

// 1. Enums
// 2. Request classes
// 3. Scheduling Strategy
// 4. LOOK Scheduling Strategy  (others are SCAN AND FIFO)
// 5. Elevator
// 6. Assignment Strategy
// 7. Nearest Elevator Strategy
// 8. ElevatorController
// 9. ElevatorSystem (Facade)
// 10. main()

//======================================================
// ENUMS
//======================================================

enum class Direction
{
    UP,
    DOWN,
    IDLE
};

enum class ElevatorState
{
    IDLE,
    MOVING
};

enum class DoorState
{
    OPEN,
    CLOSED
};

//======================================================
// REQUESTS
//======================================================

class Request
{
protected:
    int floor;

public:
    Request(int floor)
        : floor(floor)
    {
    }

    virtual ~Request() = default;

    int getFloor() const
    {
        return floor;
    }
};

//======================================================
// EXTERNAL REQUEST
//======================================================

class ExternalRequest : public Request
{
private:
    Direction direction;

public:
    ExternalRequest(int floor, Direction direction)
        : Request(floor),
          direction(direction)
    {
    }

    Direction getDirection() const
    {
        return direction;
    }
};

//======================================================
// INTERNAL REQUEST
//======================================================

class InternalRequest : public Request
{
public:
    InternalRequest(int destinationFloor)
        : Request(destinationFloor)
    {
    }
};

//======================================================
// SCHEDULING STRATEGY
//======================================================

class ElevatorSchedulingStrategy
{
public:
    virtual ~ElevatorSchedulingStrategy() = default;

    virtual void addRequest(const Request &request) = 0;

    virtual bool hasPendingRequests() const = 0;

    virtual int getNextStop(int currentFloor, Direction &direction) = 0;

    virtual void markStopCompleted(int floor) = 0;
};

//======================================================
// LOOK SCHEDULING STRATEGY
//======================================================

class LookSchedulingStrategy : public ElevatorSchedulingStrategy
{
private:
    set<int> upHallRequests;
    set<int> downHallRequests;
    set<int> cabinRequests;

public:
    void addRequest(const Request &request) override
    {
        if (const auto *externalRequest = dynamic_cast<const ExternalRequest *>(&request))
        {
            if (externalRequest->getDirection() == Direction::UP)
            {
                upHallRequests.insert(externalRequest->getFloor());
            }
            else
            {
                downHallRequests.insert(externalRequest->getFloor());
            }
        }
        else if (const auto *internalRequest = dynamic_cast<const InternalRequest *>(&request))
        {
            cabinRequests.insert(internalRequest->getFloor());
        }
    }

    bool hasPendingRequests() const override
    {
        return !upHallRequests.empty() ||
               !downHallRequests.empty() ||
               !cabinRequests.empty();
    }

    int getNextStop(int currentFloor, Direction &direction) override
    {
        // Elevator is idle
        if (direction == Direction::IDLE)
        {
            if (!upHallRequests.empty())
            {
                direction = Direction::UP;
            }
            else if (!downHallRequests.empty())
            {
                direction = Direction::DOWN;
            }
            else if (!cabinRequests.empty())
            {
                if (*cabinRequests.begin() > currentFloor)
                {
                    direction = Direction::UP;
                }
                else
                {
                    direction = Direction::DOWN;
                }
            }
            else
            {
                return currentFloor;
            }
        }

        if (direction == Direction::UP)
        {
            int candidate = INT_MAX;

            auto it = upHallRequests.lower_bound(currentFloor);
            if (it != upHallRequests.end())
            {
                candidate = min(candidate, *it);
            }

            it = cabinRequests.lower_bound(currentFloor);
            if (it != cabinRequests.end())
            {
                candidate = min(candidate, *it);
            }

            if (candidate != INT_MAX)
            {
                return candidate;
            }

            direction = Direction::DOWN;
            return getNextStop(currentFloor, direction);
        }

        int candidate = INT_MIN;

        auto it = downHallRequests.lower_bound(currentFloor);
        if (it != downHallRequests.begin())
        {
            --it;
            candidate = max(candidate, *it);
        }

        it = cabinRequests.lower_bound(currentFloor);
        if (it != cabinRequests.begin())
        {
            --it;
            candidate = max(candidate, *it);
        }

        if (candidate != INT_MIN)
        {
            return candidate;
        }

        direction = Direction::UP;
        return getNextStop(currentFloor, direction);
    }

    void markStopCompleted(int floor) override
    {
        upHallRequests.erase(floor);
        downHallRequests.erase(floor);
        cabinRequests.erase(floor);
    }
};

//======================================================
// ELEVATOR
//======================================================

class Elevator
{
private:
    int elevatorId;
    int currentFloor;

    Direction direction;
    ElevatorState state;
    DoorState doorState;

    shared_ptr<ElevatorSchedulingStrategy> schedulingStrategy;

    mutable mutex elevatorMutex;

public:
    Elevator(int elevatorId, shared_ptr<ElevatorSchedulingStrategy> schedulingStrategy)
        : elevatorId(elevatorId),
          currentFloor(0),
          direction(Direction::IDLE),
          state(ElevatorState::IDLE),
          doorState(DoorState::CLOSED),
          schedulingStrategy(schedulingStrategy)
    {
    }

    int getElevatorId() const
    {
        return elevatorId;
    }

    int getCurrentFloor() const
    {
        return currentFloor;
    }

    Direction getDirection() const
    {
        return direction;
    }

    ElevatorState getState() const
    {
        return state;
    }

    bool isIdle() const
    {
        return state == ElevatorState::IDLE;
    }

    //--------------------------------------------------
    // Accept Request
    //--------------------------------------------------

    void addRequest(const Request &request)
    {
        lock_guard<mutex> lock(elevatorMutex);

        schedulingStrategy->addRequest(request);

        if (state == ElevatorState::IDLE)
        {
            state = ElevatorState::MOVING;
        }
    }

    //--------------------------------------------------
    // Move elevator until all requests are completed
    //--------------------------------------------------

    void processRequests()
    {
        lock_guard<mutex> lock(elevatorMutex);

        while (schedulingStrategy->hasPendingRequests())
        {
            int nextStop = schedulingStrategy->getNextStop(currentFloor, direction);

            moveToFloor(nextStop);

            openDoor();

            schedulingStrategy->markStopCompleted(nextStop);

            closeDoor();
        }

        state = ElevatorState::IDLE;
        direction = Direction::IDLE;
    }

private:
    //--------------------------------------------------
    // Move one floor at a time
    //--------------------------------------------------

    void moveToFloor(int destination)
    {
        while (currentFloor < destination)
        {
            currentFloor++;

            cout << "Elevator "
                 << elevatorId
                 << " -> Floor "
                 << currentFloor
                 << endl;
        }

        while (currentFloor > destination)
        {
            currentFloor--;

            cout << "Elevator "
                 << elevatorId
                 << " -> Floor "
                 << currentFloor
                 << endl;
        }
    }

    void openDoor()
    {
        doorState = DoorState::OPEN;

        cout << "Elevator "
             << elevatorId
             << " Door Open at Floor "
             << currentFloor
             << endl;
    }

    void closeDoor()
    {
        doorState = DoorState::CLOSED;

        cout << "Elevator "
             << elevatorId
             << " Door Closed"
             << endl;
    }
};

//======================================================
// ELEVATOR ASSIGNMENT STRATEGY
//======================================================

class ElevatorAssignmentStrategy
{
public:
    virtual ~ElevatorAssignmentStrategy() = default;

    virtual shared_ptr<Elevator> assignElevator(
        const ExternalRequest &request,
        const vector<shared_ptr<Elevator>> &elevators) = 0;
};

//======================================================
// NEAREST ELEVATOR STRATEGY
//======================================================

class NearestElevatorStrategy : public ElevatorAssignmentStrategy
{
public:
    shared_ptr<Elevator> assignElevator(
        const ExternalRequest &request,
        const vector<shared_ptr<Elevator>> &elevators) override
    {
        shared_ptr<Elevator> bestElevator = nullptr;
        int minimumDistance = INT_MAX;

        //--------------------------------------------------
        // 1. Prefer Idle Elevator
        //--------------------------------------------------

        for (auto &elevator : elevators)
        {
            if (!elevator->isIdle())
            {
                continue;
            }

            int distance = abs(elevator->getCurrentFloor() - request.getFloor());

            if (distance < minimumDistance)
            {
                minimumDistance = distance;
                bestElevator = elevator;
            }
        }

        if (bestElevator != nullptr)
        {
            return bestElevator;
        }

        //--------------------------------------------------
        // 2. Elevator moving in same direction
        //--------------------------------------------------

        minimumDistance = INT_MAX;

        for (auto &elevator : elevators)
        {
            if (elevator->getDirection() != request.getDirection())
            {
                continue;
            }

            bool canPickup = false;

            if (request.getDirection() == Direction::UP)
            {
                canPickup = elevator->getCurrentFloor() <= request.getFloor();
            }
            else
            {
                canPickup = elevator->getCurrentFloor() >= request.getFloor();
            }

            if (!canPickup)
            {
                continue;
            }

            int distance = abs(elevator->getCurrentFloor() - request.getFloor());

            if (distance < minimumDistance)
            {
                minimumDistance = distance;
                bestElevator = elevator;
            }
        }

        if (bestElevator != nullptr)
        {
            return bestElevator;
        }

        //--------------------------------------------------
        // 3. Fallback
        //--------------------------------------------------

        minimumDistance = INT_MAX;

        for (auto &elevator : elevators)
        {
            int distance = abs(elevator->getCurrentFloor() - request.getFloor());

            if (distance < minimumDistance)
            {
                minimumDistance = distance;
                bestElevator = elevator;
            }
        }

        return bestElevator;
    }
};

// Instead of:
// processRequests();
// each elevator should run independently:
// while (true)
// {
//     step();
// }

//======================================================
// ELEVATOR CONTROLLER
//======================================================

class ElevatorController
{
private:
    vector<shared_ptr<Elevator>> elevators;
    shared_ptr<ElevatorAssignmentStrategy> assignmentStrategy;

public:
    ElevatorController(shared_ptr<ElevatorAssignmentStrategy> assignmentStrategy)
        : assignmentStrategy(assignmentStrategy)
    {
    }

    //--------------------------------------------------
    // Register Elevator
    //--------------------------------------------------

    void addElevator(shared_ptr<Elevator> elevator)
    {
        elevators.push_back(elevator);
    }

    //--------------------------------------------------
    // Handle Hall Request
    //--------------------------------------------------

    int requestElevator(int floor, Direction direction)
    {
        ExternalRequest request(floor, direction);

        auto elevator = assignmentStrategy->assignElevator(request, elevators);

        if (elevator == nullptr)
        {
            throw runtime_error("No Elevator Available");
        }

        cout << "Assigned Elevator " << elevator->getElevatorId() << endl;

        elevator->addRequest(request);

        return elevator->getElevatorId();
    }

    //--------------------------------------------------
    // Handle Cabin Request
    //--------------------------------------------------

    void selectFloor(int elevatorId, int destinationFloor)
    {
        auto elevator = getElevator(elevatorId);

        if (elevator == nullptr)
        {
            throw runtime_error("Invalid Elevator");
        }

        InternalRequest request(destinationFloor);

        elevator->addRequest(request);
    }

    //--------------------------------------------------
    // Simulate Elevator Movement
    //--------------------------------------------------

    void processAllElevators()
    {
        for (auto &elevator : elevators)
        {
            elevator->processRequests();
        }
    }
    // In production, each elevator would own its own worker thread (or event loop). The controller would only assign requests. Here, processAllElevators() 
    // is just a simulation helper to keep the demo simple.

private:
    shared_ptr<Elevator> getElevator(int elevatorId)
    {
        for (auto &elevator : elevators)
        {
            if (elevator->getElevatorId() == elevatorId)
            {
                return elevator;
            }
        }

        return nullptr;
    }
};

//======================================================
// ELEVATOR SYSTEM (FACADE)
//======================================================

class ElevatorSystem
{
private:
    ElevatorController controller;

public:
    ElevatorSystem(shared_ptr<ElevatorAssignmentStrategy> assignmentStrategy)
        : controller(assignmentStrategy)
    {
    }

    void addElevator(shared_ptr<Elevator> elevator)
    {
        controller.addElevator(elevator);
    }

    int requestElevator(int floor, Direction direction)
    {
        return controller.requestElevator(floor, direction);
    }

    void selectFloor(int elevatorId, int destinationFloor)
    {
        controller.selectFloor(elevatorId, destinationFloor);
    }

    // Simulation helper
    void run()
    {
        controller.processAllElevators();
    }
};

//======================================================
// MAIN
//======================================================

int main()
{
    auto assignmentStrategy = make_shared<NearestElevatorStrategy>();
    ElevatorSystem elevatorSystem(assignmentStrategy);

    //--------------------------------------------------
    // Create Elevators
    //--------------------------------------------------

    auto elevator1 = make_shared<Elevator>(1, make_shared<LookSchedulingStrategy>());
    auto elevator2 = make_shared<Elevator>(2, make_shared<LookSchedulingStrategy>());
    auto elevator3 = make_shared<Elevator>(3, make_shared<LookSchedulingStrategy>());

    elevatorSystem.addElevator(elevator1);
    elevatorSystem.addElevator(elevator2);
    elevatorSystem.addElevator(elevator3);

    //--------------------------------------------------
    // Hall Requests
    //--------------------------------------------------

    int elevatorId = elevatorSystem.requestElevator(5, Direction::UP);
    elevatorSystem.requestElevator(9, Direction::DOWN);
    elevatorSystem.requestElevator(2, Direction::UP);

    //--------------------------------------------------
    // Passenger enters elevator
    //--------------------------------------------------

    elevatorSystem.selectFloor(elevatorId, 12);

    //--------------------------------------------------
    // Start Simulation
    //--------------------------------------------------

    elevatorSystem.run();

    return 0;
}