# simple-http

SimpleHTTP is a simple cross-platform C++ library used to handle HTTP connections.

## Note
SimpleHTTP is still a prototype, and should not be used in any circumstance other than development.

# Platforms
- Windows
- Linux

## Intended Features

### Core

- [ ] Support HTTP version up to HTTP/3.0
    - [ ] HTP/0.9
    - [X] HTP/1.0
    - [ ] HTP/1.1
    - [ ] HTP/2.0
    - [ ] HTP/3.0
- [ ] HTTP Client
    - [ ] Create Server Connection
    - [ ] Send Request
    - [ ] Receive Response
- [ ] HTTP Server
    - [X] Accept Client Connection
    - [X] Receive Request
    - [X] Send Response
- [ ] HTTPS Support

### Server Executor
A Server Executor is an object that wraps an HTTP server to handle the connections with the clients.

- [X] Multi-thread execution of Request handling code
- [ ] `Keep-Alive` feature

### Server Request Handler
A Server Request Handler is an object that processes the incoming Request and generates the appropriate response.

 - [ ] Filter incoming Request
 - [X] URI-based Request dispatch
 - [X] Method-based Request dispatch
 - [ ] Resource abstraction