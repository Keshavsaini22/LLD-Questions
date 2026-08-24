#include <iostream>
#include <string>
#include <queue>
#include <unordered_map>

using namespace std;

class RateLimiter {
    int windowSize;
    int limit;

    // Each user has their own queue of request timestamps
    unordered_map<string, queue<int>> userRequests;

public:
    /**
     * @param limit Maximum requests allowed within the time window.
     * @param windowInSeconds Duration of the time window in seconds.
     */
    RateLimiter(int limit, int windowInSeconds) {
        this->limit = limit;
        this->windowSize = windowInSeconds;
    }

    /**
     * @param userId Unique identifier for the user or API client.
     * @param timestampInSeconds Current request epoch timestamp in seconds.
     * @return true if request is allowed, false if rate limited.
     */
    bool allowRequest(const string& userId, int timestampInSeconds) {

        queue<int>& requests = userRequests[userId];

        // Remove requests which are outside the current window
        while (!requests.empty() &&
               timestampInSeconds - requests.front() >= windowSize) {
            requests.pop();
        }

        // User has already reached the limit
        if (requests.size() >= limit) {
            return false;
        }

        // Allow this request
        requests.push(timestampInSeconds);

        return true;
    }
};

int main() {

    // Allow maximum 3 requests per 10-second window
    RateLimiter limiter(3, 10);

    cout << boolalpha;

    cout << limiter.allowRequest("User_101", 100) << "\n"; // true
    cout << limiter.allowRequest("User_101", 102) << "\n"; // true
    cout << limiter.allowRequest("User_101", 105) << "\n"; // true
    cout << limiter.allowRequest("User_101", 108) << "\n"; // false

    // 100 is outside [101, 111], so it gets removed
    cout << limiter.allowRequest("User_101", 111) << "\n"; // true

    // Different user has a separate rate limit
    cout << limiter.allowRequest("User_202", 111) << "\n"; // true
    cout << limiter.allowRequest("User_202", 112) << "\n"; // true

    return 0;
}