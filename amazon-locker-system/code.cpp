// I am not using State Design Pattern in this implementation as the locker system is relatively simple and does not require complex state management.
// The locker can be in one of a few states (available, occupied, maintenance), and the operations on the locker are straightforward. Using a state design
// pattern would add unnecessary complexity to the code without providing significant benefits in terms of maintainability or scalability. Instead, I will use
// simple conditional checks to manage the locker states effectively.
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <ctime>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>

using namespace std;

enum class Size
{
    SMALL,
    MEDIUM,
    LARGE
};

enum class TokenStatus
{
    ACTIVE,
    USED,
    EXPIRED
};

class Package
{
    string packageId;
    Size size;

public:
    Package(string id, Size s) : packageId(id), size(s) {}

    string getId() const { return packageId; }
    Size getSize() const { return size; }
};

class AccessToken
{
    string token;
    time_t expiryTime;
    TokenStatus status;

public:
    AccessToken(string token, time_t expiry)
        : token(token), expiryTime(expiry), status(TokenStatus::ACTIVE) {}

    string getToken() const { return token; }

    bool isExpired()
    {
        if (status == TokenStatus::USED)
            return false;

        if (time(nullptr) > expiryTime)
        {
            status = TokenStatus::EXPIRED;
            return true;
        }
        return false;
    }

    bool isUsed() const
    {
        return status == TokenStatus::USED;
    }

    void markUsed()
    {
        status = TokenStatus::USED;
    }

    TokenStatus getStatus() const
    {
        return status;
    }
};

class Compartment
{
    int id;
    Size size;
    bool occupied;
    Package *package;
    AccessToken *token;

public:
    Compartment(int id, Size size)
        : id(id), size(size), occupied(false),
          package(nullptr), token(nullptr) {}

    bool isAvailable() const
    {
        return !occupied;
    }

    Size getSize() const
    {
        return size;
    }

    int getId() const
    {
        return id;
    }

    void assignPackage(Package *pkg, AccessToken *tkn)
    {
        occupied = true;
        package = pkg;
        token = tkn;
    }

    void clearCompartment()
    {
        occupied = false;
        package = nullptr;
        token = nullptr;
    }

    AccessToken *getToken()
    {
        return token;
    }

    void open()
    {
        cout << "Opening compartment " << id << endl;
    }
};

class Locker
{
    vector<Compartment *> compartments;
    unordered_map<string, Compartment *> tokenMap;

public:
    Locker(vector<Compartment *> comps) : compartments(comps) {}

    string generateToken()
    {
        return "TOKEN_" + to_string(rand() % 1000000);
    }

    string depositPackage(Package *pkg)
    {
        for (auto compartment : compartments)
        {
            if (compartment->isAvailable() &&
                compartment->getSize() == pkg->getSize())
            {

                string tokenStr = generateToken();
                time_t expiry = time(nullptr) + 7 * 24 * 60 * 60;

                AccessToken *token =
                    new AccessToken(tokenStr, expiry);

                compartment->assignPackage(pkg, token);
                tokenMap[tokenStr] = compartment;

                compartment->open();

                return tokenStr;
            }
        }

        throw runtime_error("No available compartment");
    }

    void retrievePackage(string tokenStr)
    {
        if (tokenMap.find(tokenStr) == tokenMap.end())
        {
            throw runtime_error("Invalid token");
        }

        Compartment *compartment = tokenMap[tokenStr];
        AccessToken *token = compartment->getToken();

        if (token->isUsed())
        {
            throw runtime_error("Token already used");
        }

        if (token->isExpired())
        {
            throw runtime_error("Token expired");
        }

        compartment->open();
        token->markUsed();
        compartment->clearCompartment();
    }

    void handleExpiredPackages()
    {
        for (auto compartment : compartments)
        {
            AccessToken *token = compartment->getToken();

            if (token && token->isExpired())
            {
                compartment->open();
                compartment->clearCompartment();
            }
        }
    }
};

int main()
{
    vector<Compartment *> comps = {
        new Compartment(1, Size::SMALL),
        new Compartment(2, Size::MEDIUM),
        new Compartment(3, Size::LARGE)};

    Locker locker(comps);

    Package *pkg = new Package("PKG1", Size::SMALL);

    string token = locker.depositPackage(pkg);
    cout << "Generated token: " << token << endl;

    locker.retrievePackage(token);

    return 0;
}