# Socket Programming Examples

This repository contains various applications of **socket programming**, demonstrating concepts such as **multithreading, concurrent servers, and inter-process communication**. These examples help in understanding how network communication works using **sockets** in a client-server model.

## Features

- **Basic 1:1 Chat**: A simple client-server model allowing two users to chat over a network.
- **Multiple Client Chat Server**: A chat server that supports multiple clients communicating in a shared environment.
- **Concurrent Server**: A server that can handle multiple clients simultaneously using multithreading.
- **Timer Server**: A server that returns the current date and time to the client.
- **Sorting Server**: A server that receives an array of numbers from a client, sorts them, and sends back the sorted array.

## Applications Covered

### 1. **Basic 1:1 Chat**
- A simple implementation of a chat system where one client communicates with a single server.
- Uses TCP sockets for reliable communication.

### 2. **Multiple Client Chat Server**
- A multi-client chat server that allows multiple users to connect and chat simultaneously.
- Uses threading to handle multiple clients concurrently.

### 3. **Concurrent Server**
- Implements a multi-threaded server to manage multiple client requests at the same time.
- Demonstrates the concept of handling parallel client connections.

### 4. **Timer Server**
- A client requests the current date and time from the server.
- The server responds with a timestamp, demonstrating basic request-response handling.

### 5. **Sorting Server**
- The client sends an array of numbers to the server.
- The server sorts the array and sends it back to the client.
- Useful for understanding how data is transmitted and processed over a network.

## Technologies Used
- **C** for implementation.
- **TCP/UDP sockets** for communication.
- **Multithreading** to handle multiple clients concurrently.

## How to Run

1. Clone the repository:
   ```sh
   git clone https://github.com/Anurag-ghosh-12/socket-program.git
   cd socket-programming-examples
   ```

2. Compile and run the server:
   ```sh
   gcc server.c -o server -lpthread
   ./server
   ```
  
3. Run the client:
   ```sh
   gcc client.c -o client
   ./client
   ```
  

## Contributions
Feel free to contribute by submitting issues or pull requests!

