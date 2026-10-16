#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>
#include <atomic>

using namespace std;
using Clock = chrono::steady_clock;
using TimePoint = Clock::time_point;
using Milliseconds = chrono::milliseconds;

// =====================================================
// ENUMS
// =====================================================

enum class SeatType
{
    SILVER,
    GOLD,
    PREMIUM
};
enum class SeatStatus
{
    AVAILABLE,
    LOCKED,
    BOOKED
};
enum class BookingStatus
{
    CREATED,
    PENDING_PAYMENT,
    CONFIRMED,
    PAYMENT_FAILED,
    EXPIRED,
    CANCELLED,
    PAYMENT_UNKNOWN
};
enum class PaymentStatus
{
    SUCCESS,
    FAILED,
    UNKNOWN
};

// =====================================================
// DOMAIN ENTITIES
// =====================================================

class User
{
private:
    int id;
    string name;
    string email;

public:
    User(int id, string name, string email)
        : id(id), name(move(name)), email(move(email)) {}

    int getId() const { return id; }
    const string &getName() const { return name; }
    const string &getEmail() const { return email; }
};

class Movie
{
private:
    int id;
    string name;

public:
    Movie(int id, string name) : id(id), name(move(name)) {}
    int getId() const { return id; }
    const string &getName() const { return name; }
};

class Seat
{
private:
    int id;
    int row;
    int col;
    SeatType type;

public:
    Seat(int id, int row, int col, SeatType type)
        : id(id), row(row), col(col), type(type) {}

    int getId() const { return id; }
    int getRow() const { return row; }
    int getCol() const { return col; }
    SeatType getType() const { return type; }
};

class Screen
{
private:
    int id;
    string name;
    vector<unique_ptr<Seat>> seats;

public:
    Screen(int id, string name) : id(id), name(move(name)) {}

    Seat &addSeat(unique_ptr<Seat> seat)
    {
        seats.push_back(move(seat));
        return *seats.back();
    }

    const vector<unique_ptr<Seat>> &getSeats() const { return seats; }
    int getId() const { return id; }
};

class Theater
{
private:
    int id;
    string name;
    string city;
    vector<unique_ptr<Screen>> screens;

public:
    Theater(int id, string name, string city)
        : id(id), name(move(name)), city(move(city)) {}

    Screen &addScreen(unique_ptr<Screen> screen)
    {
        screens.push_back(move(screen));
        return *screens.back();
    }

    const vector<unique_ptr<Screen>> &getScreens() const { return screens; }
};

// =====================================================
// SHOW-SPECIFIC SEAT INVENTORY
// A physical Seat is static. ShowSeat holds per-show price/status.
// =====================================================

class ShowSeat
{
private:
    Seat *seat; // non-owning; Screen outlives Show
    int priceInPaise;
    SeatStatus status = SeatStatus::AVAILABLE;
    string lockId;
    int lockedByUserId = -1;
    TimePoint lockExpiry{};
    mutable mutex mutex_;

    bool expireIfNeededLocked(TimePoint now)
    {
        if (status == SeatStatus::LOCKED && now >= lockExpiry)
        {
            status = SeatStatus::AVAILABLE;
            lockId.clear();
            lockedByUserId = -1;
            return true;
        }
        return false;
    }

public:
    ShowSeat(Seat *seat, int priceInPaise)
        : seat(seat), priceInPaise(priceInPaise)
    {
        if (!seat)
            throw invalid_argument("Seat cannot be null");
        if (priceInPaise < 0)
            throw invalid_argument("Price cannot be negative");
    }

    int getSeatId() const { return seat->getId(); }
    int getPriceInPaise() const { return priceInPaise; }

    // Internal state is changed only while holding mutex_.
    friend class SeatInventoryService;
};

// =====================================================
// SHOW
// =====================================================

class Show
{
private:
    int id;
    Movie *movie;   // non-owning; Movie must outlive Show
    Screen *screen; // non-owning; Screen must outlive Show
    TimePoint startTime;
    vector<unique_ptr<ShowSeat>> showSeats;

public:
    Show(int id, Movie *movie, Screen *screen, TimePoint startTime)
        : id(id), movie(movie), screen(screen), startTime(startTime)
    {
        if (!movie || !screen)
            throw invalid_argument("Show needs movie and screen");
    }

    ShowSeat &addShowSeat(unique_ptr<ShowSeat> showSeat)
    {
        showSeats.push_back(move(showSeat));
        return *showSeats.back();
    }

    int getId() const { return id; }
    const string &getMovieName() const { return movie->getName(); }
    const vector<unique_ptr<ShowSeat>> &getSeats() const { return showSeats; }
};

// =====================================================
// BOOKING
// =====================================================

class Booking
{
private:
    string id;
    int userId;
    int showId;
    vector<int> seatIds;
    int totalAmountInPaise;
    BookingStatus status;

public:
    Booking(string id, int userId, int showId, vector<int> seatIds, int totalAmountInPaise)
        : id(move(id)), userId(userId), showId(showId),
          seatIds(move(seatIds)), totalAmountInPaise(totalAmountInPaise),
          status(BookingStatus::CREATED) {}

    const string &getId() const { return id; }
    int getUserId() const { return userId; }
    int getShowId() const { return showId; }
    const vector<int> &getSeatIds() const { return seatIds; }
    int getTotalAmountInPaise() const { return totalAmountInPaise; }
    BookingStatus getStatus() const { return status; }

    void setStatus(BookingStatus next) { status = next; }
};

// =====================================================
// PAYMENT STRATEGY
// In production, this interface must return a provider reference
// and support querying/reconciling UNKNOWN outcomes.
// =====================================================

struct PaymentResult
{
    PaymentStatus status;
    string providerReference;
    string message;
};

class PaymentStrategy
{
public:
    virtual ~PaymentStrategy() = default;
    virtual PaymentResult pay(
        const string &idempotencyKey,
        int amountInPaise,
        const User &user) = 0;
};

class UPIPayment : public PaymentStrategy
{
public:
    PaymentResult pay(const string &key, int amount, const User &user) override
    {
        cout << "UPI payment attempt key=" << key
             << ", amount=Rs " << amount / 100.0
             << ", user=" << user.getName() << '\n';
        // Demo implementation only. A real gateway call can return UNKNOWN.
        return {PaymentStatus::SUCCESS, "upi-demo-" + key, "Payment successful"};
    }
};

class CardPayment : public PaymentStrategy
{
public:
    PaymentResult pay(const string &key, int amount, const User &user) override
    {
        cout << "Card payment attempt key=" << key
             << ", amount=Rs " << amount / 100.0
             << ", user=" << user.getName() << '\n';
        return {PaymentStatus::SUCCESS, "card-demo-" + key, "Payment successful"};
    }
};

class PaymentService
{
public:
    PaymentResult processPayment(
        PaymentStrategy &strategy,
        const string &idempotencyKey,
        int amountInPaise,
        const User &user)
    {
        return strategy.pay(idempotencyKey, amountInPaise, user);
    }
};

// =====================================================
// NOTIFICATION OBSERVER
// Notification failures must not reverse a confirmed booking.
// =====================================================

class IObserver
{
public:
    virtual ~IObserver() = default;
    virtual void notify(const User &user, const Booking &booking) = 0;
};

class EmailNotification : public IObserver
{
public:
    void notify(const User &user, const Booking &booking) override
    {
        cout << "EMAIL sent to " << user.getEmail()
             << " for booking " << booking.getId() << '\n';
    }
};

class SMSNotification : public IObserver
{
public:
    void notify(const User &user, const Booking &booking) override
    {
        cout << "SMS sent to " << user.getName()
             << " for booking " << booking.getId() << '\n';
    }
};

class NotificationService
{
private:
    vector<shared_ptr<IObserver>> observers;

public:
    void addObserver(shared_ptr<IObserver> observer)
    {
        if (observer)
            observers.push_back(move(observer));
    }

    void notifySafely(const User &user, const Booking &booking) noexcept
    {
        for (const auto &observer : observers)
        {
            try
            {
                observer->notify(user, booking);
            }
            catch (const exception &e)
            {
                cerr << "Notification failed: " << e.what() << '\n';
                // Production: publish via durable outbox and retry asynchronously.
            }
            catch (...)
            {
                cerr << "Notification failed with unknown error\n";
            }
        }
    }
};

// =====================================================
// SEAT INVENTORY SERVICE
// Acquires multiple seat locks atomically within this process.
// All involved seat mutexes are acquired in stable seat-ID order.
// =====================================================

class SeatInventoryService
{
private:
    Milliseconds holdDuration;

public:
    explicit SeatInventoryService(Milliseconds holdDuration = Milliseconds(5 * 60 * 1000))
        : holdDuration(holdDuration)
    {
        if (holdDuration.count() <= 0)
            throw invalid_argument("Hold duration must be positive");
    }

    bool holdSeats(
        const vector<ShowSeat *> &requestedSeats,
        int userId,
        const string &lockId)
    {
        if (requestedSeats.empty())
            return false;
        if (lockId.empty())
            return false;

        vector<ShowSeat *> seats = requestedSeats;
        if (any_of(seats.begin(), seats.end(), [](ShowSeat *s)
                   { return s == nullptr; }))
            return false;

        sort(seats.begin(), seats.end(), [](ShowSeat *a, ShowSeat *b)
             { return a->getSeatId() < b->getSeatId(); });

        // Reject duplicates instead of charging twice for the same seat.
        for (size_t i = 1; i < seats.size(); ++i)
        {
            if (seats[i - 1]->getSeatId() == seats[i]->getSeatId())
                return false;
        }

        vector<unique_lock<mutex>> locks;
        locks.reserve(seats.size());
        for (ShowSeat *seat : seats)
            locks.emplace_back(seat->mutex_);

        const TimePoint now = Clock::now();

        // First pass: expire stale holds and validate every seat.
        for (ShowSeat *seat : seats)
        {
            seat->expireIfNeededLocked(now);
            if (seat->status != SeatStatus::AVAILABLE)
                return false;
        }

        // Second pass: all seats are available; hold all while locks are held.
        for (ShowSeat *seat : seats)
        {
            seat->status = SeatStatus::LOCKED;
            seat->lockId = lockId;
            seat->lockedByUserId = userId;
            seat->lockExpiry = now + holdDuration;
        }

        return true;
    }

    bool confirmSeats(const vector<ShowSeat *> &requestedSeats, int userId, const string &lockId)
    {
        if (requestedSeats.empty())
            return false;

        vector<ShowSeat *> seats = requestedSeats;
        if (any_of(seats.begin(), seats.end(), [](ShowSeat *s)
                   { return s == nullptr; }))
            return false;

        sort(seats.begin(), seats.end(), [](ShowSeat *a, ShowSeat *b)
             { return a->getSeatId() < b->getSeatId(); });

        for (size_t i = 1; i < seats.size(); ++i)
        {
            if (seats[i - 1]->getSeatId() == seats[i]->getSeatId())
                return false;
        }

        vector<unique_lock<mutex>> locks;
        locks.reserve(seats.size());
        for (ShowSeat *seat : seats)
            locks.emplace_back(seat->mutex_);

        const TimePoint now = Clock::now();
        for (ShowSeat *seat : seats)
        {
            seat->expireIfNeededLocked(now);
            if (seat->status != SeatStatus::LOCKED ||
                seat->lockedByUserId != userId ||
                seat->lockId != lockId ||
                now >= seat->lockExpiry)
            {
                return false;
            }
        }

        for (ShowSeat *seat : seats)
        {
            seat->status = SeatStatus::BOOKED;
            seat->lockId.clear();
            seat->lockedByUserId = -1;
        }
        return true;
    }

    void releaseSeats(
        const vector<ShowSeat *> &requestedSeats,
        int userId,
        const string &lockId)
    {
        vector<ShowSeat *> seats;
        for (ShowSeat *seat : requestedSeats)
            if (seat)
                seats.push_back(seat);

        sort(seats.begin(), seats.end(), [](ShowSeat *a, ShowSeat *b)
             { return a->getSeatId() < b->getSeatId(); });
        seats.erase(unique(seats.begin(), seats.end()), seats.end());

        vector<unique_lock<mutex>> locks;
        locks.reserve(seats.size());
        for (ShowSeat *seat : seats)
            locks.emplace_back(seat->mutex_);

        const TimePoint now = Clock::now();
        for (ShowSeat *seat : seats)
        {
            seat->expireIfNeededLocked(now);
            // A request can release only its own active hold, never another user's
            // hold and never a BOOKED seat.
            if (seat->status == SeatStatus::LOCKED &&
                seat->lockedByUserId == userId &&
                seat->lockId == lockId)
            {
                seat->status = SeatStatus::AVAILABLE;
                seat->lockId.clear();
                seat->lockedByUserId = -1;
            }
        }
    }
};

// =====================================================
// BOOKING SERVICE
// =====================================================

class BookingService
{
private:
    SeatInventoryService &inventoryService;
    PaymentService &paymentService;
    NotificationService &notificationService;

    mutex bookingMutex;
    unordered_map<string, unique_ptr<Booking>> bookings;
    unordered_map<string, string> idempotencyToBooking;
    atomic<unsigned long long> nextBookingId{1};

    string makeBookingId()
    {
        return "BKG-" + to_string(nextBookingId.fetch_add(1));
    }

public:
    BookingService(
        SeatInventoryService &inventoryService,
        PaymentService &paymentService,
        NotificationService &notificationService) : inventoryService(inventoryService),
                                                    paymentService(paymentService),
                                                    notificationService(notificationService) {}

    // This in-memory implementation serializes booking creation for a given
    // idempotency key. A production implementation needs a DB unique constraint.
    Booking *findByIdempotencyKey(const string &key)
    {
        lock_guard<mutex> guard(bookingMutex);
        auto it = idempotencyToBooking.find(key);
        if (it == idempotencyToBooking.end())
            return nullptr;
        return bookings.at(it->second).get();
    }

    Booking *createBooking(
        const User &user,
        Show &show,
        const vector<ShowSeat *> &selectedSeats,
        PaymentStrategy &paymentStrategy,
        const string &idempotencyKey)
    {
        if (idempotencyKey.empty())
            throw invalid_argument("Idempotency key is required");
        if (selectedSeats.empty())
            throw invalid_argument("Select at least one seat");

        // Fast idempotency check. A DB implementation must enforce uniqueness
        // atomically because separate application instances can race here.
        {
            lock_guard<mutex> guard(bookingMutex);
            auto existing = idempotencyToBooking.find(idempotencyKey);
            if (existing != idempotencyToBooking.end())
            {
                Booking *booking = bookings.at(existing->second).get();
                if (booking->getUserId() != user.getId())
                    throw runtime_error("Idempotency key belongs to another user");
                return booking;
            }
        }

        unordered_set<int> showSeatIds;
        for (const auto &seat : show.getSeats())
            showSeatIds.insert(seat->getSeatId());
        for (ShowSeat *seat : selectedSeats)
        {
            if (!seat || !showSeatIds.count(seat->getSeatId()))
                throw invalid_argument("Selected seat does not belong to this show");
        }

        const string bookingId = makeBookingId();
        const string lockId = "LOCK-" + bookingId;

        if (!inventoryService.holdSeats(selectedSeats, user.getId(), lockId))
            throw runtime_error("One or more seats are unavailable");

        int totalAmountInPaise = 0;
        vector<int> seatIds;
        seatIds.reserve(selectedSeats.size());

        for (ShowSeat *seat : selectedSeats)
        {
            if (seat->getPriceInPaise() > 0 &&
                totalAmountInPaise > INT32_MAX - seat->getPriceInPaise())
            {
                inventoryService.releaseSeats(selectedSeats, user.getId(), lockId);
                throw overflow_error("Booking total exceeds supported amount");
            }
            totalAmountInPaise += seat->getPriceInPaise();
            seatIds.push_back(seat->getSeatId());
        }
        sort(seatIds.begin(), seatIds.end());

        auto booking = make_unique<Booking>(
            bookingId, user.getId(), show.getId(), seatIds, totalAmountInPaise);
        booking->setStatus(BookingStatus::PENDING_PAYMENT);

        Booking *bookingPtr = booking.get();
        {
            lock_guard<mutex> guard(bookingMutex);
            // Re-check after holding seats. If another request with the same key
            // won, release this request's hold and return the existing booking.
            auto existing = idempotencyToBooking.find(idempotencyKey);
            if (existing != idempotencyToBooking.end())
            {
                inventoryService.releaseSeats(selectedSeats, user.getId(), lockId);
                return bookings.at(existing->second).get();
            }
            bookings.emplace(bookingId, move(booking));
            idempotencyToBooking[idempotencyKey] = bookingId;
        }

        PaymentResult paymentResult;
        try
        {
            paymentResult = paymentService.processPayment(
                paymentStrategy, idempotencyKey, totalAmountInPaise, user);
        }
        catch (const exception &e)
        {
            // An exception/time-out does not prove the provider did not charge.
            lock_guard<mutex> guard(bookingMutex);
            bookingPtr->setStatus(BookingStatus::PAYMENT_UNKNOWN);
            cerr << "Payment outcome unknown: " << e.what() << '\n';
            return bookingPtr;
        }

        if (paymentResult.status == PaymentStatus::FAILED)
        {
            inventoryService.releaseSeats(selectedSeats, user.getId(), lockId);
            lock_guard<mutex> guard(bookingMutex);
            bookingPtr->setStatus(BookingStatus::PAYMENT_FAILED);
            return bookingPtr;
        }

        if (paymentResult.status == PaymentStatus::UNKNOWN)
        {
            // Do not release the hold blindly: reconcile with provider first.
            lock_guard<mutex> guard(bookingMutex);
            bookingPtr->setStatus(BookingStatus::PAYMENT_UNKNOWN);
            return bookingPtr;
        }

        if (!inventoryService.confirmSeats(selectedSeats, user.getId(), lockId))
        {
            // Payment succeeded but the hold expired/lost. Do not pretend success.
            // Production: persist a compensation/refund workflow and reconcile.
            lock_guard<mutex> guard(bookingMutex);
            bookingPtr->setStatus(BookingStatus::PAYMENT_UNKNOWN);
            cerr << "Payment succeeded but seat confirmation failed; reconciliation required\n";
            return bookingPtr;
        }

        {
            lock_guard<mutex> guard(bookingMutex);
            bookingPtr->setStatus(BookingStatus::CONFIRMED);
        }

        // Never allow notification failure to undo or misreport a confirmed booking.
        notificationService.notifySafely(user, *bookingPtr);
        return bookingPtr;
    }
};

// =====================================================
// MAIN
// =====================================================

int main()
{
    try
    {
        User user1(1, "Alpha", "alpha@gmail.com");
        Movie movie1(1, "Avengers");

        Theater theater(1, "PVR Phoenix", "Mumbai");
        Screen &screen = theater.addScreen(make_unique<Screen>(1, "Audi 1"));

        Seat &seat1 = screen.addSeat(make_unique<Seat>(1, 1, 1, SeatType::GOLD));
        Seat &seat2 = screen.addSeat(make_unique<Seat>(2, 1, 2, SeatType::GOLD));

        Show show1(1, &movie1, &screen, Clock::now() + chrono::hours(2));
        ShowSeat &showSeat1 = show1.addShowSeat(make_unique<ShowSeat>(&seat1, 25000));
        ShowSeat &showSeat2 = show1.addShowSeat(make_unique<ShowSeat>(&seat2, 25000));

        SeatInventoryService inventoryService(Milliseconds(5 * 60 * 1000));
        PaymentService paymentService;
        NotificationService notificationService;
        notificationService.addObserver(make_shared<EmailNotification>());
        notificationService.addObserver(make_shared<SMSNotification>());

        BookingService bookingService(inventoryService, paymentService, notificationService);
        UPIPayment upiPayment;

        vector<ShowSeat *> selectedSeats{&showSeat1, &showSeat2};
        Booking *booking = bookingService.createBooking(
            user1, show1, selectedSeats, upiPayment, "checkout-request-123");

        cout << "\nBooking ID: " << booking->getId() << '\n';
        cout << "Movie: " << show1.getMovieName() << '\n';
        cout << "Amount: Rs " << booking->getTotalAmountInPaise() / 100.0 << '\n';

        switch (booking->getStatus())
        {
        case BookingStatus::CONFIRMED:
            cout << "Status: CONFIRMED\n";
            break;
        case BookingStatus::PAYMENT_FAILED:
            cout << "Status: PAYMENT_FAILED\n";
            break;
        case BookingStatus::PAYMENT_UNKNOWN:
            cout << "Status: PAYMENT_UNKNOWN (reconciliation required)\n";
            break;
        default:
            cout << "Status: Other\n";
        }
    }
    catch (const exception &e)
    {
        cerr << "Booking request failed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
