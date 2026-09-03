Rate limiter- A rate limiter answers one fundamental question: “Should I allow this request right now?”
Usually the rejected request gets: HTTP 429 Too Many Requests


Why do we need rate limiting?
Protect the server
Fairness
Protect downstream systems( Suppose your api is capable of 50k req per sec but database can handle 5k req per sec. So we might intentionally rate-limit at 4k req/sec).
Abuse prevention(Login attempts, OTP generation, Password reset, search apis, expensive AI apis)


Rate limiting is fundamentally a resource-allocation problem over time.
Requests = demand
Capacity = supply
Rate limiter = admission controller
This mental model helps you derive almost every algorithm.


Major rate-limiting algorithms
1. Fixed Window Counter
2. Sliding Window Log
3. Sliding Window Counter
4. Token Bucket
5. Leaky Bucket


FIXED WINDOW COUNTER-
class RateLimiter {
private:
    int limit;          // maximum requests
    int windowSize;     // window size in seconds

    struct UserInfo {
        int requestCount;
        int windowStart;
    };

    unordered_map<string, UserInfo> users;

public:
    RateLimiter(int limit, int windowSize) {
        this->limit = limit;
        this->windowSize = windowSize;
    }

    bool allowRequest(string userId, int currentTime) {

        // First request from this user
        if (users.find(userId) == users.end()) {
            users[userId] = {1, currentTime};
            return true;
        }

        UserInfo &user = users[userId];

        // Current window expired
        if (currentTime - user.windowStart >= windowSize) {
            user.windowStart = currentTime;
            user.requestCount = 1;
            return true;
        }

        // Limit reached
        if (user.requestCount >= limit) {
            return false;
        }

        // Allow request
        user.requestCount++;

        return true;
    }
};
int main() {

    RateLimiter limiter(5, 10);
    cout << limiter.allowRequest("user1", 1) << endl;
    cout << limiter.allowRequest("user1", 2) << endl;
    cout << limiter.allowRequest("user1", 3) << endl;
    cout << limiter.allowRequest("user1", 4) << endl;
    cout << limiter.allowRequest("user1", 5) << endl;
    // 6th request in same window
    cout << limiter.allowRequest("user1", 6) << endl;
    // New window
    cout << limiter.allowRequest("user1", 11) << endl;
}
1 1 1 1 1 0 1  


Redis Algo-
limit = 100
windowSize = 60 seconds

on every request:
    currentTime = current Unix timestamp
    windowId = currentTime / windowSize
    key = "rate_limit:" + clientId + ":" + windowId
    count = Redis.INCR(key)
    if count == 1:
        ttl = windowSize - (currentTime % windowSize)
        Redis.EXPIRE(key, ttl)

    if count > limit:
        return REJECT

    return ALLOW

rate_limit:user123:29384752 → 73   TTL: 12 sec  redis id =userid+timestamp/windowsize
rate_limit:user456:29384752 → 41   TTL: 12 sec
rate_limit:user789:29384752 → 99   TTL: 12 sec

-------------------------------------------------------------------------------------------------------------------

SLIDING WINDOW LOG
Instead of storing just a counter, store timestamps.
For:5 requests / minute
we maintain:timestamps = [t1, t2, t3, ...]
When a request arrives at now:
Remove timestamps older than: now - 60 seconds
Then:
if timestamps.size < 5:
    add(now)
    ALLOW
else:
    REJECT

Use Queue / Deque-because timestamps arrive in sorted order. Deque<Long> timestamps;
Operations:
push_back(now)
pop_front()
front()
size()
All O(1).

---------------------------------------------------------------------------------------------------

SLIDING WINDOW COUNTER (Fixed window+sliding window log)
Hybrid approach and approximation approach- Cloudflare uses this
Instead of storing every timestamp:
previous window count
current window count

Example:
Limit = 100/min
Suppose:
Previous minute = 80
Current minute = 30
If we're 25% into the current window:
estimated count =
80 × 75% + 30
= 60 + 30
= 90
Therefore:90 < 100 → allow.

It uses much less memory than a sliding log but is an approximation.

---------------------------------------------------------------------------------------------------------

Token bucket
Imagine a bucket containing tokens.

                 ┌─────────────┐
                 │ ● ● ● ● ●   │
                 │ ● ● ● ●     │
                 │             │
                 └─────────────┘
Each request needs: 1 token
Tokens are continuously added at a fixed rate.
Suppose:
capacity = 10 tokens
refill rate = 2 tokens/sec
Initially:
10 tokens
Request arrives:
token = 1
Bucket:9
Another request:8

Suppose the bucket has: 3 tokens and no requests happen for 2 seconds.
Refill: 2 tokens/sec × 2 sec = 4 tokens
So:
3 + 4 = 7
But bucket capacity is 10.
Therefore:
tokens = min(capacity, tokens + refill)

Let:
T = current timestamp
last = last refill timestamp
R = refill rate
C = bucket capacity
tokens = current token count

Elapsed time:
elapsed = T - last
New tokens: newTokens = elapsed × R
Updated tokens: tokens = min(C, tokens + newTokens)
Then:
if tokens >= 1:
    tokens -= 1
    ALLOW
else:
    REJECT

Code-
public class TokenBucket {

    private final double capacity;
    private final double refillRate;

    private double tokens;
    private long lastRefillTime;

    public TokenBucket(double capacity, double refillRate) {
        this.capacity = capacity;
        this.refillRate = refillRate;

        this.tokens = capacity;
        this.lastRefillTime = System.nanoTime();
    }

    public synchronized boolean allow() {

        refill();

        if (tokens >= 1.0) {
            tokens -= 1.0;
            return true;
        }

        return false;
    }

    private void refill() {

        long now = System.nanoTime();

        double elapsedSeconds =
                (now - lastRefillTime) / 1_000_000_000.0;

        double newTokens = elapsedSeconds * refillRate;

        tokens = Math.min(
                capacity,
                tokens + newTokens
        );

        lastRefillTime = now;
    }
}


Multiple users-
public class TokenBucketRateLimiter
        implements RateLimiter {

    private final Map<String, TokenBucket> buckets =
            new ConcurrentHashMap<>();

    private final int capacity;
    private final double refillRate;

    public TokenBucketRateLimiter(
            int capacity,
            double refillRate) {

        this.capacity = capacity;
        this.refillRate = refillRate;
    }

    @Override
    public boolean allow(String key) {

        TokenBucket bucket = buckets.computeIfAbsent(
                key,
                k -> new TokenBucket(
                        capacity,
                        refillRate
                )
        );

        return bucket.allow();
    }
}

Usage:
RateLimiter limiter = new TokenBucketRateLimiter(5, 1);
limiter.allow("user123");

--------------------------------------------------------------------------------------------------------

Leaky Bucket
Bucket de vich meri requests add hoyi jani h and i will have fixed size queue after bucket jis vicho requests leak hongiya then i wil process it
Requests
   |
   v
+---------+
| Queue   |
+---------+
    |
    | fixed rate
    v
 Backend


------------------------------------------------------------------------------------------------------------
Which one would you use?  - It depends on whether we need strict rolling-window semantics, burst tolerance, or traffic smoothing. For a typical API gateway, I'd use token bucket because it gives O(1) state per key, supports controlled bursts, and can be implemented efficiently in a distributed system. If strict sliding-window semantics are required, I'd consider a sliding-window counter or log.
Dont directly say that Token bucket is the best

------------------------------------------------------------------------------------------------------------

Strategy Pattern in LLD
                 RateLimiter
                     |
          +----------+----------+
          |          |          |
          v          v          v
     FixedWindow  TokenBucket  SlidingWindow

--------------------------------------------------------------------------------------------------------------
HLD Architecture

                  Internet
                     |
                     v
               Load Balancer
                     |
          +----------+----------+
          |          |          |
          v          v          v
       API-1      API-2      API-3
          |          |          |
          +----------+----------+
                     |
                     v
              Rate Limiter
                     |
                     v
                  Redis

Why Redis? Because Redis provides:
Very low latency
In-memory storage
Atomic commands
Lua scripting
TTL
Replication
Cluster support

------------------------------------------------------------------------------------------------------------------

Rate Limit Headers
A good API often returns information such as:
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 37
X-RateLimit-Reset: 1720000000

When rejected:
HTTP/1.1 429 Too Many Requests
Retry-After: 3

This helps clients back off.

------------------------------------------------------------------------------------------------------------------

What Happens When Rate Limiter Is Down?
Suppose-
API
 |
 v
Rate Limiter
 |
 X
Redis unavailable

Do we:
Fail open? Allow request.
Redis failure
    |
    v
allow

Pros:
Availability
Users aren't incorrectly blocked
Cons:
Backend can become overloaded
Attackers may exploit the failure

Fail closed? Reject request.
Redis failure
    |
    v
reject

Pros:
Protects backend
Cons:
Rate limiter outage becomes application outage

There isn't one universally correct answer.
For a critical backend, you might choose:
Fail closed for highly expensive/protected operations, but use a local emergency limiter or bounded fail-open policy for less critical traffic.

------------------------------------------------------------------------------------------------------------------

What if Redis dies? use local emergency limiter.This isn't perfectly globally accurate, but protects the service.
The trade-off is:
Consistency <-> Availability

Possible architecture:

                Request
                   |
                   v
             API Gateway
                   |
            +------+------+
            |             |
            v             v
      Local limiter     Redis

------------------------------------------------------------------------------------------------------------------

One Redis instance won't necessarily be enough-> use Redis Cluster
We can shard:
             Redis Cluster
          /       |       \
       shard1   shard2   shard3
For user-based rate limiting: hash(userId) -> shard

------------------------------------------------------------------------------------------------------------------

Hot Keys - This is a classic distributed-systems problem.
Suppose one extremely popular API key: user = celebrity
generates: 500K requests/sec
All requests hit: rate_limit:celebrity
One Redis shard becomes hot.
Even though the cluster has 100 nodes, one key maps to one shard.

Solutions depend on semantics:
Local pre-limiting
Hierarchical limiting- multilevel limiting
Partitioning/sharding state where possible
Request coalescing
Dedicated handling for exceptionally hot identities
Approximate/global limits where exactness isn't necessary

But splitting one user's bucket across many nodes is not trivial because it can break the exact global limit.

------------------------------------------------------------------------------------------------------------------

Strong vs Approximate Rate Limiting
Ask:Do we need mathematically exact enforcement?
For some APIs:100 requests/minute exactly matters.
For others:approximately 100 requests/minute is sufficient.

Approximate approaches can dramatically improve:
scalability
latency
memory
availability 