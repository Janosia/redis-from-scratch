#  🐙 Redis-From-Scratch 🐙

A lightweight Redis-like in-memory key-value store implemented from scratch in C++ using the POSIX Socket API. This project is based on the [Build Your Own Redis](https://build-your-own.org/redis/) tutorial, with support for IPv6. Made on WSL

## Pre-requisite
1. C++ compatible serve
2. Linux/WSL 🐧

## 📍 Running 

1. **Build server, then run it**
   ```bash
    g++ -Wall -Wextra -Og -g server.cpp -o server
   
   ./server
   ```
    Server runs on port 1234

2. **Build client, then run it**

   ```bash
   g++ -Wall -Wextra -Og -g client.cpp -o client
   
   ./client
   ```
