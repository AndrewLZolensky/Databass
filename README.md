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

# Image
![alt text](bassfish.png)