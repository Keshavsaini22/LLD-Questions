#include<bits/stdc++.h>
#include <iostream>
#include <ctime>

using namespace std;

enum class SeatType{
    SILVER,
    GOLD,
    PREMIUM
};

//City, theater, screens, seats, shows
class Seat{
    SeatType type;
    int number;
    int row;
    int col;

public:
    Seat(SeatType type, int number,int row,int col){
        this->number = number;
        this->type = type;
        this->row = row;
        this->col = col;
    }

    int getNumber(){
        return number;
    }
};

class User{
public:
    string name;
    int id;
    int age;
    string city;

    User(string name,int id,int age,string city){
        this->name = name;
        this->id = id;
        this->age = age;
        this->city = city;
    }
};

class Screen{
    vector<Seat*> seats;
    string name;

public:

    Screen(vector<Seat*> seats,string name){
        this->seats = seats;
        this->name = name;
    }

    void addSeat(Seat* seat){
        seats.push_back(seat);
    }
};

class Theter{
    string name;
    string city;
    vector<Screen*> screens;

public:

    Theter(string name,string city, vector<Screen*> screens){
        this->name = name;
        this->city = city;
        this->screens = screens;
    }

    void addScreen(Screen* screen){
        screens.push_back(screen);
    }
};

class ShowSeat: public Seat{
    int user;
    int price;

public:
    ShowSeat(SeatType type, int number,int row,int col,int user,int price)
        : Seat(type, number, row, col){
        this->user = user;
        this->price = price;
    }
};

class Show{
    Theter* theter;
    int id;
    Screen* screen;
    vector<ShowSeat*> seats;
    time_t date;

public:
    Show(Theter* theter, Screen * screen, vector<ShowSeat*> seats,time_t date,int id){
        this->theter = theter;
        this->screen = screen;
        this->seats = seats;
        this->date = date;
        this->id = id;
    }
};

class Booking{
public:
    int userId;
    int theterId;
    int showId;
    int id;
    vector<ShowSeat*> seats;

    Booking(int userId,int theterId,int showId,vector<ShowSeat*> seats,int id){
        this->userId = userId;
        this->theterId = theterId;
        this->showId = showId;
        this->seats = seats;
        this->id = id;
    }
};

class PaymentStrategy{
public:
    virtual void pay(int amount) = 0;
};

class UPIPaymentStrategy: public PaymentStrategy{
public:

    void pay(int amount) override{
        cout<<"Paying the amount of "<<amount<<" using UPI";
    }
};

class CardPaymentStrategy: public PaymentStrategy{
public:

    void pay(int amount) override{
        cout<<"Paying the amount of "<<amount<<" using card";
    }
};

class NetBankingPaymentStrategy: public PaymentStrategy{
public:

    void pay(int amount) override{
        cout<<"Paying the amount of "<<amount<<" using net banking";
    }
};

class IObserver{
public:
    virtual void notify(User* user,Booking* booking) = 0;
};

class EmailNotification: public IObserver{
public:
    void notify(User* user,Booking* booking) override{
        cout<<"Sending the email to "<<user->name<<" for booking "<<booking->id;
    }
};

class SMSNotification: public IObserver{
public:
    void notify(User* user,Booking* booking) override{
        cout<<"Sending the SMS to "<<user->name<<" for booking "<<booking->id;
    }
};

class NotificationObserable{
    vector<IObserver*> observers;

public:

    NotificationObserable(){
    }

    void addObserver(IObserver* observer){
        observers.push_back(observer);
    }

    void notify(User* user, Booking* booking){
        for(int i=0;i<observers.size();i++){
            observers[i]->notify(user,booking);
        }
    }
};