# W. Richard Stevens: TCP/IP Illustrated Vol 1

**Essential chapters:**

**Chapter 18: TCP Connection Establishment and Termination**
- Three-way handshake (SYN, SYN-ACK, ACK)
- Why this design necessary
- Four-way termination (FIN, ACK, FIN, ACK)
- `TIME_WAIT` state (affects server restarts)

**Chapter 20: TCP Timeout and Retransmission**
- Reliable stream semantics
- Why HTTP can assume reliable delivery
- Exponential backoff algorithms


Significance for WebServ:
Understand the layer below HTTP.
Know what TCP provides so you understand what HTTP doesn't need to handle.