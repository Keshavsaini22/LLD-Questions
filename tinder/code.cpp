#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <ctime>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <stdexcept>

using namespace std;

//======================================================
// ENUMS
//======================================================

enum class Gender {
    MALE,
    FEMALE,
    OTHER
};

enum class SwipeType {
    LEFT,
    RIGHT
};

//======================================================
// LOCATION
//======================================================

class Location {
private:
    double latitude;
    double longitude;

public:
    Location(double latitude = 0, double longitude = 0)
        : latitude(latitude), longitude(longitude) {}

    double distanceTo(const Location &other) const {
        double dx = latitude - other.latitude;
        double dy = longitude - other.longitude;
        return sqrt(dx * dx + dy * dy);
    }
};

//======================================================
// MATCH PREFERENCE
//======================================================

class MatchPreference {
private:
    int minAge;
    int maxAge;
    Gender interestedGender;
    double maxDistance;

public:
    MatchPreference(int minAge, int maxAge, Gender interestedGender, double maxDistance)
        : minAge(minAge), maxAge(maxAge), interestedGender(interestedGender), maxDistance(maxDistance) {}

    int getMinAge() const {
        return minAge;
    }

    int getMaxAge() const {
        return maxAge;
    }

    Gender getInterestedGender() const {
        return interestedGender;
    }

    double getMaxDistance() const {
        return maxDistance;
    }
};

//======================================================
// USER PROFILE
//======================================================

class UserProfile {
private:
    int id;
    string name;
    int age;
    Gender gender;
    Location location;
    string bio;
    vector<string> photos;
    vector<string> interests;
    MatchPreference preference;

public:
    UserProfile(
        int id,
        const string &name,
        int age,
        Gender gender,
        const Location &location,
        const string &bio,
        const vector<string> &photos,
        const vector<string> &interests,
        const MatchPreference &preference)
        : id(id),
          name(name),
          age(age),
          gender(gender),
          location(location),
          bio(bio),
          photos(photos),
          interests(interests),
          preference(preference) {}

    int getId() const {
        return id;
    }

    string getName() const {
        return name;
    }

    int getAge() const {
        return age;
    }

    Gender getGender() const {
        return gender;
    }

    const Location &getLocation() const {
        return location;
    }

    const vector<string> &getInterests() const {
        return interests;
    }

    const MatchPreference &getPreference() const {
        return preference;
    }
};

//======================================================
// SWIPE
//======================================================

class Swipe {
private:
    int swipeId;
    shared_ptr<UserProfile> fromUser;
    shared_ptr<UserProfile> toUser;
    SwipeType type;

public:
    Swipe(
        int swipeId,
        shared_ptr<UserProfile> fromUser,
        shared_ptr<UserProfile> toUser,
        SwipeType type)
        : swipeId(swipeId),
          fromUser(fromUser),
          toUser(toUser),
          type(type) {}

    shared_ptr<UserProfile> getFromUser() const {
        return fromUser;
    }

    shared_ptr<UserProfile> getToUser() const {
        return toUser;
    }

    SwipeType getType() const {
        return type;
    }
};

//======================================================
// MATCH
//======================================================

class Match {
private:
    int matchId;
    shared_ptr<UserProfile> user1;
    shared_ptr<UserProfile> user2;

public:
    Match(
        int matchId,
        shared_ptr<UserProfile> user1,
        shared_ptr<UserProfile> user2)
        : matchId(matchId),
          user1(user1),
          user2(user2) {}

    int getMatchId() const {
        return matchId;
    }

    shared_ptr<UserProfile> getFirstUser() const {
        return user1;
    }

    shared_ptr<UserProfile> getSecondUser() const {
        return user2;
    }
};

//======================================================
// MESSAGE
//======================================================

class Message {
private:
    int messageId;
    shared_ptr<UserProfile> sender;
    string content;
    time_t timestamp;

public:
    Message(
        int messageId,
        shared_ptr<UserProfile> sender,
        const string &content)
        : messageId(messageId),
          sender(sender),
          content(content),
          timestamp(time(nullptr)) {}

    shared_ptr<UserProfile> getSender() const {
        return sender;
    }

    string getContent() const {
        return content;
    }

    time_t getTimestamp() const {
        return timestamp;
    }
};

//======================================================
// CHAT ROOM
//======================================================

class ChatRoom {
private:
    int roomId;
    shared_ptr<Match> match;
    vector<Message> messages;

public:
    ChatRoom(int roomId, shared_ptr<Match> match)
        : roomId(roomId), match(match) {}

    void addMessage(const Message &message) {
        messages.push_back(message);
    }

    const vector<Message> &getMessages() const {
        return messages;
    }

    shared_ptr<Match> getMatch() const {
        return match;
    }
};

//======================================================
// NOTIFICATION OBSERVER
//======================================================

class INotificationObserver {
public:
    virtual ~INotificationObserver() = default;
    virtual void notify(const UserProfile &user, const string &message) = 0;
};

//======================================================
// PUSH NOTIFICATION
//======================================================

class PushNotification : public INotificationObserver {
public:
    void notify(const UserProfile &user, const string &message) override {
        cout << "[Push] " << user.getName() << " : " << message << endl;
    }
};

//======================================================
// EMAIL NOTIFICATION
//======================================================

class EmailNotification : public INotificationObserver {
public:
    void notify(const UserProfile &user, const string &message) override {
        cout << "[Email] " << user.getName() << " : " << message << endl;
    }
};

//======================================================
// NOTIFICATION SERVICE
//======================================================

class NotificationService {
private:
    vector<shared_ptr<INotificationObserver>> observers;

    void notifyAll(const UserProfile &user, const string &message) {
        for (auto &observer : observers) {
            observer->notify(user, message);
        }
    }

public:
    void addObserver(shared_ptr<INotificationObserver> observer) {
        observers.push_back(observer);
    }

    void notifyMatch(const UserProfile &user, const UserProfile &matchedUser) {
        notifyAll(user, "You matched with " + matchedUser.getName());
    }

    void notifyNewMessage(const UserProfile &receiver, const UserProfile &sender) {
        notifyAll(receiver, "New message from " + sender.getName());
    }

    void notifyProfileLiked(const UserProfile &user) {
        notifyAll(user, "Someone liked your profile.");
    }

    void notifyProfileUpdated(const UserProfile &user) {
        notifyAll(user, "Your profile has been updated.");
    }
};

//======================================================
// USER MANAGER
//======================================================

class UserManager {
private:
    unordered_map<int, shared_ptr<UserProfile>> users;

public:
    void addUser(shared_ptr<UserProfile> user) {
        users[user->getId()] = user;
    }

    shared_ptr<UserProfile> getUser(int userId) const {
        auto it = users.find(userId);
        if (it == users.end()) {
            throw runtime_error("User not found");
        }
        return it->second;
    }

    vector<shared_ptr<UserProfile>> getAllUsers() const {
        vector<shared_ptr<UserProfile>> result;
        for (const auto &[id, user] : users) {
            result.push_back(user);
        }
        return result;
    }

    void updateUser(shared_ptr<UserProfile> user) {
        auto it = users.find(user->getId());
        if (it == users.end()) {
            throw runtime_error("User not found");
        }
        it->second = user;
    }

    bool userExists(int userId) const {
        return users.count(userId);
    }
};

//======================================================
// MATCH SERVICE
//======================================================

class MatchService {
private:
    int nextMatchId = 1;
    int nextRoomId = 1;
    NotificationService &notificationService;

    // user -> users liked by him
    unordered_map<int, unordered_set<int>> rightSwipes;

    // user -> users disliked by him
    unordered_map<int, unordered_set<int>> leftSwipes;

    // user -> matched users
    unordered_map<int, unordered_set<int>> matches;

    unordered_map<int, shared_ptr<Match>> matchMap;

    unordered_map<int, shared_ptr<ChatRoom>> chatRooms;

public:
    MatchService(NotificationService &notificationService)
        : notificationService(notificationService) {}

    //--------------------------------------------------
    // Used by DiscoveryService
    //--------------------------------------------------
    bool hasAlreadySwiped(int fromUserId, int toUserId) const {
        auto right = rightSwipes.find(fromUserId);
        if (right != rightSwipes.end() && right->second.count(toUserId)) {
            return true;
        }

        auto left = leftSwipes.find(fromUserId);
        if (left != leftSwipes.end() && left->second.count(toUserId)) {
            return true;
        }

        return false;
    }

    bool isAlreadyMatched(int user1, int user2) const {
        auto it = matches.find(user1);
        if (it == matches.end()) {
            return false;
        }
        return it->second.count(user2);
    }

    //--------------------------------------------------
    // Swipe Left
    //--------------------------------------------------
    void swipeLeft(shared_ptr<UserProfile> fromUser, shared_ptr<UserProfile> toUser) {
        leftSwipes[fromUser->getId()].insert(toUser->getId());
    }

    //--------------------------------------------------
    // Swipe Right
    //--------------------------------------------------
    void swipeRight(shared_ptr<UserProfile> fromUser, shared_ptr<UserProfile> toUser) {
        int fromId = fromUser->getId();
        int toId = toUser->getId();

        rightSwipes[fromId].insert(toId);

        //--------------------------------------------------
        // Mutual Swipe ?
        //--------------------------------------------------
        if (rightSwipes[toId].count(fromId) == 0) {
            return;
        }

        //--------------------------------------------------
        // Already matched ?
        //--------------------------------------------------
        if (isAlreadyMatched(fromId, toId)) {
            return;
        }

        //--------------------------------------------------
        // Create Match
        //--------------------------------------------------
        auto match = make_shared<Match>(nextMatchId++, fromUser, toUser);
        matchMap[match->getMatchId()] = match;

        matches[fromId].insert(toId);
        matches[toId].insert(fromId);

        //--------------------------------------------------
        // Create Chat Room
        //--------------------------------------------------
        auto room = make_shared<ChatRoom>(nextRoomId++, match);
        chatRooms[match->getMatchId()] = room;

        //--------------------------------------------------
        // Notify Users
        //--------------------------------------------------
        notificationService.notifyMatch(*fromUser, *toUser);
        notificationService.notifyMatch(*toUser, *fromUser);
    }

    //--------------------------------------------------
    // APIs
    //--------------------------------------------------
    vector<int> getMatches(int userId) const {
        vector<int> result;
        auto it = matches.find(userId);
        if (it == matches.end()) {
            return result;
        }
        for (int id : it->second) {
            result.push_back(id);
        }
        return result;
    }

    shared_ptr<ChatRoom> getChatRoom(int matchId) {
        return chatRooms.at(matchId);
    }

    shared_ptr<ChatRoom> getChatRoom(int user1Id, int user2Id) {
        for (auto &[matchId, room] : chatRooms) {
            auto m = room->getMatch();
            int u1 = m->getFirstUser()->getId();
            int u2 = m->getSecondUser()->getId();
            if ((u1 == user1Id && u2 == user2Id) || (u1 == user2Id && u2 == user1Id)) {
                return room;
            }
        }
        throw runtime_error("Chat room not found");
    }
};

//======================================================
// PROFILE RANKING STRATEGY
//======================================================

class ProfileRankingStrategy {
public:
    virtual ~ProfileRankingStrategy() = default;

    virtual vector<shared_ptr<UserProfile>> rankProfiles(
        shared_ptr<UserProfile> currentUser,
        vector<shared_ptr<UserProfile>> candidates) = 0;
};

//======================================================
// DEFAULT PROFILE RANKING STRATEGY
//======================================================

class DefaultProfileRankingStrategy : public ProfileRankingStrategy {
private:
    double calculateScore(shared_ptr<UserProfile> currentUser, shared_ptr<UserProfile> candidate) {
        double score = 0;

        //--------------------------------------------------
        // Distance Score
        //--------------------------------------------------
        double distance = currentUser->getLocation().distanceTo(candidate->getLocation());
        score += max(0.0, 100.0 - distance);

        //--------------------------------------------------
        // Interest Score
        //--------------------------------------------------
        unordered_set<string> interests(
            currentUser->getInterests().begin(),
            currentUser->getInterests().end()
        );

        int commonInterest = 0;
        for (const auto &interest : candidate->getInterests()) {
            if (interests.count(interest)) {
                commonInterest++;
            }
        }
        score += commonInterest * 20;

        //--------------------------------------------------
        // Age Preference Score
        //--------------------------------------------------
        const auto &pref = currentUser->getPreference();
        if (candidate->getAge() >= pref.getMinAge() && candidate->getAge() <= pref.getMaxAge()) {
            score += 50;
        }

        return score;
    }

public:
    vector<shared_ptr<UserProfile>> rankProfiles(
        shared_ptr<UserProfile> currentUser,
        vector<shared_ptr<UserProfile>> candidates) override {
        sort(candidates.begin(), candidates.end(), [&](auto &user1, auto &user2) {
            return calculateScore(currentUser, user1) > calculateScore(currentUser, user2);
        });
        return candidates;
    }
};

//======================================================
// DISCOVERY SERVICE
//======================================================

class DiscoveryService {
private:
    UserManager &userManager;
    MatchService &matchService;
    ProfileRankingStrategy &rankingStrategy;

public:
    DiscoveryService(
        UserManager &userManager,
        MatchService &matchService,
        ProfileRankingStrategy &rankingStrategy)
        : userManager(userManager),
          matchService(matchService),
          rankingStrategy(rankingStrategy) {}

    vector<shared_ptr<UserProfile>> getProfilesForUser(shared_ptr<UserProfile> currentUser) {
        vector<shared_ptr<UserProfile>> candidates;
        auto users = userManager.getAllUsers();

        for (auto &user : users) {
            //--------------------------------------------------
            // Skip Self
            //--------------------------------------------------
            if (user->getId() == currentUser->getId()) {
                continue;
            }

            //--------------------------------------------------
            // Gender Preference
            //--------------------------------------------------
            if (user->getGender() != currentUser->getPreference().getInterestedGender()) {
                continue;
            }

            //--------------------------------------------------
            // Age Filter
            //--------------------------------------------------
            if (user->getAge() < currentUser->getPreference().getMinAge() ||
                user->getAge() > currentUser->getPreference().getMaxAge()) {
                continue;
            }

            //--------------------------------------------------
            // Distance Filter
            //--------------------------------------------------
            double distance = currentUser->getLocation().distanceTo(user->getLocation());
            if (distance > currentUser->getPreference().getMaxDistance()) {
                continue;
            }

            //--------------------------------------------------
            // Already Swiped
            //--------------------------------------------------
            if (matchService.hasAlreadySwiped(currentUser->getId(), user->getId())) {
                continue;
            }

            //--------------------------------------------------
            // Already Matched
            //--------------------------------------------------
            if (matchService.isAlreadyMatched(currentUser->getId(), user->getId())) {
                continue;
            }

            candidates.push_back(user);
        }

        return rankingStrategy.rankProfiles(currentUser, candidates);
    }
};

//======================================================
// CHAT SERVICE
//======================================================

class ChatService {
private:
    MatchService &matchService;
    NotificationService &notificationService;
    int nextMessageId = 1;

public:
    ChatService(MatchService &matchService, NotificationService &notificationService)
        : matchService(matchService), notificationService(notificationService) {}

    void sendMessage(shared_ptr<UserProfile> sender, shared_ptr<UserProfile> receiver, const string &content) {
        auto room = matchService.getChatRoom(sender->getId(), receiver->getId());
        Message message(nextMessageId++, sender, content);
        room->addMessage(message);
        notificationService.notifyNewMessage(*receiver, *sender);
    }

    vector<Message> getMessages(shared_ptr<UserProfile> user1, shared_ptr<UserProfile> user2) {
        auto room = matchService.getChatRoom(user1->getId(), user2->getId());
        return room->getMessages();
    }
};

//======================================================
// TINDER FACADE
//======================================================

class TinderFacade {
private:
    UserManager &userManager;
    DiscoveryService &discoveryService;
    MatchService &matchService;
    ChatService &chatService;

public:
    TinderFacade(
        UserManager &userManager,
        DiscoveryService &discoveryService,
        MatchService &matchService,
        ChatService &chatService)
        : userManager(userManager),
          discoveryService(discoveryService),
          matchService(matchService),
          chatService(chatService) {}

    void registerUser(shared_ptr<UserProfile> user) {
        userManager.addUser(user);
    }

    void updateProfile(shared_ptr<UserProfile> user) {
        userManager.updateUser(user);
    }

    vector<shared_ptr<UserProfile>> getRecommendedProfiles(int userId) {
        return discoveryService.getProfilesForUser(userManager.getUser(userId));
    }

    void swipeRight(int fromUserId, int toUserId) {
        matchService.swipeRight(userManager.getUser(fromUserId), userManager.getUser(toUserId));
    }

    void swipeLeft(int fromUserId, int toUserId) {
        matchService.swipeLeft(userManager.getUser(fromUserId), userManager.getUser(toUserId));
    }

    void sendMessage(int senderId, int receiverId, const string &message) {
        chatService.sendMessage(userManager.getUser(senderId), userManager.getUser(receiverId), message);
    }

    vector<Message> getMessages(int user1Id, int user2Id) {
        return chatService.getMessages(userManager.getUser(user1Id), userManager.getUser(user2Id));
    }

    vector<int> getMatches(int userId) {
        return matchService.getMatches(userId);
    }
};

int main() {
    NotificationService notificationService;

    notificationService.addObserver(make_shared<PushNotification>());
    notificationService.addObserver(make_shared<EmailNotification>());

    UserManager userManager;
    DefaultProfileRankingStrategy rankingStrategy;
    MatchService matchService(notificationService);

    DiscoveryService discoveryService(
        userManager,
        matchService,
        rankingStrategy
    );

    ChatService chatService(
        matchService,
        notificationService
    );

    TinderFacade tinder(
        userManager,
        discoveryService,
        matchService,
        chatService
    );

    auto user1 = make_shared<UserProfile>(
        1,
        "Alpha",
        24,
        Gender::MALE,
        Location(0, 0),
        "Love travelling",
        vector<string>{"p1"},
        vector<string>{"Music", "Coding", "Travel"},
        MatchPreference(20, 28, Gender::FEMALE, 20)
    );

    auto user2 = make_shared<UserProfile>(
        2,
        "Neha",
        23,
        Gender::FEMALE,
        Location(2, 3),
        "Coffee Lover",
        vector<string>{"p2"},
        vector<string>{"Music", "Movies"},
        MatchPreference(22, 28, Gender::MALE, 20)
    );

    auto user3 = make_shared<UserProfile>(
        3,
        "Priya",
        25,
        Gender::FEMALE,
        Location(5, 4),
        "Traveller",
        vector<string>{"p3"},
        vector<string>{"Travel", "Coding"},
        MatchPreference(22, 30, Gender::MALE, 30)
    );

    tinder.registerUser(user1);
    tinder.registerUser(user2);
    tinder.registerUser(user3);

    cout << "\nRecommended Profiles\n";

    auto profiles = tinder.getRecommendedProfiles(1);
    for (auto &profile : profiles) {
        cout << profile->getName() << endl;
    }

    tinder.swipeRight(1, 2);
    tinder.swipeRight(2, 1);

    tinder.sendMessage(1, 2, "Hi Neha!");
    tinder.sendMessage(2, 1, "Hello Alpha!");

    cout << "\nChat\n";

    auto messages = tinder.getMessages(1, 2);
    for (auto &message : messages) {
        cout << message.getSender()->getName() << " : " << message.getContent() << endl;
    }

    return 0;
}

// 🔶 Minor improvements (only if interviewer asks)
// Store Swipe objects instead of only IDs (audit/history).
// ProfileRankingStrategy can return Top K instead of sorting all candidates.
// Extract SwipeRepository if swipe persistence grows.
// Make MatchService thread-safe (mutex) for simultaneous swipes.
// Use time_point instead of time_t.