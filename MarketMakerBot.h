#pragma once

#include "DvfSimulator.h"
#include "Common.h"
#include <map>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>


class MarketMakerBot {
    
public:
    MarketMakerBot();

    // can take any custom third party client trading algo
    void run(std::function<std::vector<OrderSuggestion>(double, double)> tradingAlgo);

    ~MarketMakerBot();

protected:
    // cancel orders that may damage your portfolio 
    void cancelOutlierOrders(const double bid, const double ask);

    /* checks for any existing order gets filled (clause 5). 
             If yes, remove from placedOrders, and update PnL   ****************/
    void updatePnL(const double bestBid, const double bestAsk);

    bool creditCheckFailed (const bool isBid, const double price) const;

    // void displayPortfolio(const MarketMakerBot * mmb) const
    void displayPortfolio() const;

    void startTrading();

    std::atomic<double> balanceETH;
    std::atomic<double> balanceUSD;

    std::multiset<OrderSuggestion, std::greater<OrderSuggestion>> placedBidOrders;
    std::multiset<OrderSuggestion> placedAskOrders;

    std::function<std::vector<OrderSuggestion>(double, double)> clientTradingAlgo;

    std::thread orderPlacer;
    std::thread marketDataFetcher;
    std::thread balanceDisplayer;

    bool keepRunning;
    IDvfSimulator * simulator;

};


