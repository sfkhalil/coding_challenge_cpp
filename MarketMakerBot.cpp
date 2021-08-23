
#include "MarketMakerBot.h"

#include <thread>
#include <functional>


MarketMakerBot::MarketMakerBot(): balanceETH(10), balanceUSD(2000), keepRunning(true) {
    simulator = DvfSimulator::Create();
}


// can take any custom third party client trading algo
void MarketMakerBot::run(std::function<std::vector<OrderSuggestion>(double, double)> tradingAlgo)
{
    clientTradingAlgo = tradingAlgo;
    balanceDisplayer = std::thread (&MarketMakerBot::displayPortfolio, this);
    
    startTrading();
}

MarketMakerBot::~MarketMakerBot()
{
    keepRunning = false;
    if (balanceDisplayer.joinable())
        balanceDisplayer.join();
    delete simulator;
}



// cancel orders that may damage your portfolio 
void MarketMakerBot::cancelOutlierOrders(const double medianBid, const double medianAsk)
{
    constexpr double sweetSpot = 3.0; // some hacky constant

    auto avgBid = placedBidOrders.lower_bound (OrderSuggestion(medianBid, 1));
    for (auto it = avgBid; (it != placedBidOrders.end() && it->price > (medianBid - sweetSpot)); ++it ) {
        std::cout << RED << "[DEBUG] cancelling bid: " << it->price << NC << std::endl ;
        simulator->CancelOrder(it->orderId);  
    }
    placedBidOrders.erase(avgBid, placedBidOrders.end()); 

    auto avgAsk = placedAskOrders.lower_bound (OrderSuggestion(medianAsk, 1));
    for (auto it = avgAsk; (it != placedAskOrders.end() && it->price < medianAsk + sweetSpot); ++it ) {
        std::cout << RED << "[DEBUG] cancelling ask: " << it->price << NC << std::endl ;
        simulator->CancelOrder(it->orderId);  
    }
    placedAskOrders.erase(avgAsk, placedAskOrders.end()); 

}

/* checks for any existing order gets filled (clause 5). 
            If yes, remove from placedOrders, and update PnL   ****************/
void MarketMakerBot::updatePnL(const double bestBid, const double bestAsk)
{
    std::cout << RED << "[DEBUG] received BestBid " << bestBid << " BestAsk: " << bestAsk << NC << std::endl ;

    /*  when you execute bid, you want to spend $ to buy ETH
        when you execute ask, you want to sell ETH and earn $   */

    // Any bid orders that are above the best bid are filled 
    auto aboveBestBid = placedBidOrders.upper_bound (OrderSuggestion(bestBid, 1));
    for (auto it = placedBidOrders.begin(); it != aboveBestBid; ++it )
    {
        balanceUSD.store(balanceUSD - it->price * it->amount);
        balanceETH.store(balanceETH + it->amount);
    }
    placedBidOrders.erase(placedBidOrders.begin(), aboveBestBid);
    // std::erase_if(placedBidOrders, [bestBid](auto x) { return x.price > bestBid; });

    // Any sell orders that are below the best ask are filled 
    auto belowBestAsk = placedAskOrders.lower_bound (OrderSuggestion(bestAsk, 1));
    for (auto it = belowBestAsk; it != placedAskOrders.end(); ++it ){
        balanceUSD.store(balanceUSD - it->price * it->amount);
        balanceETH.store(balanceETH + it->amount);
    }
    placedAskOrders.erase(placedAskOrders.begin(), belowBestAsk);
    // std::erase_if(placedAskOrders, [bestAsk](auto x) { return x.price < bestAsk; });
}

bool MarketMakerBot::creditCheckFailed (const bool isBid, const double usdPrice) const
{
    constexpr double sweetness = 3.5;  // some hacky constant that works.
    const bool isAsk = !isBid;

    if (isBid && balanceUSD < usdPrice * sweetness) 
        return true;
    else if (isAsk && balanceETH < sweetness) 
        return true;

    return false;
}

// void displayPortfolio(const MarketMakerBot * mmb) const
void MarketMakerBot::displayPortfolio() const
{
    using namespace std::chrono_literals;
    std::string msg;
    msg.reserve(200);
    while(this->keepRunning) {
        msg.clear();
        msg.append(GRN).append("\n____________PORTFOLIO SNAPSHOT____________\n ETH: ") 
            .append(std::to_string(this->balanceETH)).append(" \t USD: ")
            .append(std::to_string(this->balanceUSD))
            .append("\n------------------------------------------\n").append(NC);
        std::cout << msg;
        std::this_thread::sleep_for(PortfolioDisplayThreshold * 1000ms);
    }
}


void MarketMakerBot::startTrading() 
{
    std::map<double, double, std::greater<double>> bids;
    std::map<double, double> asks;
    
    // keep trading until broke :) 
    while (balanceETH > 0.0 || balanceUSD > 0.0 )
    {
        auto freshOrderBook = simulator->GetOrderBook();
        bids.clear(); asks.clear();

        for(auto& level :freshOrderBook)
            level.second > 0.0 
                ? bids[level.first]=level.second
                : asks[level.first]=level.second;
        
        updatePnL(bids.begin()->first, asks.begin()->first);

        // remove potential PortfolioDamagers
        cancelOutlierOrders(bids.begin()->first, asks.begin()->first);
        // cancelOutlierOrders((++(++(++bids.begin())))->first, (++(++(++asks.begin())))->first);


        // now make new suggestions 
        auto newSuggestions = this->clientTradingAlgo(bids.begin()->first, asks.begin()->first);
        
        for (auto& suggestion : newSuggestions)
        {
            const bool isBid = (suggestion.amount > 0.0);
            if (creditCheckFailed(isBid, suggestion.price)) 
                continue;   // bad credit. don't execute suggestion

            auto placedOrder = simulator->PlaceOrder(suggestion.price, suggestion.amount);
            if (placedOrder.has_value())
            { // order succesfully placed
                suggestion.orderId = placedOrder.value();
                isBid   ? placedBidOrders.insert(suggestion)
                        : placedAskOrders.insert(suggestion);
            }
        }
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(RefreshThreshold * 1000ms);
    }
}

