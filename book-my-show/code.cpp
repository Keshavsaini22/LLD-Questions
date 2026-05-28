#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include <ctime>
#include <algorithm>
#include <stdexcept>

using namespace std;

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
    CONFIRMED,
    CANCELLED,
    FAILED
};

// =====================================================
// USER
// =====================================================

class User
{
public:
    int id;
    string name;
    string email;
    string city;

    User(int id,
         string name,
         string email,
         string city)
        : id(id),
          name(name),
          email(email),
          city(city) {}
};

// =====================================================
// MOVIE
// =====================================================

class Movie
{
public:
    int id;
    string name;
    string genre;
    string language;

    Movie(int id,
          string name,
          string genre,
          string language)
        : id(id),
          name(name),
          genre(genre),
          language(language) {}
};

// =====================================================
// SEAT
// =====================================================

class Seat
{

    int id;
    int row;
    int col;
    SeatType type;

public:
    Seat(int id,
         int row,
         int col,
         SeatType type)
        : id(id),
          row(row),
          col(col),
          type(type) {}

    int getId() const
    {
        return id;
    }

    SeatType getType() const
    {
        return type;
    }
};

// =====================================================
// SCREEN
// =====================================================

class Screen
{

    int id;
    string name;

    vector<unique_ptr<Seat>> seats;

public:
    Screen(int id,
           string name)
        : id(id),
          name(name) {}

    void addSeat(unique_ptr<Seat> seat)
    {
        seats.push_back(move(seat));
    }

    vector<unique_ptr<Seat>> &getSeats()
    {
        return seats;
    }
};

// =====================================================
// THEATER
// =====================================================

class Theater
{

    int id;
    string name;
    string city;

    vector<unique_ptr<Screen>> screens;

public:
    Theater(int id,
            string name,
            string city)
        : id(id),
          name(name),
          city(city) {}

    void addScreen(unique_ptr<Screen> screen)
    {
        screens.push_back(move(screen));
    }

    vector<unique_ptr<Screen>> &getScreens()
    {
        return screens;
    }

    string getName() const
    {
        return name;
    }
};

// =====================================================
// SHOW SEAT
// =====================================================

class ShowSeat
{

    Seat *seat;

    int price;

    SeatStatus status;

    User *lockedBy;

    time_t lockTime;

    mutex seatMutex;

public:
    ShowSeat(Seat *seat,
             int price)
        : seat(seat),
          price(price),
          status(SeatStatus::AVAILABLE),
          lockedBy(nullptr),
          lockTime(0) {}

    bool lockSeat(User *user)
    {

        lock_guard<mutex> lock(seatMutex);

        if (status != SeatStatus::AVAILABLE)
            return false;

        status = SeatStatus::LOCKED;

        lockedBy = user;

        lockTime = time(nullptr);

        return true;
    }

    bool bookSeat(User *user)
    {

        lock_guard<mutex> lock(seatMutex);

        if (status != SeatStatus::LOCKED ||
            lockedBy != user)
            return false;

        status = SeatStatus::BOOKED;

        return true;
    }

    void releaseSeat()
    {

        lock_guard<mutex> lock(seatMutex);

        status = SeatStatus::AVAILABLE;

        lockedBy = nullptr;
    }

    int getPrice() const
    {
        return price;
    }

    SeatStatus getStatus() const
    {
        return status;
    }

    int getSeatId() const
    {
        return seat->getId();
    }
};

// =====================================================
// SHOW
// =====================================================

class Show
{

    int id;

    Movie *movie;

    Screen *screen;

    time_t startTime;

    vector<unique_ptr<ShowSeat>> seats;

public:
    Show(int id,
         Movie *movie,
         Screen *screen,
         time_t startTime)
        : id(id),
          movie(movie),
          screen(screen),
          startTime(startTime) {}

    void addShowSeat(unique_ptr<ShowSeat> seat)
    {
        seats.push_back(move(seat));
    }

    vector<unique_ptr<ShowSeat>> &getSeats()
    {
        return seats;
    }

    string getMovieName() const
    {
        return movie->name;
    }
};

// =====================================================
// BOOKING
// =====================================================

class Booking
{

public:
    int id;

    User *user;

    Show *show;

    vector<ShowSeat *> bookedSeats;

    int totalAmount;

    BookingStatus status;

    Booking(int id,
            User *user,
            Show *show,
            vector<ShowSeat *> bookedSeats,
            int totalAmount)
        : id(id),
          user(user),
          show(show),
          bookedSeats(bookedSeats),
          totalAmount(totalAmount),
          status(BookingStatus::CREATED) {}
};

// =====================================================
// PAYMENT STRATEGY
// =====================================================

class PaymentStrategy
{
public:
    virtual ~PaymentStrategy() = default;

    virtual bool pay(int amount) = 0;
};

class UPIPayment : public PaymentStrategy
{
public:
    bool pay(int amount) override
    {

        cout << "UPI Payment of Rs "
             << amount
             << " successful\n";

        return true;
    }
};

class CardPayment : public PaymentStrategy
{
public:
    bool pay(int amount) override
    {

        cout << "Card Payment of Rs "
             << amount
             << " successful\n";

        return true;
    }
};

// =====================================================
// NOTIFICATION OBSERVER
// =====================================================

class IObserver
{
public:
    virtual ~IObserver() = default;

    virtual void notify(User *user,
                        Booking *booking) = 0;
};

class EmailNotification : public IObserver
{
public:
    void notify(User *user,
                Booking *booking) override
    {

        cout << "EMAIL sent to "
             << user->email
             << " for booking id "
             << booking->id
             << endl;
    }
};

class SMSNotification : public IObserver
{
public:
    void notify(User *user,
                Booking *booking) override
    {

        cout << "SMS sent to "
             << user->name
             << " for booking id "
             << booking->id
             << endl;
    }
};

// =====================================================
// NOTIFICATION SERVICE
// =====================================================

class NotificationService
{

    vector<shared_ptr<IObserver>> observers;

public:
    void addObserver(shared_ptr<IObserver> observer)
    {
        observers.push_back(observer);
    }

    void notify(User *user,
                Booking *booking)
    {

        for (auto &observer : observers)
        {
            observer->notify(user, booking);
        }
    }
};

// =====================================================
// PAYMENT SERVICE
// =====================================================

class PaymentService
{
public:
    bool processPayment(int amount,
                        PaymentStrategy *strategy)
    {

        return strategy->pay(amount);
    }
};

// =====================================================
// BOOKING SERVICE
// =====================================================

class BookingService
{

    PaymentService paymentService;

    NotificationService notificationService;

    int bookingCounter = 1;

public:
    BookingService()
    {

        notificationService.addObserver(
            make_shared<EmailNotification>());

        notificationService.addObserver(
            make_shared<SMSNotification>());
    }

    Booking *createBooking(
        User *user,
        Show *show,
        vector<ShowSeat *> selectedSeats,
        PaymentStrategy *strategy)
    {

        // STEP 1: LOCK SEATS

        for (auto seat : selectedSeats)
        {

            bool success = seat->lockSeat(user);

            if (!success)
            {

                releaseSeats(selectedSeats);

                throw runtime_error(
                    "Seat already booked");
            }
        }

        // STEP 2: CALCULATE TOTAL

        int totalAmount = 0;

        for (auto seat : selectedSeats)
            totalAmount += seat->getPrice();

        // STEP 3: PAYMENT

        bool paymentSuccess =
            paymentService.processPayment(
                totalAmount,
                strategy);

        // STEP 4: PAYMENT FAILED

        if (!paymentSuccess)
        {

            releaseSeats(selectedSeats);

            throw runtime_error(
                "Payment Failed");
        }

        // STEP 5: BOOK SEATS

        for (auto seat : selectedSeats)
            seat->bookSeat(user);

        // STEP 6: CREATE BOOKING

        Booking *booking = new Booking(
            bookingCounter++,
            user,
            show,
            selectedSeats,
            totalAmount);

        booking->status =
            BookingStatus::CONFIRMED;

        // STEP 7: SEND NOTIFICATION

        notificationService.notify(
            user,
            booking);

        return booking;
    }

    void releaseSeats(
        vector<ShowSeat *> seats)
    {

        for (auto seat : seats)
            seat->releaseSeat();
    }
};

// =====================================================
// MAIN
// =====================================================

int main()
{

    // =================================================
    // USERS
    // =================================================

    User user1(
        1,
        "Alpha",
        "alpha@gmail.com",
        "Mumbai");

    // =================================================
    // MOVIE
    // =================================================

    Movie movie1(
        1,
        "Avengers",
        "Action",
        "English");

    // =================================================
    // THEATER + SCREEN
    // =================================================

    auto theater =
        make_unique<Theater>(
            1,
            "PVR Phoenix",
            "Mumbai");

    auto screen =
        make_unique<Screen>(
            1,
            "Audi 1");

    // =================================================
    // CREATE SEATS
    // =================================================

    screen->addSeat(
        make_unique<Seat>(
            1,
            1,
            1,
            SeatType::GOLD));

    screen->addSeat(
        make_unique<Seat>(
            2,
            1,
            2,
            SeatType::GOLD));

    // KEEP RAW POINTER BEFORE MOVING

    Screen *screenPtr = screen.get();

    theater->addScreen(move(screen));

    // =================================================
    // CREATE SHOW
    // =================================================

    Show show1(
        1,
        &movie1,
        screenPtr,
        time(nullptr));

    // =================================================
    // CREATE SHOW SEATS
    // =================================================

    auto &screenSeats =
        screenPtr->getSeats();

    for (auto &seat : screenSeats)
    {

        show1.addShowSeat(
            make_unique<ShowSeat>(
                seat.get(),
                250));
    }

    // =================================================
    // SELECT SEATS
    // =================================================

    vector<ShowSeat *> selectedSeats;

    auto &showSeats = show1.getSeats();

    selectedSeats.push_back(
        showSeats[0].get());

    selectedSeats.push_back(
        showSeats[1].get());

    // =================================================
    // PAYMENT STRATEGY
    // =================================================

    UPIPayment upiPayment;

    // =================================================
    // BOOKING SERVICE
    // =================================================

    BookingService bookingService;

    try
    {

        Booking *booking =
            bookingService.createBooking(
                &user1,
                &show1,
                selectedSeats,
                &upiPayment);

        cout << "\nBooking Successful\n";

        cout << "Booking Id: "
             << booking->id
             << endl;

        cout << "Movie: "
             << booking->show->getMovieName()
             << endl;

        cout << "Total Amount: Rs "
             << booking->totalAmount
             << endl;
    }
    catch (exception &e)
    {

        cout << e.what() << endl;
    }

    return 0;
}
