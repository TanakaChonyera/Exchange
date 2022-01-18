#include <algorithm>
#include <exception>

#include "exchange.h"

void Exchange::MakeDeposit(const std::string &username, 
                           const std::string &asset, int amount)
{
    portfolios[username][asset] += amount;
}

//===========================================================================================================================
//===========================================================================================================================

void Exchange::PrintUserPortfolios(std::ostream &os) const
{
    os << "User Portfolios (in alphabetical order):" << std::endl;
    for (auto name_itr = portfolios.begin(); name_itr != portfolios.end(); name_itr++) 
    {
        os << name_itr->first << "'s Portfolio: ";
        for (auto asset_itr = name_itr->second.begin(); asset_itr != name_itr->second.end(); asset_itr++) 
        {
            if (asset_itr->second > 0)
            {
                os << asset_itr->second << ' ' << asset_itr->first << ", "; 
            }
        }
        os << std::endl;
    }
}

//===========================================================================================================================
//===========================================================================================================================

bool Exchange::MakeWithdrawal(const std::string &username, 
                              const std::string &asset, 
                              int amount)
{
    auto name_itr = portfolios.find(username);
    if (name_itr != portfolios.end()) 
    {
        if (name_itr->second.find(asset) != name_itr->second.end())
        {
            if (portfolios[username][asset] - amount >= 0)
            {
                int withdraw_amount = -amount;
                MakeDeposit(username, asset, withdraw_amount);
                return true;
            }
        } 
        else
        {
            return false;
        }
    }
    return false;
}

//===========================================================================================================================
//===========================================================================================================================

void Exchange::AddOrder_Buy(const Order& order) 
{
    bool add_to_market = true;
    // mo: modified order
    bool add_to_market_mo = false;
    Order buy_order_to_add;
    buy_order_to_add.amount = order.amount;
    // Iterate through each persons key (their name) value (their orders) pair
    for (auto map_itr = open_orders.begin(); map_itr != open_orders.end(); map_itr++) 
    {
        int orders_vec_size = static_cast<int>(map_itr->second.size());
        // Loops through orders for particular person
        for (int i = 0; i < orders_vec_size; ++i)
        {
            Order& open_order = map_itr->second.at(i);
            // Finds valid Sell order
            if (open_order.side == "Sell" && open_order.asset == order.asset && open_order.price <= order.price)
            {
                Order open_sell_order = open_order;
                Trade trade(order.username, open_sell_order.username, order.asset, open_sell_order.amount, open_sell_order.price); 
                Trade record = trade;
                // Seller sells all thier stock, Buyer's order complete 
                if (open_sell_order.amount == order.amount)
                {
                    // Buyer obtains all of Sell order stock
                    portfolios[trade.buyer_username][trade.asset] += order.amount;
                    // Credit Seller total sold 
                    portfolios[trade.seller_username]["USD"] += trade.amount * order.price; 
                    // Add Sell order to filled orders for seller
                    filled_orders[trade.seller_username].push_back(open_sell_order);
                    // Remove Sell order from open market
                    open_order.side = "DELETED";
                    // Add Buy order to filled orders for Buyer
                    filled_orders[trade.buyer_username].push_back(order);
                    // Add record to Trade History
                    record.price = order.price; 
                    trade_history.push_back(record);
                    // Set Flag
                    add_to_market = false;
                }
                // Seller sells part of their stock, Buyer's order complete 
                else if (open_sell_order.amount > order.amount)
                {
                    Order partial_sell_order = open_sell_order; 
                    partial_sell_order.amount = order.amount; 
                    // Buyer obtains all their required stock
                    portfolios[trade.buyer_username][trade.asset] += order.amount;
                    // Credit Seller total sold 
                    portfolios[trade.seller_username]["USD"] += order.amount * order.price; 
                    // Modify Sell order 
                    open_order.amount -= order.amount;
                    // Add partial Sell order to Sellers filled orders 
                    partial_sell_order.price = order.price; // Adjust partial sell price
                    filled_orders[trade.seller_username].push_back(partial_sell_order); 
                    // Add Buy order to filled orders for Buyer
                    filled_orders[trade.buyer_username].push_back(order);
                    // Add record to Trade History
                    record.price = order.price;
                    record.amount = order.amount;
                    trade_history.push_back(record);
                    // Set Flag
                    add_to_market = false;
                }
                // Seller sells part of their stock, Buyer's order incomplete 
                else if (open_sell_order.amount < order.amount)
                {
                    Order partial_buy_order;
                    partial_buy_order.username = trade.buyer_username;
                    partial_buy_order.side = order.side;
                    partial_buy_order.asset = order.asset;
                    partial_buy_order.amount = open_sell_order.amount;
                    partial_buy_order.price = order.price;
                    // Buyer obtains part of their required stock
                    portfolios[trade.buyer_username][trade.asset] += open_sell_order.amount;
                    // Add partial Buy order to filled orders for Buyer
                    filled_orders[trade.buyer_username].push_back(partial_buy_order);
                    // Modify Buy order
                    buy_order_to_add.amount -= open_sell_order.amount;
                    buy_order_to_add.asset = order.asset;
                    buy_order_to_add.price = order.price;
                    buy_order_to_add.side = order.side;
                    buy_order_to_add.username = trade.buyer_username;
                    // Credit Seller total sold 
                    portfolios[trade.seller_username]["USD"] += open_sell_order.amount * order.price; 
                    // Add Sell order to filled orders for seller
                    open_sell_order.price = order.price;
                    filled_orders[trade.seller_username].push_back(open_sell_order); 
                    // Remove Sell order from open market
                    open_order.side = "DELETED";
                    // Add record to Trade History
                    record.price = order.price;
                    trade_history.push_back(record);
                    // Set Flags 
                    add_to_market = false;
                    add_to_market_mo = true;
                }
            }
        }
    }

    if (add_to_market && add_to_market_mo == false)
    {
        // Add order to open market
        open_orders[order.username].push_back(order);
    } else if (add_to_market == false && add_to_market_mo)
    {
        // Add modified order to open market
        open_orders[order.username].push_back(buy_order_to_add);
    }

}
//===========================================================================================================================
//===========================================================================================================================

void Exchange::AddOrder_Sell(const Order& order)
{
    bool add_to_market = true;
    // mo: modified order
    bool add_to_market_mo = false;
    Order sell_order_to_add;
    sell_order_to_add.amount = order.amount;
    // Iterate through each persons key (their name) value (their orders) pair
    for (auto map_itr = open_orders.begin(); map_itr != open_orders.end(); ++map_itr)
    {
        int orders_vec_size = static_cast<int>(map_itr->second.size());
        // Loops through orders for particular person
        for (int i = 0; i < orders_vec_size; ++i)
        {
            Order& open_order = map_itr->second.at(i);
            // Finds valid Buy order
            if (open_order.side == "Buy" && open_order.asset == order.asset && order.price <= open_order.price)
            {
                Order open_buy_order = open_order;
                Trade trade(open_buy_order.username, order.username, order.asset, open_buy_order.amount, open_buy_order.price);
                Trade record = trade;
                // Seller sells all their stock
                if (open_buy_order.amount == order.amount)
                {
                    // Buyer obtains all of Sell order stock
                    portfolios[trade.buyer_username][trade.asset] += order.amount;
                    // Credit Seller total sold 
                    portfolios[trade.seller_username]["USD"] += trade.amount * order.price;
                    // Add Buy order to filled orders for Buyer
                    filled_orders[trade.buyer_username].push_back(open_buy_order);
                    // Remove Buy order from open market
                    open_order.side = "DELETED";
                    // Add Sell order to filled orders for Seller
                    filled_orders[trade.seller_username].push_back(order);
                    // Add record to Trade History
                    record.price = order.price; 
                    trade_history.push_back(record);
                    // Set Flag
                    add_to_market = false;
                }
                else if (open_buy_order.amount > order.amount)
                {
                    Order partial_buy_order = open_buy_order; 
                    partial_buy_order.amount = order.amount; 
                    // Buyer obtains part of their required stock
                    portfolios[trade.buyer_username][trade.asset] += order.amount;
                    // Credit Seller total sold 
                    portfolios[trade.seller_username]["USD"] += order.amount * order.price;
                    // Modify Buy order 
                    open_order.amount -= order.amount; 
                    // Add partial Buy order to Buyers filled orders 
                    partial_buy_order.price = order.price; // Adjust partial buy price
                    filled_orders[trade.buyer_username].push_back(partial_buy_order); 
                    // Add Sell order to filled orders for Seller
                    filled_orders[trade.seller_username].push_back(order);
                    // Add record to Trade History
                    record.price = order.price; 
                    record.amount = order.amount;
                    trade_history.push_back(record);
                    // Set Flag
                    add_to_market = false;
                }
                else if (open_buy_order.amount < order.amount)
                {
                    Order partial_sell_order;
                    partial_sell_order.username = trade.seller_username;
                    partial_sell_order.side = order.side;
                    partial_sell_order.asset = order.asset;
                    partial_sell_order.amount = open_buy_order.amount;
                    partial_sell_order.price = order.price;
                    // Buyer obtains all their required stock
                    portfolios[trade.buyer_username][trade.asset] += open_buy_order.amount;
                    // Add completed Buy order to filled orders for Buyer
                    open_buy_order.price = order.price; // ADDED TO ACCOUNT FOR DIFFERENT PRICES
                    filled_orders[trade.buyer_username].push_back(open_buy_order);
                    // Modify Sell order
                    sell_order_to_add.amount -= open_buy_order.amount;
                    sell_order_to_add.asset = order.asset;  
                    sell_order_to_add.price = order.price;
                    sell_order_to_add.side = order.side;
                    sell_order_to_add.username = trade.seller_username;
                    // Credit Seller total sold 
                    portfolios[trade.seller_username]["USD"] += open_buy_order.amount * order.price;
                    // Add partial Sell order to filled orders for seller
                    filled_orders[trade.seller_username].push_back(partial_sell_order);
                    // Remove Buy order from open market
                    open_order.side = "DELETED";
                    // Add record to Trade History
                    record.price = order.price; 
                    //record.amount = order.amount;
                    trade_history.push_back(record);
                    // Set Flags
                    add_to_market = false;
                    add_to_market_mo = true;
                }
            }
        }
    }
    if (add_to_market && add_to_market_mo == false)
    {
        // Add order to open market
        open_orders[order.username].push_back(order);
    } else if (add_to_market == false && add_to_market_mo)
    {
        // Add modified order to open market
        open_orders[order.username].push_back(sell_order_to_add);
    }
}

//===========================================================================================================================
//===========================================================================================================================

bool Exchange::AddOrder(const Order &order) 
{  
    if (order.side == "Buy")
    {
        // Invalid order 
        if (portfolios[order.username]["USD"] < (order.amount * order.price))
        {
            return false;
        }
        // Order is valid
        Exchange::AddOrder_Buy(order);
        // Remove funds from buyer's portfolio
        portfolios[order.username]["USD"] -= order.amount * order.price;
    }
    else if (order.side == "Sell")
    {
        // Invalid order 
        if (portfolios[order.username][order.asset] < order.amount)
        {
            return false;
        }
        //Order is valid
        Exchange::AddOrder_Sell(order);
        // Remove asset from seller's portforlio
        portfolios[order.username][order.asset] -= order.amount;
        
    } 
    return true;
}

//===========================================================================================================================
//===========================================================================================================================

void Exchange::PrintUsersOrders(std::ostream &os) 
{
    os << "Users Orders (in alphabetical order):" << std::endl;
    for (auto name_itr = portfolios.begin(); name_itr != portfolios.end(); name_itr++) {
        os << name_itr->first << "'s Open Orders (in chronological order):" << std::endl;
        std::copy_if(open_orders[name_itr->first].begin(), open_orders[name_itr->first].end(), std::ostream_iterator<Order>(os, "\n"), [] (Order& order)
                                                                                                                                          { 
                                                                                                                                              if (order.side == "Buy" || order.side == "Sell")
                                                                                                                                              {
                                                                                                                                                return true;
                                                                                                                                              }
                                                                                                                                              return false;
                                                                                                                                          });
        os << name_itr->first << "'s Filled Orders (in chronological order):" << std::endl;
        std::copy_if(filled_orders[name_itr->first].begin(), filled_orders[name_itr->first].end(), std::ostream_iterator<Order>(os, "\n"), [] (Order& order)
                                                                                                                                          { 
                                                                                                                                              if (order.side == "Buy" || order.side == "Sell")
                                                                                                                                              {
                                                                                                                                                return true;
                                                                                                                                              }
                                                                                                                                              return false;
                                                                                                                                          });
    }
}

//===========================================================================================================================
//===========================================================================================================================

void Exchange::PrintTradeHistory(std::ostream &os) const
{
    os << "Trade History (in chronological order):" << std::endl;
    for (auto itr = trade_history.begin(); itr != trade_history.end(); ++itr)
    {
        os << itr->buyer_username << " Bought " << itr->amount << " of " << itr->asset 
           << " From " << itr->seller_username << " for " << itr->price << " USD" << std::endl;
    }
}

//===========================================================================================================================
//===========================================================================================================================
  
void Exchange::PrintBidAskSpread(std::ostream &os) const
{
    
}

//===========================================================================================================================
//===========================================================================================================================

