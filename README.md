# Databass
A Distributed Tabular Database with Load-Balancing, Replication, and Recovery

# Features
+ Shell Interface
+ Networked Interface
+ Thread-Safe Database Operations
- Distribution
- Replication
- Consistency
- Fault-Tolerance
- Scalability
- Crash Detection

# Interface
```
RFC:
GET <row> <col> -> 250 OK <bytes>, 550 FAILURE
PUT <row> <col> <bytes> -> 250 OK, 550 FAILURE
DEL <row> <col> -> 250 OK, 550 FAILURE
```
+ Thread-Safe Database Operations

# Flow

Client-Coordinator
1. Client stub contacts the coordinator with a LOOKUP <row>
2. Coordinator replies with +OK <ip> <port> or -BAD <error-code>

Client-Server
3. Client stub contacts server with <method> <row> <col> <optional-value>
4. Server replies with +OK <optional-value> or -BAD <error-code>
```
RFC:
GET <row> <col> -> +OK <bytes>, -BAD
PUT <row> <col> <bytes> -> +OK, -BAD
DEL <row> <col> -> +OK, -BAD
EXIT, +OK
```

Coordinator-Server

Server-Server


# Image
![alt text](bassfish.png)