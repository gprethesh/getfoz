# Foz Betting Contract

This is an Wax Blockchain smart contract for placing and resolving bets. It allows users to place bets on events, resolve events, withdraw their funds, and more. 

## Installation

Compile the contract with EOSIO.CDT (Contract Development Toolkit). 
After installing EOSIO.CDT, compile the contract using:

```
eosio-cpp foz.cpp -o foz.wasm
```

## Features

### Deposit

Users can deposit WAX tokens to their account. Minimum deposit amount is 5 WAX. 

```c++
void deposit(name user, asset amount);
```

### Withdraw

Users can withdraw their funds from the contract. 

```c++
ACTION withdraw(name user, asset amount, string memo);
```

### Create Event

The contract account can create events. Events contain a description, creator, id, expiration time, source, tag, and price.

```c++
ACTION createvent(std::string description, name event_creator, uint64_t event_id, uint32_t expiration_seconds, std::string source, std::string tag, asset price);
```

### Place Bet

Users can place a bet on an event. A bet includes the user, event id, the side of the bet, and the amount.

```c++
ACTION placebet(name user, uint64_t event_id, bool bet_on, asset amount);
```

### Resolve Event

The contract account can resolve an event by specifying the event id, the outcome, and a batch size for how many bets to resolve at once. The outcome of the event determines the result of the bets.

```c++
ACTION resolveevent(name resolver, uint64_t event_id, bool outcome, uint64_t batch_size);
```

### Tax Withdrawal

The contract account can withdraw taxes collected from winning bets.

```c++
ACTION taxwithdraw(name to_account, asset amount);
```

### Clear Event

The contract account can clear (erase) a specific event.

```c++
ACTION clearevent(uint64_t event_id);
```

### Erase All Events

The contract account can erase all events.

```c++
ACTION eraseevents(name caller);
```

### Erase All Bets

The contract account can erase all bets.

```c++
ACTION erasebets(name caller);
```

## Tables

The contract has several tables to keep track of events, bets, deposits, taxes, and bet history.

- `deposits_table`: Keeps track of the deposits of each user.
- `history_table`: Keeps track of the history of bets.
- `taxes_table`: Keeps track of the total tax collected.
- `events_table`: Keeps track of all events.
- `bets_table`: Keeps track of all bets placed.

## Transfer Notifications

The contract listens for transfer notifications in order to process deposits.

```c++
void ontransfer(name from, name to, asset quantity, string memo);
```

## Notes

This code is property of https://getfoz.com.
