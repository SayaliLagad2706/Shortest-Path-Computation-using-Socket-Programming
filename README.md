# Shortest-Path-Computation-using-Socket-Programming
- All network related applications require identification of shortest path between source host and destination host to optimize routing performance
- The aim of this project is to implement a distributed system that computes the shortest path given a client query involving map ID and source and destination host ID
- The client server architecture involves a client, a main server and 3 back-end servers (A, B and C). Server A and Server B store map information for several map IDs and Server C is responsible for path calculation
- Socket programming is used to carry out communication between the client and each of the back-end servers via the main server using TCP and UDP socket connections in the transport layer
- Dijkstra’s algorithm is implemented by Server C to determine the shortest path cost and thus the least propagation and transmission delay for transmitting a file from one host to another
- The result is the optimal routing path and a detailed calculation of delays displayed on the client terminal
- Refer to project_description.pdf for detailed explanation of the system to be built

Technologies Used:
  -
  - Socket Programming
  - TCP and UDP sockets
  - Environment: UNIX
  - Language: C
