#  🐙 Redis-From-Scratch 🐙

A lightweight Redis-like in-memory key-value store implemented from scratch in C++ using Socket API. This project is based on the [Build Your Own Redis](https://build-your-own.org/redis/) tutorial, with support for IPv6. Made on WSL

## Pre-requisite
1. C++ compatible serve
2. Linux/WSL 

## 📍 Running 

1. **Build server, then run it**
   ```bash
    cd build

    make server
   
   ./server
   ```
    Server runs on port 1234

2. **Build client, then run it**

   ```bash
   cd build

   make client
   
   ./client
   ```

## To Do
1. Add Test scripts

2. Restructure Client.cpp to run commands implemented (SET, ZSET, etc.). Currently only TTL Expiry is being run.

3. Convert all C style imports to C++ style imports

4. Add documentation over functions
