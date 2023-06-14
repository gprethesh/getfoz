#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>

using namespace eosio;
using namespace std;

CONTRACT foz : public contract {
public:
    using contract::contract;

    ACTION withdraw(name user, asset amount, string memo);
    ACTION clearevent(uint64_t event_id);
    ACTION eraseevents(name caller);
    ACTION erasebets(name caller);
    ACTION taxwithdraw(name to_account, asset amount);
    ACTION createvent(std::string description, name event_creator, uint64_t event_id, uint32_t expiration_seconds, std::string source, std::string tag, asset price);
    ACTION placebet(name user, uint64_t event_id, bool bet_on, asset amount);
    ACTION resolveevent(name resolver, uint64_t event_id, bool outcome, uint64_t batch_size);

    [[eosio::on_notify("eosio.token::transfer")]]
    void ontransfer(name from, name to, asset quantity, string memo) {
        if (to != get_self()) {
            return;
        }

        // Check if the transfer is a withdrawal
        if (from == get_self()) {
            if (memo == "withdraw") {
                // Ignore the memo check during withdrawal
                return;
            }
        }

        check(memo == "deposit", "Only transfers with memo 'deposit' or 'withdraw' are allowed");

        deposit(from, quantity);
    }



private:
    TABLE deposit_info {
        name user;
        asset balance;

        uint64_t primary_key() const { return user.value; }
    };

    typedef multi_index<"deposits"_n, deposit_info> deposits_table;
    
    
    // Inside your class declaration
    TABLE history_struct {
        uint64_t history_id;  // new unique ID for each history
        name user;
        uint64_t event_id;
        asset amount;
        std::string status;

        uint64_t primary_key() const { return history_id; }
        uint64_t by_user() const { return user.value; }  // secondary index
    };

    typedef eosio::multi_index<"history"_n, history_struct,
    indexed_by<"user"_n, const_mem_fun<history_struct, uint64_t, &history_struct::by_user>>
    > history_table;

    
    
    TABLE tax_info {
        asset tax_balance;
        uint64_t primary_key() const { return tax_balance.symbol.raw(); }
    };

    typedef eosio::multi_index<"taxes"_n, tax_info> taxes_table;

    TABLE event {
        uint64_t event_id;
        bool resolved;
        bool outcome;
        time_point_sec expiration;
        uint64_t resolved_bets;
        std::optional<uint64_t> last_processed_bet_id;
        std::string description;
        uint64_t yes_bets = 0;          // Added field to store total number of yes bets
        uint64_t no_bets = 0;           // Added field to store total number of no bets
        asset total_amount;             // Added field to store total amount bet
        std::string source;             // New field for source
        std::string tag;                // New field for tag
        asset price;                    // New field for price

        uint64_t primary_key() const { return event_id; }
    };


    typedef multi_index<"events"_n, event> events_table;

    TABLE bet {
        uint64_t bet_id;
        name user;
        uint64_t event_id;
        bool bet_on;
        asset amount;
        std::optional<uint64_t> matched_with;

        uint64_t primary_key() const { return bet_id; }
        uint64_t by_user() const { return user.value; }
        uint64_t by_event() const { return event_id; }
    };

    typedef eosio::multi_index<"bets"_n, bet,
        indexed_by<"byuser"_n, const_mem_fun<bet, uint64_t, &bet::by_user>>,
        indexed_by<"byevent"_n, const_mem_fun<bet, uint64_t, &bet::by_event>>
    > bets_table;
    
    void deposit(name user, asset amount);
};

void foz::deposit(name user, asset amount) {
    require_auth(user);

    check(amount.symbol == symbol("WAX", 8), "Only WAX tokens are allowed");
    check(amount.amount >= 50000000, "Minimum deposit is 5 WAX");

    deposits_table deposits(get_self(), get_self().value);
    auto dep_itr = deposits.find(user.value);
    if (dep_itr == deposits.end()) {
        deposits.emplace(get_self(), [&](auto &row) {
            row.user = user;
            row.balance = amount;
        });
    } else {
        deposits.modify(dep_itr, same_payer, [&](auto &row) {
            row.balance += amount;
        });
    }
}

ACTION foz::withdraw(name user, asset amount, string memo) {
    require_auth(user);

    check(amount.symbol == symbol("WAX", 8), "Only WAX tokens are allowed");

    deposits_table deposits(get_self(), get_self().value);
    auto dep_itr = deposits.find(user.value);
    check(dep_itr != deposits.end(), "User not found");
    check(dep_itr->balance >= amount, "Insufficient balance");

    deposits.modify(dep_itr, same_payer, [&](auto &row) {
        row.balance -= amount;
    });

    action(permission_level{get_self(), "active"_n},
           "eosio.token"_n,
           "transfer"_n,
           make_tuple(get_self(), user, amount, memo))
    .send();
}


ACTION foz::clearevent(uint64_t event_id) {
    require_auth(get_self()); // Only the contract account can call this action.

    events_table events(get_self(), get_self().value);
    auto event_itr = events.find(event_id);
    
    check(event_itr != events.end(), "Event not found");

    events.erase(event_itr);
}

ACTION foz::eraseevents(name caller) {
require_auth(caller);
check(caller == get_self(), "Only the contract account can call this action.");

events_table events(get_self(), get_self().value);
auto event_itr = events.begin();

while (event_itr != events.end()) {
    event_itr = events.erase(event_itr);
}
}

ACTION foz::erasebets(name caller) {
require_auth(caller);
check(caller == get_self(), "Only the contract account can call this action.");

bets_table bets(get_self(), get_self().value);
auto bet_itr = bets.begin();

while (bet_itr != bets.end()) {
    bet_itr = bets.erase(bet_itr);
}
}

ACTION foz::createvent(std::string description, name event_creator, uint64_t event_id, uint32_t expiration_seconds, std::string source, std::string tag, asset price) {
    require_auth(event_creator);

    check(event_creator == get_self(), "Only the contract account can create events");

    check(price.symbol == symbol("WAX", 8), "Price must be in WAX"); // Check for WAX symbol

    events_table events(get_self(), get_self().value);
    auto event_itr = events.find(event_id);
    check(event_itr == events.end(), "Event ID already exists");

    events.emplace(event_creator, [&](auto &row) {
        row.event_id = event_id;
        row.resolved = false;
        row.outcome = false;
        row.expiration = time_point_sec(current_time_point().sec_since_epoch() + expiration_seconds);
        row.resolved_bets = 0;
        row.description = description;
        row.total_amount = asset(0, symbol("WAX", 8)); // Initialize total_amount with 0 WAX
        row.source = source;   // set the source
        row.tag = tag;         // set the tag
        row.price = price;     // set the price
    });
}


ACTION foz::taxwithdraw(name to_account, asset amount) {
require_auth(get_self());

taxes_table taxes(get_self(), get_self().value);
auto tax_itr = taxes.find(amount.symbol.raw());
check(tax_itr != taxes.end(), "No tax balance found for the given symbol");
check(tax_itr->tax_balance >= amount, "Insufficient tax balance");

// Update the tax balance in the taxes table
taxes.modify(tax_itr, same_payer, [&](auto &row) {
    row.tax_balance -= amount;
});

// Transfer the tax amount to the specified account
action(permission_level{get_self(), "active"_n},
       "eosio.token"_n, "transfer"_n,
       std::make_tuple(get_self(), to_account, amount, std::string("Tax withdrawal")))
    .send();
}

ACTION foz::placebet(name user, uint64_t event_id, bool bet_on, asset amount) {
    require_auth(user);

    check(amount.symbol == symbol("WAX", 8), "Only WAX tokens are allowed");

    events_table events(get_self(), get_self().value);
    auto event_itr = events.find(event_id);
    check(event_itr != events.end(), "Event not found");
    check(event_itr->expiration > current_time_point(), "Event expired");

    deposits_table deposits(get_self(), get_self().value);
    auto dep_itr = deposits.find(user.value);
    check(dep_itr != deposits.end(), "Please deposit to place trade or User not found");
    check(dep_itr->balance >= amount, "Insufficient balance");

    // Check if user has already placed a bet on this event
    bets_table bets(get_self(), get_self().value);
    auto user_index = bets.get_index<"byuser"_n>();
    auto user_bets_itr = user_index.lower_bound(user.value);

    while(user_bets_itr != user_index.end() && user_bets_itr->user == user) {
        check(user_bets_itr->event_id != event_id, "User has already placed a bet on this event");
        user_bets_itr++;
    }

    // Attempt to match bet
    auto matched_bet_id = std::optional<uint64_t>{};
    for (auto& bet : bets) {
        if (bet.event_id == event_id && bet.bet_on != bet_on && bet.amount == amount && !bet.matched_with) {
            matched_bet_id = bet.bet_id;
            break;
        }
    }

    deposits.modify(dep_itr, same_payer, [&](auto& row) {
        row.balance -= amount;
    });

    bets.emplace(get_self(), [&](auto& row) {
        row.bet_id = bets.available_primary_key();
        row.user = user;
        row.event_id = event_id;
        row.bet_on = bet_on;
        row.amount = amount;
        row.matched_with = matched_bet_id;
    });

    if (matched_bet_id) {
        auto matched_bet_itr = bets.find(*matched_bet_id);
        bets.modify(matched_bet_itr, get_self(), [&](auto& row) {
            row.matched_with = bets.available_primary_key() - 1;
        });
    }

    // Update the event to store the bet
    events.modify(event_itr, get_self(), [&](auto& row) {
        if (bet_on) {
            row.yes_bets += 1;
        } else {
            row.no_bets += 1;
        }
        row.total_amount += amount;
    });
}



ACTION foz::resolveevent(name resolver, uint64_t event_id, bool outcome, uint64_t batch_size) {
    require_auth(resolver);

    check(resolver == get_self(), "Only the contract account can resolve events");

    events_table events(get_self(), get_self().value);
    auto event_itr = events.find(event_id);
    check(event_itr != events.end(), "Event not found");
    check(!event_itr->resolved, "Event already resolved");

    uint64_t total_bets_for_event = 0;
    uint64_t history_id = 0;
    bets_table bets(get_self(), get_self().value);
    for (const auto &bet : bets) {
        if (bet.event_id == event_id) {
            total_bets_for_event++;
        }
    }

    deposits_table deposits(get_self(), get_self().value);
    taxes_table taxes(get_self(), get_self().value);
    history_table history(get_self(), get_self().value); // Define history table here
    auto bet_itr = bets.begin();
    uint64_t processed_bets = 0;

    uint64_t start_index = event_itr->last_processed_bet_id.has_value() ? event_itr->last_processed_bet_id.value() + 1 : 0;
    
    auto itr = history.rbegin(); // Gets an iterator pointing to the last element of history
    if (itr != history.rend()) { // If history is not empty
        history_id = itr->history_id + 1; // Start from the last history_id plus one
    }

    while (bet_itr != bets.end() && processed_bets < batch_size) {
        if (bet_itr->event_id == event_id && bet_itr->bet_id >= start_index) {
            int64_t winning_amount = bet_itr->amount.amount;
            int64_t tax_amount = 0;
            std::string bet_status = "";

            if (bet_itr->bet_on == outcome && bet_itr->matched_with) {
                winning_amount = (bet_itr->amount.amount * 2);
                tax_amount = winning_amount * 20 / 100;
                winning_amount -= tax_amount;
                bet_status = "won";
            } else if (!bet_itr->matched_with) {
                winning_amount = bet_itr->amount.amount;
                bet_status = "refunded";
            } else {
                winning_amount = 0;
                bet_status = "lost";
            }

            if (winning_amount > 0) {
                auto dep_itr = deposits.find(bet_itr->user.value);
                deposits.modify(dep_itr, same_payer, [&](auto &row) {
                    row.balance += asset(winning_amount, bet_itr->amount.symbol);
                });

                if (tax_amount > 0) {
                    auto tax_itr = taxes.find(bet_itr->amount.symbol.raw());
                    if (tax_itr == taxes.end()) {
                        taxes.emplace(get_self(), [&](auto &row) {
                            row.tax_balance = asset(tax_amount, bet_itr->amount.symbol);
                        });
                    } else {
                        taxes.modify(tax_itr, same_payer, [&](auto &row) {
                            row.tax_balance += asset(tax_amount, bet_itr->amount.symbol);
                        });
                    }
                }
            }

            events.modify(event_itr, same_payer, [&](auto &row) {
                row.resolved_bets += 1;
                row.last_processed_bet_id = bet_itr->bet_id;
            });

            // Add the bet resolution to the history table
            history.emplace(get_self(), [&](auto &row) {
                row.history_id = history_id++;
                row.user = bet_itr->user;
                row.event_id = event_id;
                row.amount = asset(winning_amount, bet_itr->amount.symbol);
                row.status = bet_status;
            });


            bet_itr = bets.erase(bet_itr);
            processed_bets++;
        } else {
            ++bet_itr;
        }
    }

    if (event_itr->resolved_bets == total_bets_for_event) {
        events.modify(event_itr, same_payer, [&](auto &row) {
            row.resolved = true;
            row.outcome = outcome;
        });
    }
}


