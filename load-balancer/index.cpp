
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <limits>
#include <algorithm>

using namespace std;

// ============================================================
// 1. ENUMS AND DATA MODELS
// ============================================================

enum class ServerStatus
{
    HEALTHY,
    UNHEALTHY,
    DRAINING
};

enum class LoadBalancerError
{
    NONE,
    NO_BACKEND_AVAILABLE,
    DISPATCH_FAILED
};

struct Request
{
    string requestId;
    string clientIP;
    string path;
};

struct DispatchResult
{
    bool success = false;
    LoadBalancerError error =
        LoadBalancerError::NO_BACKEND_AVAILABLE;
    string backendId;
    string response;
};

// ============================================================
// 2. BACKEND SERVER
// Owns its status and active-request count.
// tryAcquire() atomically checks health and reserves capacity.
// ============================================================

class BackendServer
{
private:
    string id;
    string address;
    int maxConcurrentRequests;

    mutable mutex mtx;
    ServerStatus status;
    int activeRequests = 0;

public:
    BackendServer(
        string id,
        string address,
        int maxConcurrentRequests =
            numeric_limits<int>::max())
        : id(move(id)),
          address(move(address)),
          maxConcurrentRequests(maxConcurrentRequests),
          status(ServerStatus::HEALTHY)
    {

        if (this->maxConcurrentRequests <= 0)
        {
            throw invalid_argument(
                "Maximum concurrent requests must be positive");
        }
    }

    string getId() const
    {
        return id;
    }

    string getAddress() const
    {
        return address;
    }

    ServerStatus getStatus() const
    {
        lock_guard<mutex> lock(mtx);
        return status;
    }

    int getActiveRequests() const
    {
        lock_guard<mutex> lock(mtx);
        return activeRequests;
    }

    void setStatus(ServerStatus newStatus)
    {
        lock_guard<mutex> lock(mtx);
        status = newStatus;
    }

    bool tryAcquire()
    {
        lock_guard<mutex> lock(mtx);

        if (status != ServerStatus::HEALTHY)
        {
            return false;
        }

        if (activeRequests >= maxConcurrentRequests)
        {
            return false;
        }

        ++activeRequests;
        return true;
    }

    void release()
    {
        lock_guard<mutex> lock(mtx);

        if (activeRequests > 0)
        {
            --activeRequests;
        }
    }
};

// ============================================================
// 3. SERVER REGISTRY
// Responsible only for adding, removing and listing servers.
// ============================================================

class IServerRegistry
{
public:
    virtual ~IServerRegistry() = default;

    virtual void addServer(
        shared_ptr<BackendServer> server) = 0;

    virtual void removeServer(
        const string &serverId) = 0;

    virtual vector<shared_ptr<BackendServer>>
    getServers() const = 0;
};

class ServerRegistry : public IServerRegistry
{
private:
    unordered_map<string, shared_ptr<BackendServer>> servers;
    mutable shared_mutex mtx;

public:
    void addServer(
        shared_ptr<BackendServer> server) override
    {
        if (!server)
        {
            throw invalid_argument("Server cannot be null");
        }

        unique_lock<shared_mutex> lock(mtx);

        string serverId = server->getId();

        if (servers.count(serverId))
        {
            throw invalid_argument(
                "Server already registered: " + serverId);
        }

        servers[serverId] = move(server);
    }

    void removeServer(
        const string &serverId) override
    {
        unique_lock<shared_mutex> lock(mtx);
        servers.erase(serverId);
    }

    vector<shared_ptr<BackendServer>>
    getServers() const override
    {
        shared_lock<shared_mutex> lock(mtx);

        vector<shared_ptr<BackendServer>> result;
        result.reserve(servers.size());

        for (const auto &entry : servers)
        {
            result.push_back(entry.second);
        }

        return result;
    }
};

// ============================================================
// 4. LOAD-BALANCING STRATEGY
// Strategy selects a server; it does not dispatch requests.
// ============================================================

class ILoadBalancingStrategy
{
public:
    virtual ~ILoadBalancingStrategy() = default;

    virtual shared_ptr<BackendServer> selectServer(
        const vector<shared_ptr<BackendServer>> &candidates,
        const Request &request) = 0;
};

// ============================================================
// 5. ROUND ROBIN
// Selects the next candidate in rotation.
// ============================================================

class RoundRobinStrategy : public ILoadBalancingStrategy
{
private:
    mutex mtx;
    size_t nextIndex = 0;

public:
    shared_ptr<BackendServer> selectServer(
        const vector<shared_ptr<BackendServer>> &candidates,
        const Request &request) override
    {
        (void)request;

        if (candidates.empty())
        {
            return nullptr;
        }

        lock_guard<mutex> lock(mtx);

        nextIndex %= candidates.size();

        auto selected = candidates[nextIndex];

        nextIndex = (nextIndex + 1) % candidates.size();

        return selected;
    }
};

// ============================================================
// 6. LEAST CONNECTIONS
// Selects the server with the fewest active requests.
// ============================================================

class LeastConnectionsStrategy
    : public ILoadBalancingStrategy
{

public:
    shared_ptr<BackendServer> selectServer(
        const vector<shared_ptr<BackendServer>> &candidates,
        const Request &request) override
    {
        (void)request;

        if (candidates.empty())
        {
            return nullptr;
        }

        shared_ptr<BackendServer> selected = nullptr;
        int minimumRequests = numeric_limits<int>::max();

        for (const auto &server : candidates)
        {
            int currentRequests = server->getActiveRequests();

            if (currentRequests < minimumRequests)
            {
                minimumRequests = currentRequests;
                selected = server;
            }
        }

        return selected;
    }
};

// ============================================================
// 7. REQUEST DISPATCHER
// Responsible for forwarding a request to a chosen backend.
// This implementation simulates forwarding.
// ============================================================

class IRequestDispatcher
{
public:
    virtual ~IRequestDispatcher() = default;

    virtual DispatchResult dispatch(
        const Request &request,
        const shared_ptr<BackendServer> &server) = 0;
};

class SimulatedRequestDispatcher
    : public IRequestDispatcher
{

public:
    DispatchResult dispatch(
        const Request &request,
        const shared_ptr<BackendServer> &server) override
    {
        if (!server)
        {
            return {
                false,
                LoadBalancerError::DISPATCH_FAILED,
                "",
                "Invalid backend"};
        }

        cout << "Dispatching request "
             << request.requestId
             << " to " << server->getAddress()
             << " for path " << request.path
             << '\n';

        // Real implementation:
        // 1. Send the HTTP request to the backend.
        // 2. Wait for its response or timeout.
        // 3. Return the response and outcome.

        return {
            true,
            LoadBalancerError::NONE,
            server->getId(),
            "200 OK"};
    }
};

// ============================================================
// 8. REQUEST LEASE
// Releases the reservation automatically, including on errors.
// ============================================================

class ServerLease
{
private:
    shared_ptr<BackendServer> server;

public:
    explicit ServerLease(
        shared_ptr<BackendServer> server) : server(move(server)) {}

    ~ServerLease()
    {
        if (server)
        {
            server->release();
        }
    }

    ServerLease(const ServerLease &) = delete;
    ServerLease &operator=(const ServerLease &) = delete;
};

// ============================================================
// 9. LOAD BALANCER
// Coordinates candidate selection, reservation and dispatch.
// ============================================================

class LoadBalancer
{
private:
    shared_ptr<IServerRegistry> registry;
    shared_ptr<ILoadBalancingStrategy> strategy;
    shared_ptr<IRequestDispatcher> dispatcher;

public:
    LoadBalancer(
        shared_ptr<IServerRegistry> registry,
        shared_ptr<ILoadBalancingStrategy> strategy,
        shared_ptr<IRequestDispatcher> dispatcher)
        : registry(move(registry)),
          strategy(move(strategy)),
          dispatcher(move(dispatcher))
    {

        if (!this->registry ||
            !this->strategy ||
            !this->dispatcher)
        {
            throw invalid_argument(
                "LoadBalancer dependencies cannot be null");
        }
    }

    DispatchResult handleRequest(
        const Request &request)
    {
        // Take a snapshot of the registered servers.
        auto servers = registry->getServers();

        // Prevent infinite retries if a selected server
        // becomes unavailable between selection and reservation.
        unordered_set<string> attemptedServers;

        for (size_t attempt = 0; attempt < servers.size(); ++attempt)
        {
            vector<shared_ptr<BackendServer>> candidates;

            for (const auto &server : servers)
            {
                if (attemptedServers.count(server->getId()))
                {
                    continue;
                }

                if (server->getStatus() == ServerStatus::HEALTHY)
                {
                    candidates.push_back(server);
                }
            }

            if (candidates.empty())
            {
                break;
            }

            auto selected =
                strategy->selectServer(candidates, request);

            if (!selected)
            {
                break;
            }

            attemptedServers.insert(selected->getId());

            // Re-check status and capacity atomically.
            // The server may have become unhealthy after selection.
            if (!selected->tryAcquire())
            {
                continue;
            }

            // Reservation is released on every exit from this scope.
            ServerLease lease(selected);

            try
            {
                DispatchResult result =
                    dispatcher->dispatch(request, selected);

                if (result.backendId.empty())
                {
                    result.backendId = selected->getId();
                }

                return result;
            }
            catch (const exception &e)
            {
                return {
                    false,
                    LoadBalancerError::DISPATCH_FAILED,
                    selected->getId(),
                    string("Dispatch failed: ") + e.what()};
            }
            catch (...)
            {
                return {
                    false,
                    LoadBalancerError::DISPATCH_FAILED,
                    selected->getId(),
                    "Unknown dispatch failure"};
            }
        }

        return {
            false,
            LoadBalancerError::NO_BACKEND_AVAILABLE,
            "",
            "No healthy backend with available capacity"};
    }
};

// ============================================================
// 10. DEMO
// ============================================================

int main()
{
    auto registry = make_shared<ServerRegistry>();

    auto server1 = make_shared<BackendServer>(
        "S1", "http://10.0.0.1:8080", 100);

    auto server2 = make_shared<BackendServer>(
        "S2", "http://10.0.0.2:8080", 100);

    auto server3 = make_shared<BackendServer>(
        "S3", "http://10.0.0.3:8080", 100);

    registry->addServer(server1);
    registry->addServer(server2);
    registry->addServer(server3);

    // Simulate an unhealthy server.
    server2->setStatus(ServerStatus::UNHEALTHY);

    // Choose the strategy here:
    shared_ptr<ILoadBalancingStrategy> strategy =
        make_shared<RoundRobinStrategy>();

    // Alternatively:
    // shared_ptr<ILoadBalancingStrategy> strategy =
    //     make_shared<LeastConnectionsStrategy>();

    auto dispatcher =
        make_shared<SimulatedRequestDispatcher>();

    LoadBalancer loadBalancer(
        registry,
        strategy,
        dispatcher);

    vector<Request> requests = {
        {"REQ-1", "192.168.1.10", "/users"},
        {"REQ-2", "192.168.1.11", "/orders"},
        {"REQ-3", "192.168.1.12", "/payments"},
        {"REQ-4", "192.168.1.13", "/products"}};

    for (const auto &request : requests)
    {
        DispatchResult result =
            loadBalancer.handleRequest(request);

        cout << "Request: " << request.requestId
             << " | Success: "
             << (result.success ? "YES" : "NO")
             << " | Backend: "
             << (result.backendId.empty()
                     ? "NONE"
                     : result.backendId)
             << " | Response: " << result.response
             << "\n\n";
    }

    cout << "Active requests after dispatch:\n";

    cout << server1->getId() << ": "
         << server1->getActiveRequests() << '\n';

    cout << server2->getId() << ": "
         << server2->getActiveRequests() << '\n';

    cout << server3->getId() << ": "
         << server3->getActiveRequests() << '\n';

    // Runtime removal:
    registry->removeServer("S3");

    return 0;
}
