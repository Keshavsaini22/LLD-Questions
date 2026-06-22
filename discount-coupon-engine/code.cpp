// User
// Product
// CartItem
// Cart
//     |
//     +-- User
//     +-- CartItems
// Coupon
//     |
//     +-- DiscountStrategy
//     +-- EligibilityRules  ->Can this user/cart use this coupon? 
//     +-- ApplicabilityRules->Where should discount be applied?  Examples:iPhone Coupon-IPHONE10Applicable only on: iPhone
//     +-- StackingRule -> Can two coupons coexist?
// CouponManager
// CouponSelectionStrategy -> Among valid coupons,which coupons should be applied?
//     |
//     +-- MaximumSavingsStrategy
// CouponEngine
// DiscountResult

// Why both are needed? ApplicabilityRules && EligibilityRule
// Coupon:
// IPHONE10
// Eligibility:
// Cart Value > 5000
// Applicability:
// Only iPhone products
// These are completely different concerns.

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

using namespace std;

class Product {
private:
    int productId;
    string name;
    string category;
    double price;

public:
    Product(int productId, const string& name, const string& category, double price)
        : productId(productId), name(name), category(category), price(price) {}

    int getProductId() const {
        return productId;
    }

    string getCategory() const {
        return category;
    }

    double getPrice() const {
        return price;
    }
};

class CartItem {
private:
    shared_ptr<Product> product;
    int quantity;

public:
    CartItem(shared_ptr<Product> product, int quantity)
        : product(product), quantity(quantity) {}

    shared_ptr<Product> getProduct() const {
        return product;
    }

    int getQuantity() const {
        return quantity;
    }

    double getTotalPrice() const {
        return product->getPrice() * quantity;
    }
};

class User {
private:
    int userId;
    string name;

public:
    User(int userId, const string& name)
        : userId(userId), name(name) {}

    int getUserId() const {
        return userId;
    }
};

class Cart {
private:
    shared_ptr<User> user;
    vector<CartItem> items;

public:
    Cart(shared_ptr<User> user)
        : user(user) {}

    void addItem(const CartItem& item) {
        items.push_back(item);
    }

    const vector<CartItem>& getItems() const {
        return items;
    }

    shared_ptr<User> getUser() const {
        return user;
    }

    double getCartValue() const {
        double total = 0;
        for (const auto& item : items) {
            total += item.getTotalPrice();
        }
        return total;
    }
};

class EligibilityRule {
public:
    virtual bool isEligible(const Cart& cart) = 0;
    virtual ~EligibilityRule() = default;
};

class DiscountStrategy {
public:
    virtual double calculateDiscount(double amount) = 0;
    virtual ~DiscountStrategy() = default;
};

class FlatDiscountStrategy : public DiscountStrategy {
private:
    double flatAmount;

public:
    FlatDiscountStrategy(double flatAmount)
        : flatAmount(flatAmount) {}

    double calculateDiscount(double amount) override {
        return min(amount, flatAmount);
    }
};

class PercentageDiscountStrategy : public DiscountStrategy {
private:
    double percentage;

public:
    PercentageDiscountStrategy(double percentage)
        : percentage(percentage) {}

    double calculateDiscount(double amount) override {
        return amount * percentage / 100.0;
    }
};

class PercentageWithCapStrategy : public DiscountStrategy {
private:
    double percentage;
    double capAmount;

public:
    PercentageWithCapStrategy(double percentage, double capAmount)
        : percentage(percentage), capAmount(capAmount) {}

    double calculateDiscount(double amount) override {
        return min(capAmount, amount * percentage / 100.0);
    }
};

class MinCartValueRule : public EligibilityRule {
private:
    double minValue;

public:
    MinCartValueRule(double minValue)
        : minValue(minValue) {}

    bool isEligible(const Cart& cart) override {
        return cart.getCartValue() >= minValue;
    }
};

class NewUserRule : public EligibilityRule {
public:
    bool isEligible(const Cart& cart) override {
        return true;
    }
};

class ApplicabilityRule {
public:
    virtual vector<CartItem> getApplicableItems(const Cart& cart) = 0;
    virtual ~ApplicabilityRule() = default;
};

class CartLevelApplicability : public ApplicabilityRule {
public:
    vector<CartItem> getApplicableItems(const Cart& cart) override {
        return cart.getItems();
    }
};

class ProductApplicability : public ApplicabilityRule {
private:
    unordered_set<int> productIds;

public:
    ProductApplicability(const vector<int>& ids) {
        for (auto id : ids) {
            productIds.insert(id);
        }
    }

    vector<CartItem> getApplicableItems(const Cart& cart) override {
        vector<CartItem> result;
        for (const auto& item : cart.getItems()) {
            if (productIds.count(item.getProduct()->getProductId())) {
                result.push_back(item);
            }
        }
        return result;
    }
};

// Before coding, one thing.
// I would slightly change the stacking design.
// Your requirement says:
// One coupon can be applied on top of another coupon.
// One coupon cannot be applied on top of another coupon.
// This is actually a property of the coupon itself.
// For interview purposes, I'd start with:
// bool stackable;
// inside Coupon.
// Later if interviewer asks:
// Bank coupons can stack only with loyalty coupons
// then evolve to:
// StackingRule
// For now let's keep it simple and extensible.

class Coupon {
private:
    string code;
    string description;
    bool active;
    bool stackable;
    shared_ptr<DiscountStrategy> discountStrategy;
    vector<shared_ptr<EligibilityRule>> eligibilityRules;
    shared_ptr<ApplicabilityRule> applicabilityRule;

public:
    Coupon(const string& code,
           const string& description,
           bool stackable,
           shared_ptr<DiscountStrategy> discountStrategy,
           shared_ptr<ApplicabilityRule> applicabilityRule)
        : code(code),
          description(description),
          active(true),
          stackable(stackable),
          discountStrategy(discountStrategy),
          applicabilityRule(applicabilityRule) {}

    string getCode() const {
        return code;
    }

    bool isActive() const {
        return active;
    }

    bool isStackable() const {
        return stackable;
    }

    void deactivate() {
        active = false;
    }

    void addEligibilityRule(shared_ptr<EligibilityRule> rule) {
        eligibilityRules.push_back(rule);
    }

    bool isEligible(const Cart& cart) const {
        for (const auto& rule : eligibilityRules) {
            if (!rule->isEligible(cart)) {
                return false;
            }
        }
        return true;
    }

    double calculateDiscount(const Cart& cart) const {
        auto applicableItems = applicabilityRule->getApplicableItems(cart);
        double amount = 0;
        for (const auto& item : applicableItems) {
            amount += item.getTotalPrice();
        }
        return discountStrategy->calculateDiscount(amount);
    }
};

class StackingRule {
public:
    virtual bool canStack(const Coupon& existingCoupon, const Coupon& incomingCoupon) = 0;
    virtual ~StackingRule() = default;
};

class DefaultStackingRule : public StackingRule {
public:
    bool canStack(const Coupon& existingCoupon, const Coupon& incomingCoupon) override {
        if (!existingCoupon.isStackable()) {
            return false;
        }
        if (!incomingCoupon.isStackable()) {
            return false;
        }
        return true;
    }
};

class CouponManager {
private:
    unordered_map<string, shared_ptr<Coupon>> coupons;

public:
    void addCoupon(shared_ptr<Coupon> coupon) {
        coupons[coupon->getCode()] = coupon;
    }

    void removeCoupon(const string& code) {
        coupons.erase(code);
    }

    shared_ptr<Coupon> getCoupon(const string& code) {
        if (coupons.count(code)) {
            return coupons[code];
        }
        return nullptr;
    }

    vector<shared_ptr<Coupon>> getAllCoupons() {
        vector<shared_ptr<Coupon>> result;
        for (auto& [code, coupon] : coupons) {
            if (coupon->isActive()) {
                result.push_back(coupon);
            }
        }
        return result;
    }
};

class DiscountResult {
private:
    double originalAmount;
    double finalAmount;
    double totalDiscount;
    vector<string> appliedCoupons;

public:
    DiscountResult(double originalAmount,
                   double finalAmount,
                   double totalDiscount,
                   const vector<string>& appliedCoupons)
        : originalAmount(originalAmount),
          finalAmount(finalAmount),
          totalDiscount(totalDiscount),
          appliedCoupons(appliedCoupons) {}

    void print() const {
        cout << "\nOriginal Amount : " << originalAmount << endl;
        cout << "Total Discount : " << totalDiscount << endl;
        cout << "Final Amount : " << finalAmount << endl;
        cout << "Applied Coupons : ";
        for (const auto& code : appliedCoupons) {
            cout << code << " ";
        }
        cout << endl;
    }
};

class CouponSelectionStrategy {
public:
    virtual vector<shared_ptr<Coupon>> selectCoupons(
        const Cart& cart,
        const vector<shared_ptr<Coupon>>& validCoupons) = 0;
    virtual ~CouponSelectionStrategy() = default;
};

// Take all valid stackable coupons.
// If a non-stackable coupon exists,
// compare it against stackable combination.
class MaximumSavingsStrategy : public CouponSelectionStrategy {
private:
    shared_ptr<StackingRule> stackingRule;

public:
    MaximumSavingsStrategy(shared_ptr<StackingRule> stackingRule)
        : stackingRule(stackingRule) {}

    vector<shared_ptr<Coupon>> selectCoupons(
        const Cart& cart,
        const vector<shared_ptr<Coupon>>& validCoupons) override {
        vector<shared_ptr<Coupon>> selected;
        for (auto& coupon : validCoupons) {
            bool canAdd = true;
            for (auto& existing : selected) {
                if (!stackingRule->canStack(*existing, *coupon)) {
                    canAdd = false;
                    break;
                }
            }
            if (canAdd) {
                selected.push_back(coupon);
            }
        }
        return selected;
    }
};

// class MaximumSavingsStrategy
//     : public CouponSelectionStrategy
// {
// private:
//     shared_ptr<StackingRule> stackingRule;
//
//     bool canAddCoupon(const vector<shared_ptr<Coupon>>& selected, shared_ptr<Coupon> coupon) {
//         for (auto& existing : selected) {
//             if (!stackingRule->canStack(*existing, *coupon)) {
//                 return false;
//             }
//         }
//         return true;
//     }
//
// public:
//     MaximumSavingsStrategy(shared_ptr<StackingRule> stackingRule)
//         : stackingRule(stackingRule) {}
//
//     vector<shared_ptr<Coupon>> selectCoupons(
//         const Cart& cart,
//         const vector<shared_ptr<Coupon>>& validCoupons) override {
//         vector<shared_ptr<Coupon>> bestCombination;
//         double bestDiscount = 0;
//         int n = validCoupons.size();
//
//         for (int mask = 0; mask < (1 << n); mask++) {
//             vector<shared_ptr<Coupon>> current;
//             bool valid = true;
//
//             for (int i = 0; i < n; i++) {
//                 if (!(mask & (1 << i))) {
//                     continue;
//                 }
//                 if (!canAddCoupon(current, validCoupons[i])) {
//                     valid = false;
//                     break;
//                 }
//                 current.push_back(validCoupons[i]);
//             }
//
//             if (!valid) {
//                 continue;
//             }
//
//             double totalDiscount = 0;
//             for (auto& coupon : current) {
//                 totalDiscount += coupon->calculateDiscount(cart);
//             }
//
//             if (totalDiscount > bestDiscount) {
//                 bestDiscount = totalDiscount;
//                 bestCombination = current;
//             }
//         }
//         return bestCombination;
//     }
// };

class CouponEngine {
private:
    CouponManager& couponManager;
    shared_ptr<CouponSelectionStrategy> selectionStrategy;

public:
    CouponEngine(CouponManager& couponManager,
                 shared_ptr<CouponSelectionStrategy> selectionStrategy)
        : couponManager(couponManager),
          selectionStrategy(selectionStrategy) {}

    DiscountResult calculateFinalPrice(const Cart& cart) {
        double originalAmount = cart.getCartValue();
        auto allCoupons = couponManager.getAllCoupons();

        vector<shared_ptr<Coupon>> validCoupons;
        for (auto& coupon : allCoupons) { //eligible coupons find kr rhe h
            if (coupon->isEligible(cart)) {
                validCoupons.push_back(coupon);
            }
        }

        auto selectedCoupons = selectionStrategy->selectCoupons(cart, validCoupons);

        double totalDiscount = 0;
        vector<string> appliedCoupons;

        for (auto& coupon : selectedCoupons) {
            double discount = coupon->calculateDiscount(cart);
            totalDiscount += discount;
            appliedCoupons.push_back(coupon->getCode());
        }

        double finalAmount = max(0.0, originalAmount - totalDiscount);

        return DiscountResult(
            originalAmount,
            finalAmount,
            totalDiscount,
            appliedCoupons);
    }
};

int main() {
    auto user = make_shared<User>(1, "Alpha");
    Cart cart(user);

    auto iphone = make_shared<Product>(1, "iPhone", "Mobile", 100000);
    auto milk = make_shared<Product>(2, "Milk", "Grocery", 100);
    auto bread = make_shared<Product>(3, "Bread", "Grocery", 50);

    cart.addItem(CartItem(iphone, 1));
    cart.addItem(CartItem(milk, 2));
    cart.addItem(CartItem(bread, 3));

    CouponManager couponManager;

    auto coupon1 = make_shared<Coupon>(
        "SAVE10",
        "10 Percent Off",
        true,
        make_shared<PercentageDiscountStrategy>(10),
        make_shared<CartLevelApplicability>()
    );
    coupon1->addEligibilityRule(make_shared<MinCartValueRule>(500));
    couponManager.addCoupon(coupon1);

    auto coupon2 = make_shared<Coupon>(
        "FLAT500",
        "Flat 500 Off",
        false,
        make_shared<FlatDiscountStrategy>(500),
        make_shared<CartLevelApplicability>()
    );
    coupon2->addEligibilityRule(make_shared<MinCartValueRule>(1000));
    couponManager.addCoupon(coupon2);

    auto coupon3 = make_shared<Coupon>(
        "IPHONE5",
        "5 Percent Off iPhone",
        true,
        make_shared<PercentageDiscountStrategy>(5),
        make_shared<ProductApplicability>(vector<int>{1})
    );
    couponManager.addCoupon(coupon3);

    auto stackingRule = make_shared<DefaultStackingRule>();
    auto selectionStrategy = make_shared<MaximumSavingsStrategy>(stackingRule);
    CouponEngine engine(couponManager, selectionStrategy);

    auto result = engine.calculateFinalPrice(cart);
    result.print();

    return 0;
}