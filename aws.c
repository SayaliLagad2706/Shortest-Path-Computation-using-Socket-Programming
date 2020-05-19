#include<arpa/inet.h>
#include<ctype.h>
#include<netdb.h>
#include<netinet/in.h>
#include<signal.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>

#define PORT_TCP 34953
#define PORT_UDP 33953
#define PORT_SERVERA 30953
#define PORT_SERVERB 31953
#define PORT_SERVERC 32953
#define QUEUE_LIMIT 2

/* stores the map lines received from Server A or Server B */
struct lines {
 char line[1000];
};

/* stores the map data split into vertices and edges */
struct data {
   char line[1000];
   int start_vertex, destination_vertex;
   float distance;
};

struct in_addr ip_addr, ip_addr_UDP;
struct sockaddr_in address_TCP, address_UDP, address_serverA, address_serverB, address_serverC;
struct sockaddr_storage client_address;
struct data arr[40];
struct lines line_data[40];

int j, start_vertex, destination_vertex, socket_Id_UDP, vertex_in_A = 100, vertex_in_B = 100;
float A, B;
char *op[90], map_ID[100],trans_speed[100], prop_speed[100], buffer[2048] = {0}, bufferA[1000], bufferC[1000], recv_bufferA[4000], recv_bufferB[4000], recv_bufferC[4000], path[10], shortest_dist[10], trans_delay[10], prop_delay[10];

int acceptTCPReq(int);
void bindAddressToTCPPort(int);
void bindUDP(int);
void checkAddressUse(int);
void checkReceivedData(char *);
void clearBuffers();
void closeConnection(int);
int connectToServer(char *);
void createServerAddr(char *);
void createTCPAddress();
int createTCPSocket();
void createUDPAddress();
int createUDPSocket();
void formDataBuffer();
void formDataForClient();
void getDataFromServer(char *);
void listenOnPort(int);
void populateData();
void readData();
void recvDataFromClient(int);
void recvDataFromServer(int, char *);
int searchInMap(char *);
void sendDataToClient(int, bool);
void sendDataToServer(int, char []);
bool validateReceivedData();

int main() {
   
   printf("The AWS is up and running.\n");
   
   int socket_Id = createTCPSocket();
   checkAddressUse(socket_Id);
   createTCPAddress();
   bindAddressToTCPPort(socket_Id);
   listenOnPort(socket_Id);
   
   socket_Id_UDP = createUDPSocket();
   checkAddressUse(socket_Id_UDP);
   createUDPAddress();
   bindUDP(socket_Id_UDP);
   
   while(true) {
      int new_socket = acceptTCPReq(socket_Id);
      recvDataFromClient(new_socket);
      
      // connect to Server A
      createServerAddr("A");
      connectToServer("A");
      checkReceivedData("A");
      
      // only if A does not have Map ID, check it further with Server B
      if(j == 0) {
         createServerAddr("B");
         connectToServer("B");
         checkReceivedData("B");
      }
      
      // if the vertices are found, proceed to connect with Server C
      bool flag = validateReceivedData();
      if(flag) {
         createServerAddr("C");
         connectToServer("C");
      }
      
      sendDataToClient(new_socket, flag);
      clearBuffers();
      closeConnection(new_socket);
   }
   closeConnection(socket_Id_UDP);
   closeConnection(socket_Id);
} // main

int acceptTCPReq(int socket_Id) {
   socklen_t addr_size;
   addr_size = sizeof(client_address);
   
   // create a new socket to accept the connection request from client
   int new_socket = accept(socket_Id, (struct sockaddr *) &client_address, &addr_size);
   
   // if the socket isn't created properly, return an error
   if(new_socket == -1) {
      exit(-1);
   }

   return new_socket;
} // acceptTCPReq

void bindAddressToTCPPort(int socket_Id) {
   // bind the created socket to the TCP address
   int bind_status = bind(socket_Id, (struct sockaddr *) &address_TCP, sizeof(address_TCP));
   
   // check if binding is done successfully
   if(bind_status == -1) {
      exit(-1);
   } 
} // bindAddressToTCPPort

void bindUDP(int socket_ID) {
   // bind the created socket to the UDP address
   int bind_status = bind(socket_ID, (struct sockaddr *) &address_UDP, sizeof(address_UDP));
   
   // check if binding is done successfully
   if(bind_status == -1) {
      exit(-1);
   }
} // bindUDP

void checkAddressUse(int socket_Id) {
   int opt = 1;
   if(setsockopt(socket_Id, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
      exit(-1);
   }
} // checkAddressUse

void clearBuffers() {
   memset(bufferA, 0, sizeof(bufferA));
   memset(bufferC, 0, sizeof(bufferC));
   memset(recv_bufferA, 0, sizeof(recv_bufferA));
   memset(recv_bufferB, 0, sizeof(recv_bufferB));
   memset(recv_bufferC, 0, sizeof(recv_bufferC));
   memset(buffer, 0, sizeof(buffer));
   memset(arr, 0, sizeof(arr));
   memset(line_data, 0, sizeof(line_data));
   j = 0;   
} // clearBuffers

void checkReceivedData(char *server_name) {
   char *p;

   if(strcmp(server_name, "A") == 0) {
      // check if received data contains propagation speed and thus map data
      A = strtod(recv_bufferA, &p);
      
      // if map data is present, store the data and search for start and destination vertex
      if(A != 0) {
         getDataFromServer("A");
         vertex_in_A = searchInMap("A");
      }
   }
   else {
      B = strtod(recv_bufferB, &p);
      if(B != 0) {
         getDataFromServer("B");
         vertex_in_B = searchInMap("B");
      }
   }
} // checkReceivedData

void closeConnection(int socket_ID) {
  int status = close(socket_ID);
  
  // check if connection is closed successfully
  if(status == -1) {
  }
} // closeConnection

int connectToServer(char *server_name) {
   sendDataToServer(socket_Id_UDP, server_name);
   recvDataFromServer(socket_Id_UDP, server_name);
} // connectToServer

void createServerAddr(char *server_name) {
   if(strcmp(server_name, "A") == 0) {
      address_serverA.sin_family = AF_INET;
      address_serverA.sin_addr = ip_addr_UDP;
      address_serverA.sin_port = htons(PORT_SERVERA);
   }
   else if(strcmp(server_name, "B") == 0) {
      address_serverB.sin_family = AF_INET;
      address_serverB.sin_addr = ip_addr_UDP;
      address_serverB.sin_port = htons(PORT_SERVERB);
   }
   else {
      address_serverC.sin_family = AF_INET;
      address_serverC.sin_addr = ip_addr_UDP;
      address_serverC.sin_port = htons(PORT_SERVERC);
   }  
} // createServerAddr

void createTCPAddress() {
   // create an address for self for TCP
   ip_addr.s_addr = inet_addr("127.0.0.1");
   address_TCP.sin_family = AF_INET;
   address_TCP.sin_addr = ip_addr;
   address_TCP.sin_port = htons(PORT_TCP);
} // createTCPAddress

int createTCPSocket() {
   // create a socket to accept connection requests from clients
   int socket_Id = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   
   // if the socket isn't created properly, return an error
   if(socket_Id == -1) {
      exit(-1);
   }
   return socket_Id;
} // createTCPSocket

void createUDPAddress() {
   // create an address for self for UDP
   ip_addr_UDP.s_addr = inet_addr("127.0.0.1");
   address_UDP.sin_family = AF_INET;
   address_UDP.sin_addr = ip_addr_UDP;
   address_UDP.sin_port = htons(PORT_UDP);
} // createUDPAddress

int createUDPSocket() {
   // create a socket for connection with Servers over UDP
   int socket_ID = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   
   // if the socket isn't created properly, return an error
   if(socket_ID == -1) {
      exit(-1);
   }
   
   return socket_ID;
} // createUDPSocket

void formDataBuffer() {
   char start_index[10], dest_index[10];
   sprintf(start_index, "%d\n", start_vertex);
   sprintf(dest_index, "%d\n", destination_vertex);
   
   // store the map info to send it to Server C
   sprintf(bufferC, "%s\n", op[0]);
   strcat(bufferC, start_index);
   strcat(bufferC, dest_index);
   strcat(bufferC, op[3]);
   strcat(bufferC, "\n");
   strcat(bufferC, prop_speed);
   strcat(bufferC, trans_speed);
  
   for(int i = 0; i < j; i++) {
      strcat(bufferC, arr[i].line);
      strcat(bufferC, "\n");
   }
} // formDataBuffer

void formDataForClient() {
   // store the data to be sent to the client in buffer recv_bufferC
   sprintf(recv_bufferC, "%s", shortest_dist);
   strcat(recv_bufferC, trans_delay);
   strcat(recv_bufferC, prop_delay);
   strcat(recv_bufferC, path);
} // formDataForClient

void getDataFromServer(char* server_name) {
   int i = 0;
   char *recv_data;
   
   if(strcmp(server_name, "A") == 0) {
      recv_data = recv_bufferA;
   }
   else {
      recv_data = recv_bufferB;
   }
   
   // split the received data and store it in data structure
   recv_data = strtok(recv_data, "\n");
   while(recv_data != NULL) {
      
      if(i == 0) {
         sprintf(prop_speed, "%s\n", recv_data);
      }
      else if(i == 1) {
         sprintf(trans_speed, "%s\n", recv_data);
      }
      else {
         sprintf(line_data[j].line, "%s", recv_data);
         j++;
      }
      i++;
      recv_data = strtok(NULL, "\n");
   }
   populateData();
   
} // getDataFromServer

void listenOnPort(int socket_Id) {
   // listen on the port
   int listen_status = listen(socket_Id, QUEUE_LIMIT);
   
   // check to see if the server can listen without errors
   if(listen_status == -1) {
      exit(-1);
   }
   
   printf("AWS server is listening on port %d\n", PORT_TCP);
} // listenOnPort

void populateData() {
   
   for(int i = 0; i < j; i++) {
      sprintf(arr[i].line, "%s", line_data[i].line);
      char *data = strtok(line_data[i].line, " ");
      int k = 0;
      
      while(data != NULL) {
         if(k == 0) {
            arr[i].start_vertex = atoi(data);
         }
         else if(k == 1) {
            arr[i].destination_vertex = atoi(data);
         }
         else { 
            arr[i].distance = atof(data);
         }
         k++;
         data = strtok(NULL, " ");
      }
   }
} // populateData

void readData() {
   char *data = recv_bufferC;
   int i = 0;
   
   data = strtok(data, "\n");
   while(data != NULL) {
      if(i == 0) {
         printf("%s\n", data);
         sprintf(path, "%s\n", data);
      }
      else if(i == 1) {
         printf("\tShortest distance: %.2f km\n", atof(data));
         sprintf(shortest_dist, "%s\n", data);
      }
      else if(i == 2) {
         printf("\tTransmission delay: %.2f s\n", atof(data));
         sprintf(trans_delay, "%s\n", data);
      }
      else {
         printf("\tPropagation delay: %.2f s\n", atof(data));
         sprintf(prop_delay, "%s\n", data);
      }
      i++;
      data = strtok(NULL, "\n");
   }
} // readData

void recvDataFromClient(int new_socket) {
   // receive the data from client
   int count = recv(new_socket, buffer, 2048, 0); 

   // check if the data is received successfully
   if(count == -1) {
      exit(-1);
   }
   
   char* recv_data = strtok(buffer, "\t"); // split the data to get individual information
   
   // store the split data into array
   int i = 0;
   while(recv_data != NULL) {
      op[i] = recv_data;
      i++;
      recv_data = strtok(NULL, "\t");
   }
   
   start_vertex = atoi(op[1]);
   destination_vertex = atoi(op[2]);
   
   printf("The AWS has received map ID %s, start vertex %s, destination vertex %s and file size %s from the client using TCP over port %d\n", op[0], op[1], op[2], op[3], PORT_TCP);
} // recvDataFromClient

void recvDataFromServer(int socket_ID, char *server_name) {
  int n;
  
  if(strcmp(server_name, "A") == 0) {
   int len = sizeof(address_serverA);
   n = recvfrom(socket_ID, recv_bufferA, sizeof(recv_bufferA), 0, (struct sockaddr *)&address_serverA, &len);
  }
  else if(strcmp(server_name, "B") == 0) {
    int len = sizeof(address_serverB);
    n = recvfrom(socket_ID, recv_bufferB, sizeof(recv_bufferB), 0, (struct sockaddr *)&address_serverB, &len);
  }
  else {
    int len = sizeof(address_serverC);
    n = recvfrom(socket_ID, recv_bufferC, sizeof(recv_bufferC), 0, (struct sockaddr *)&address_serverC, &len);  
  }
  
  // check if the data is received successfully
  if(n == -1) {
    exit(-1);
  }
  
  if(strcmp(server_name, "A") == 0 || strcmp(server_name, "B") == 0) {
   printf("The AWS has received map information from Server %s\n", server_name);
  }
  else {
   printf("The AWS has received results from Server C:\n\tShortest Path: ");
   readData();
  }
} // recvDataFromServer

int searchInMap(char *server_name) {
   bool found_start_vertex, found_dest_vertex;

   // check if the start and destination vertices are present in the given map
   for(int i = 0; i < j; i++) {
      if(arr[i].start_vertex == start_vertex || arr[i].destination_vertex == start_vertex) {
         found_start_vertex = true;
      }
      if(arr[i].start_vertex == destination_vertex || arr[i].destination_vertex == destination_vertex) {
         found_dest_vertex = true;
      }
   }
   
   if(found_start_vertex && found_dest_vertex) {
      formDataBuffer();
      return -1;
   }
   else if(!found_start_vertex) {
      return start_vertex;
   }
   else {
      return destination_vertex;
   }
} // searchInMap

void sendDataToClient(int socketId, bool flag) {
   if(flag) {
      memset(recv_bufferC, 0, sizeof(recv_bufferC));
      formDataForClient();
   }
   
   // send the data to server C
   int sent_count = send(socketId, (void *) &recv_bufferC, sizeof(recv_bufferC), 0);
   
   // check if data is sent successfully to client
   if(sent_count == -1) {
      exit(-1);
   }
   
   if(flag) {
      printf("The AWS has sent calculated results to client using TCP over port %d\n", PORT_TCP);
   }
} // sendDataToClient

void sendDataToServer(int socket_ID, char *server_name) {
   sprintf(bufferA, "%s\t", op[0]);
   int n;
   
   if(strcmp(server_name, "A") == 0) {
      n = sendto(socket_ID, (char *) bufferA, sizeof(bufferA), MSG_CONFIRM, (const struct sockaddr *) &address_serverA, sizeof(address_serverA));
   }
   else if(strcmp(server_name, "B") == 0) {
      n = sendto(socket_ID, (char *) bufferA, sizeof(bufferA), MSG_CONFIRM, (const struct sockaddr *) &address_serverB, sizeof(address_serverB));
   }
   else {
      n = sendto(socket_ID, (char *) bufferC, sizeof(bufferC), MSG_CONFIRM, (const struct sockaddr *) &address_serverC, sizeof(address_serverC));
   }
   
   // check if data is sent successfully
   if(n == -1) {
      exit(-1);
   }
   
   if(strcmp(server_name, "C") == 0) {
      printf("The AWS has sent map, source ID, destination ID, propagation speed and transmission speed to Server C using UDP over port %d\n", PORT_UDP);
   }
   else {
      printf("The AWS has sent map ID to Server %s using UDP over port %d\n", server_name, PORT_UDP);
   }
} // sendDataToServer

bool validateReceivedData() {
   bool result;

   if(A == 0 && B == 0) {
      sprintf(recv_bufferC, "%s\n", "No Map ID found");
      result = false;
   }
   else {
      if(A != 0) {
         if(vertex_in_A == -1) {
            printf("The source and destination vertex are in the graph\n");
            result = true;
         }
         else {
            printf("%d vertex not found in the graph, sending error to client using TCP over port %d\n", vertex_in_A, PORT_TCP);
            sprintf(recv_bufferC, "%d\n", vertex_in_A);
            strcat(recv_bufferC, "No Vertex ID found\n");
            result = false;
         }
      }
      else if(B != 0) {
         if(vertex_in_B == -1) {
            printf("The source and destination vertex are in the graph\n");
            result = true;
         }
         else {
            sprintf(recv_bufferC, "%d\n", vertex_in_B);
            printf("%d vertex not found in the graph, sending error to client using TCP over port %d\n", vertex_in_B, PORT_TCP);
            strcat(recv_bufferC, "No Vertex ID found\n");
            result = false;
         }
      }
   }
   return result;
} // validateReceivedData
