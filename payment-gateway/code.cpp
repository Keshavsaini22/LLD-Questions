// Online C++ compiler to run C++ program online
#include <iostream>
#include <bits/stdc++.h>
using namespace std;

class Payment{
  float amount;
  int currency;
  string sender;
  string receiver;
  public:
  
  Payment(float amount,int currency,string sender,string receiver){
      this->amount=amount;
      this->currency=currency;
      this->sender=sender;
      this->receiver=receiver;
  }
  
  float getAmount(){
      return amount;
  }
  int getCurrency(){
      return currency;
  }
  string getSender(){
      return sender;
  }
  string getReceiver(){
      return receiver;
  }
};

class RazorpayApi{
  public:
  bool processPaymentByRazorpayApi(Payment* payment){
      cout<<"Processing the payemnt of "<<payment->getSender()<<"  amount "<<payment->getAmount()<<endl;
      return true;
  }
};

class StripeApi{
  public:
  bool processPaymentByStripeApi(Payment* payment){
      cout<<"Processing the payemnt of "<<payment->getSender()<<"  amount "<<payment->getAmount()<<endl;
      return true;
  }
};

class PaymentGateway{
  public:
  virtual ~PaymentGateway()=default;
  virtual bool processPayment(Payment* payment)=0;
  
  virtual bool validatePayment(Payment* payment){
      return payment->getAmount()>0;
  }
  
  virtual bool confirmPayment(Payment* payment){
      return true;
  }
};

class RazorpayPaymentGateway : public PaymentGateway{
    RazorpayApi* api;
    
    public:
    RazorpayPaymentGateway(){
        api=new RazorpayApi();
    }
    
    bool processPayment(Payment* payment) override {
        if(!validatePayment(payment)){
            throw runtime_error("Payment is less than zero");
        }
        
        return api->processPaymentByRazorpayApi(payment);
        
        
    }
};

class StripeGateway : public PaymentGateway{
    StripeApi* api;
    
    public:
    StripeGateway(){
        api=new StripeApi();
    }
    
    bool processPayment(Payment* payment) override {
        if(!validatePayment(payment)){
            throw runtime_error("Payment is less than zero");
        }
        
        return api->processPaymentByStripeApi(payment);
 }
};

enum class Gateway{
    RAZORPAY,
    STRIPE,
};

class GatewayFactory{
    GatewayFactory(){
        
    }
    
    public:
    
    static GatewayFactory& getInstance(){
        static GatewayFactory instance;
        return instance;
    }
    
    PaymentGateway* createGateway(Gateway gateway){
        switch(gateway){
            case Gateway::RAZORPAY :
            return new RazorpayPaymentGateway();
            break;
            
            case Gateway:: STRIPE :
            return new StripeGateway();
            break;
            
            default:
            throw runtime_error("Gateway does not exist");
            
        }
    }
};

class PaymentProxy: public PaymentGateway{
  PaymentGateway* gateway;
  int retry;
  public:
  
  PaymentProxy(PaymentGateway* gateway,int retry){
      this->gateway=gateway;
      this->retry=retry;
  }
  
  bool processPayment(Payment* payment) override {
      for(int i=1;i<=retry;i++){
          if(gateway->processPayment(payment)){
              return true;
          }
      }
      
      cout<<"Max retry has been reached for the payment processing"<<endl;
      
      return false;
 }
  
};

class PaymentService{
    
    public:
    bool processPayment(Gateway type,Payment* payment){
        PaymentGateway* gateway= GatewayFactory::getInstance().createGateway(type);
        return gateway->processPayment(payment);
    }
};

int main() {
    Payment payment(500,1,"keshav","chetan");

    PaymentService service;

    service.processPayment(
        Gateway::RAZORPAY,
        &payment
    );

    return 0;
}

//POrt and adapter for all kinds of payments
//Different different strategy for card,upi etc etc
//Enternal apis
//My port and adapter
//factory
//service
// -----------------------------------------------------------------------------------------------------------------------------

#include <iostream>
#include <memory>
#include <string>
using namespace std;

//////////////////////////////////////////////////////////
// ENUMS
//////////////////////////////////////////////////////////

enum class Currency {
    INR,
    USD
};

enum class PaymentStatus {
    INITIATED,
    SUCCESS,
    FAILED
};

enum class GatewayType {
    RAZORPAY,
    STRIPE
};

//////////////////////////////////////////////////////////
// PAYMENT REQUEST + RESULT
//////////////////////////////////////////////////////////

class PaymentRequest {
private:
    string paymentId;
    string sender;
    string receiver;
    double amount;
    Currency currency;

public:
    PaymentRequest(
        string paymentId,
        string sender,
        string receiver,
        double amount,
        Currency currency
    )
        : paymentId(paymentId),
          sender(sender),
          receiver(receiver),
          amount(amount),
          currency(currency) {}

    string getPaymentId() const { return paymentId; }
    string getSender() const { return sender; }
    string getReceiver() const { return receiver; }
    double getAmount() const { return amount; }
    Currency getCurrency() const { return currency; }
};

class PaymentResult {
public:
    bool success;
    string transactionId;
    string errorMessage;

    PaymentResult(
        bool success,
        string transactionId = "",
        string errorMessage = ""
    )
        : success(success),
          transactionId(transactionId),
          errorMessage(errorMessage) {}
};

//////////////////////////////////////////////////////////
// PAYMENT METHOD STRATEGY
//////////////////////////////////////////////////////////

class PaymentMethod {
public:
    virtual ~PaymentMethod() = default;
    virtual bool pay(const PaymentRequest& request) = 0;
};

class UpiPayment : public PaymentMethod {
public:
    bool pay(const PaymentRequest& request) override {
        cout << "[UPI] Validating UPI payment for "
             << request.getSender() << endl;
        return true;
    }
};

class CardPayment : public PaymentMethod {
public:
    bool pay(const PaymentRequest& request) override {
        cout << "[CARD] Validating card details for "
             << request.getSender() << endl;
        return true;
    }
};

class NetBankingPayment : public PaymentMethod {
public:
    bool pay(const PaymentRequest& request) override {
        cout << "[NETBANKING] Redirecting to bank for "
             << request.getSender() << endl;
        return true;
    }
};

//////////////////////////////////////////////////////////
// EXTERNAL APIS (ADAPTER TARGETS)
//////////////////////////////////////////////////////////

class RazorpayApi {
public:
    bool makePayment(double amount) {
        cout << "[RazorpayAPI] Processing payment: " << amount << endl;
        return true;
    }
};

class StripeApi {
public:
    bool charge(double amount) {
        cout << "[StripeAPI] Charging amount: " << amount << endl;
        return true;
    }
};

//////////////////////////////////////////////////////////
// PAYMENT GATEWAY TEMPLATE METHOD
//////////////////////////////////////////////////////////

class PaymentGateway {
public:
    virtual ~PaymentGateway() = default;

    PaymentResult processPayment(const PaymentRequest& request) {
        if (!validate(request)) {
            return PaymentResult(false, "", "Validation failed");
        }

        if (!initiate(request)) {
            return PaymentResult(false, "", "Gateway initiation failed");
        }

        if (!confirm(request)) {
            return PaymentResult(false, "", "Confirmation failed");
        }

        return PaymentResult(true, "TXN_" + request.getPaymentId());
    }

protected:
    virtual bool initiate(const PaymentRequest& request) = 0;

    virtual bool validate(const PaymentRequest& request) {
        return request.getAmount() > 0;
    }

    virtual bool confirm(const PaymentRequest&) {
        return true;
    }
};

//////////////////////////////////////////////////////////
// GATEWAY IMPLEMENTATIONS
//////////////////////////////////////////////////////////

class RazorpayGateway : public PaymentGateway {
private:
    unique_ptr<RazorpayApi> api;

public:
    RazorpayGateway() {
        api = make_unique<RazorpayApi>();
    }

protected:
    bool initiate(const PaymentRequest& request) override {
        return api->makePayment(request.getAmount());
    }
};

class StripeGateway : public PaymentGateway {
private:
    unique_ptr<StripeApi> api;

public:
    StripeGateway() {
        api = make_unique<StripeApi>();
    }

protected:
    bool initiate(const PaymentRequest& request) override {
        return api->charge(request.getAmount());
    }
};

//////////////////////////////////////////////////////////
// PROXY (RETRY LOGIC)
//////////////////////////////////////////////////////////

class PaymentGatewayProxy {
private:
    unique_ptr<PaymentGateway> gateway;
    int retries;

public:
    PaymentGatewayProxy(unique_ptr<PaymentGateway> gateway, int retries = 3)
        : gateway(move(gateway)), retries(retries) {}

    PaymentResult processPayment(const PaymentRequest& request) {
        for (int i = 1; i <= retries; i++) {
            cout << "[Retry Attempt] " << i << endl;

            PaymentResult result = gateway->processPayment(request);

            if (result.success) {
                return result;
            }
        }

        return PaymentResult(false, "", "Max retries exceeded");
    }
};

//////////////////////////////////////////////////////////
// FACTORY
//////////////////////////////////////////////////////////

class GatewayFactory {
public:
    static unique_ptr<PaymentGateway> createGateway(GatewayType type) {
        switch (type) {
            case GatewayType::RAZORPAY:
                return make_unique<RazorpayGateway>();

            case GatewayType::STRIPE:
                return make_unique<StripeGateway>();

            default:
                throw runtime_error("Unsupported gateway");
        }
    }
};

//////////////////////////////////////////////////////////
// PAYMENT SERVICE
//////////////////////////////////////////////////////////

class PaymentService {
public:
    PaymentResult processPayment(
        PaymentMethod& method,
        GatewayType gatewayType,
        const PaymentRequest& request
    ) {
        if (!method.pay(request)) {
            return PaymentResult(false, "", "Payment method failed");
        }

        auto gateway = GatewayFactory::createGateway(gatewayType);
        PaymentGatewayProxy proxy(move(gateway), 3);

        return proxy.processPayment(request);
    }
};

//////////////////////////////////////////////////////////
// MAIN
//////////////////////////////////////////////////////////

int main() {
    PaymentRequest request(
        "1001",
        "Aditya",
        "Shubham",
        500,
        Currency::INR
    );

    UpiPayment upiMethod;
    PaymentService service;

    PaymentResult result = service.processPayment(
        upiMethod,
        GatewayType::RAZORPAY,
        request
    );

    if (result.success) {
        cout << "Payment Success. Txn ID: "
             << result.transactionId << endl;
    } else {
        cout << "Payment Failed: "
             << result.errorMessage << endl;
    }

    return 0;
}