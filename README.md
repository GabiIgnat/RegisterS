# RegisterS: A Service Registration and Discovery System

**Author:** Ignat Gabriel-Andrei, Year II, Group A4  

## Project Overview

RegisterS is a system designed to map service names to their corresponding IP addresses and ports, similar to the DNS protocol. This simplifies access to network services by allowing users to connect using easily memorable service names.

## Key Features

- **Service Registration & Discovery**: Servers register their services with IP and port, and clients query these services.
- **Concurrency**: Handles multiple clients simultaneously using threads.
- **Reliability**: Utilizes TCP/IP for stable and reliable communication.

## Technologies Used

- **TCP/IP Protocol**
- **POSIX Sockets**
- **Threading with Mutexes**
- **C Programming Language**

## Usage

RegisterS allows servers to register their services, and clients to discover and connect to these services through the RegisterS server.

## Documentation

Detailed implementation, architecture, and additional information can be found in the [project documentation](RegisterS.pdf).

## Running the aplication

    make
    ./registerS
    ./SayHello 127.0.0.1 2777
    ./client 127.0.0.1 2777

Basicaly you may connect to server with any IP address at that specific PORT, but since we want to just test it will use the loopback address: 127.0.0.1
