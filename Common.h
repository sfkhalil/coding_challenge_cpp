#pragma once

#include <string_view>
#include <random>

constexpr short RefreshThreshold = 5; // Every 5 second the bot should refresh the state of the mkt
constexpr short PortfolioDisplayThreshold = 10; // should show overall asset balances every 30 seconds

constexpr std::string_view RED = "\033[1;31m"; // red font
constexpr std::string_view GRN = "\033[1;32m"; // green font
constexpr std::string_view NC  = "\033[0m"   ; // No Color

struct OrderSuggestion
{
    size_t orderId;
    double price;
    double amount;

    OrderSuggestion(double p, double a): price(p), amount(a), orderId(0) {}

    bool operator < (const OrderSuggestion& rhs) const {
        return price < rhs.price;
    }
    bool operator > (const OrderSuggestion& rhs) const {
        return !operator<(rhs);
    }
};

