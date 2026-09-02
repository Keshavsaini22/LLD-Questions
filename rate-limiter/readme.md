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
