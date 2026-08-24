#include <iostream>
#include <string>
#include <queue>

using namespace std;

class RateLimiter {
    queue<int> requests;
    int windowSize;
    int limit;

public:
    RateLimiter(int limit, int windowInSeconds) {
        this->limit = limit;
        this->windowSize = windowInSeconds;
    }

    bool allowRequest(const string& userId, int timestampInSeconds) {

        // Remove all requests that are outside the window
        while (!requests.empty() &&
               timestampInSeconds - requests.front() >= windowSize) {
            requests.pop();
        }

        // If limit has already been reached, reject
        if (requests.size() >= limit) {
            return false;
        }

        // Otherwise accept the request
        requests.push(timestampInSeconds);

        return true;
    }
};


int main() {
    // Allow maximum 3 requests per 10-second window
    RateLimiter limiter(3, 10);

    std::cout << std::boolalpha;
    std::cout << limiter.allowRequest("User_101", 100) << "\n"; // Expected: true  (Request 1 at t=100s)
    std::cout << limiter.allowRequest("User_101", 102) << "\n"; // Expected: true  (Request 2 at t=102s)
    std::cout << limiter.allowRequest("User_101", 105) << "\n"; // Expected: true  (Request 3 at t=105s)
    std::cout << limiter.allowRequest("User_101", 108) << "\n"; // Expected: false (BLOCKED! 3 requests in [98, 108])

    // At timestamp 111s, window is [101, 111]. Request at t=100 falls outside.
    std::cout << limiter.allowRequest("User_101", 111) << "\n"; // Expected: true  (Request Allowed)

    return 0;
}