# RegisterS

Computer Networks project

## Getting Started

Cerinta: 
To implement a server that will provide customers with information about the servers (IP address and port) that offer the services they want.
The way of operation is as follows: when a server offering a certain service is started, it will send to the registration server the IP address, port and name of the service offered;
 when a client wants a certain service, he will first send a request to the registration server with the name of the desired service, and he will return a message containing the IP address and port of the server offering the desired service.

## Running the aplication

    make
    ./registerS
    ./SayHello 127.0.0.1 2777
    ./client 127.0.0.1 2777

Basicaly you may connect to server with any IP address at that specific PORT, but since we want to just test it will use the loopback address: 127.0.0.1

## License

This project is licensed under GNU 
GPL (General Public License)
