# Foz Betting Contract

This is an Wax Blockchain smart contract for placing and resolving bets. It allows users to place bets on events, resolve events, withdraw their funds, and more. 

## Requirements

To compile and deploy this contract, you will need:
1. EOSIO software installed on your system.
2. The account on the WAX testnet where you want to deploy this contract.

## Compile the Contract

Firstly, we need to compile the contract using `eosio-cpp`. Navigate to the folder containing `foz.cpp` and run:

```
eosio-cpp -abigen -o foz.wasm foz.cpp
```

This command generates two files `foz.wasm` (the compiled contract) and `foz.abi` (the contract's Application Binary Interface file).

## Set the Contract

Next, we set the contract to our account. In this example, we're using an account named `boatheadboat`.

Replace `/Users/georgeprethesh/contracts/foz` with the path where your compiled contract is located.

```
cleos -u  https://wax-testnet.eosphere.io set contract boatheadboat /Users/georgeprethesh/contracts/foz foz.wasm --abi foz.abi -p boatheadboat
```

## Set the Contract Permissions

The contract needs permission to execute its actions. Here we set the `eosio.code` permission for our contract.

```
cleos --url https://wax-testnet.eosphere.io set account permission boatdeadboat active '{"threshold": 1, "keys": [{"key": "EOS83PPpjU9zQwLwMGsA9SBbNDGh62eDU7mNvX4y3rr4jZcow5Qa8", "weight": 1}], "accounts": [{"permission": {"actor": "boatdeadboat", "permission": "eosio.code"}, "weight": 1}]}' -p boatdeadboat@active
```

Note: Always ensure you replace the account names and key with your account details.

Replace `EOS83PPpjU9zQwLwMGsA9SBbNDGh62eDU7mNvX4y3rr4jZcow5Qa8` with your account public key.

`https://wax-testnet.eosphere.io` with your testnet rpc or endpoint

`boatdeadboat` with your account

That's it! Your smart contract is now deployed on the WAX testnet.


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
