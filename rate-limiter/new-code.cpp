
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace chrono;

// ============================================================
// 1. MODELS
// ============================================================

enum class RateLimitAlgorithm
{
    TOKEN_BUCKET,
    FIXED_WINDOW
};

struct RateLimitPolicy
{
    RateLimitAlgorithm algorithm = RateLimitAlgorithm::TOKEN_BUCKET;

    // Fixed Window configuration
    int maxRequests = 10;
    seconds windowSize{60};

    // Token Bucket configuration
    double bucketCapacity = 10.0;
    double refillTokensPerSecond = 1.0;
};

struct RateLimitDecision
{
    bool allowed = false;
    int remaining = 0;
    milliseconds retryAfter{0};
    string reason;
};

// Use a monotonic clock for measuring elapsed time.
using Clock = steady_clock;
using TimePoint = Clock::time_point;

// ============================================================
// 2. STRATEGY INTERFACE
// ============================================================

class IRateLimitStrategy
{
public:
    virtual ~IRateLimitStrategy() = default;

    virtual RateLimitDecision allowRequest(
        const string &clientKey,
        const RateLimitPolicy &policy,
        TimePoint now) = 0;
};

// ============================================================
// 3. TOKEN BUCKET STRATEGY
//
// Each client has a bucket.
// Tokens refill continuously at a configured rate.
// One token is consumed for each allowed request.
//
// A single mutex makes the state update atomic across threads.
// ============================================================

class TokenBucketStrategy : public IRateLimitStrategy
{
private:
    struct Bucket
    {
        double tokens = 0.0;
        TimePoint lastRefill{};
        bool initialized = false;
    };

    unordered_map<string, Bucket> buckets;
    mutex mtx;

public:
    RateLimitDecision allowRequest(
        const string &clientKey,
        const RateLimitPolicy &policy,
        TimePoint now) override
    {
        if (policy.bucketCapacity < 1.0 ||
            policy.refillTokensPerSecond <= 0.0)
        {
            return {false, 0, milliseconds{0},
                    "Invalid token bucket policy"};
        }

        lock_guard<mutex> lock(mtx);

        Bucket &bucket = buckets[clientKey];

        if (!bucket.initialized)
        {
            // Start with a full bucket.
            bucket.tokens = policy.bucketCapacity;
            bucket.lastRefill = now;
            bucket.initialized = true;
        }

        // Refill based on elapsed monotonic time.
        double elapsed =
            duration<double>(now - bucket.lastRefill).count();

        if (elapsed > 0.0)
        {
            bucket.tokens = min(
                policy.bucketCapacity,
                bucket.tokens +
                    elapsed * policy.refillTokensPerSecond);

            bucket.lastRefill = now;
        }

        if (bucket.tokens >= 1.0)
        {
            bucket.tokens -= 1.0;

            return {
                true,
                static_cast<int>(floor(bucket.tokens)),
                milliseconds{0},
                "Request allowed"};
        }

        // Time until at least one token is available.
        double secondsToWait =
            (1.0 - bucket.tokens) /
            policy.refillTokensPerSecond;

        auto retryAfter = milliseconds{
            static_cast<long long>(
                ceil(secondsToWait * 1000.0))};

        return {
            false,
            0,
            retryAfter,
            "Token bucket limit exceeded"};
    }
};

// ============================================================
// 4. FIXED WINDOW STRATEGY
//
// Each client gets a counter for the current time window.
// The counter resets when the window expires.
// ============================================================

class FixedWindowStrategy : public IRateLimitStrategy
{
private:
    struct Window
    {
        TimePoint start{};
        int requestCount = 0;
        bool initialized = false;
    };

    unordered_map<string, Window> windows;
    mutex mtx;

public:
    RateLimitDecision allowRequest(
        const string &clientKey,
        const RateLimitPolicy &policy,
        TimePoint now) override
    {
        if (policy.maxRequests <= 0 ||
            policy.windowSize <= seconds::zero())
        {
            return {false, 0, milliseconds{0},
                    "Invalid fixed window policy"};
        }

        lock_guard<mutex> lock(mtx);

        Window &window = windows[clientKey];

        if (!window.initialized ||
            now - window.start >= policy.windowSize)
        {
            window.start = now;
            window.requestCount = 0;
            window.initialized = true;
        }

        if (window.requestCount < policy.maxRequests)
        {
            ++window.requestCount;

            return {
                true,
                policy.maxRequests - window.requestCount,
                milliseconds{0},
                "Request allowed"};
        }

        auto elapsed = now - window.start;
        auto remainingWindow =
            duration_cast<milliseconds>(policy.windowSize - elapsed);

        return {
            false,
            0,
            max(milliseconds{0}, remainingWindow),
            "Fixed window limit exceeded"};
    }
};

// ============================================================
// 5. RATE LIMITER FACADE
//
// Stores per-client policies and delegates to the chosen strategy.
// ============================================================

class RateLimiter
{
private:
    unordered_map<string, RateLimitPolicy> policies;
    mutable shared_mutex policyMutex;

    unordered_map<
        RateLimitAlgorithm,
        shared_ptr<IRateLimitStrategy>>
        strategies;

public:
    RateLimiter()
    {
        strategies[RateLimitAlgorithm::TOKEN_BUCKET] =
            make_shared<TokenBucketStrategy>();

        strategies[RateLimitAlgorithm::FIXED_WINDOW] =
            make_shared<FixedWindowStrategy>();
    }

    void setPolicy(
        const string &clientKey,
        const RateLimitPolicy &policy)
    {
        // Validate before publishing the policy.
        if (policy.algorithm == RateLimitAlgorithm::TOKEN_BUCKET)
        {
            if (policy.bucketCapacity < 1.0 ||
                policy.refillTokensPerSecond <= 0.0)
            {
                throw invalid_argument(
                    "Invalid token bucket policy");
            }
        }
        else
        {
            if (policy.maxRequests <= 0 ||
                policy.windowSize <= seconds::zero())
            {
                throw invalid_argument(
                    "Invalid fixed window policy");
            }
        }

        unique_lock<shared_mutex> lock(policyMutex);
        policies[clientKey] = policy;
    }

    RateLimitDecision allowRequest(
        const string &clientKey)
    {
        RateLimitPolicy policy;

        {
            shared_lock<shared_mutex> lock(policyMutex);

            auto it = policies.find(clientKey);

            if (it == policies.end())
            {
                return {
                    false, 0, milliseconds{0},
                    "No rate limit policy configured"};
            }

            policy = it->second;
        }

        auto it = strategies.find(policy.algorithm);

        if (it == strategies.end())
        {
            return {
                false, 0, milliseconds{0},
                "Unsupported rate limit algorithm"};
        }

        return it->second->allowRequest(
            clientKey,
            policy,
            Clock::now());
    }
};

// ============================================================
// 6. DEMONSTRATION
// ============================================================

int main()
{
    RateLimiter limiter;

    // Alice: burst of 3 requests, then refill at 1 token/sec.
    RateLimitPolicy alicePolicy;
    alicePolicy.algorithm = RateLimitAlgorithm::TOKEN_BUCKET;
    alicePolicy.bucketCapacity = 3.0;
    alicePolicy.refillTokensPerSecond = 1.0;

    limiter.setPolicy("alice", alicePolicy);

    // Bob: at most 2 requests per 10-second fixed window.
    RateLimitPolicy bobPolicy;
    bobPolicy.algorithm = RateLimitAlgorithm::FIXED_WINDOW;
    bobPolicy.maxRequests = 2;
    bobPolicy.windowSize = seconds{10};

    limiter.setPolicy("bob", bobPolicy);

    cout << "=== Token Bucket: Alice ===\n";

    for (int i = 1; i <= 5; ++i)
    {
        auto result = limiter.allowRequest("alice");

        cout << "Request " << i
             << " | Allowed: " << boolalpha << result.allowed
             << " | Remaining: " << result.remaining
             << " | Retry after: "
             << result.retryAfter.count() << " ms"
             << " | " << result.reason << '\n';
    }

    cout << "\n=== Fixed Window: Bob ===\n";

    for (int i = 1; i <= 3; ++i)
    {
        auto result = limiter.allowRequest("bob");

        cout << "Request " << i
             << " | Allowed: " << boolalpha << result.allowed
             << " | Remaining: " << result.remaining
             << " | Retry after: "
             << result.retryAfter.count() << " ms"
             << " | " << result.reason << '\n';
    }

    // Concurrency test: a fresh limiter with a limit of 5.
    cout << "\n=== Concurrent Request Test ===\n";

    RateLimiter concurrentLimiter;

    RateLimitPolicy testPolicy;
    testPolicy.algorithm = RateLimitAlgorithm::FIXED_WINDOW;
    testPolicy.maxRequests = 5;
    testPolicy.windowSize = seconds{60};

    concurrentLimiter.setPolicy("test-client", testPolicy);

    atomic<int> allowedCount{0};
    vector<thread> threads;

    for (int i = 0; i < 20; ++i)
    {
        threads.emplace_back([&]()
                             {
            auto result =
                concurrentLimiter.allowRequest("test-client");

            if (result.allowed) {
                ++allowedCount;
            } });
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    cout << "Concurrent requests: 20\n";
    cout << "Allowed requests: " << allowedCount.load() << '\n';
    cout << "Expected allowed count: 5\n";

    return 0;
}
