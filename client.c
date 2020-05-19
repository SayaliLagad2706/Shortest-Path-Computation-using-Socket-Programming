#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include <unistd.h> 

#define SERVER_PORT 34953

// structure to store input from command line argument 
struct input_data {
   char map_ID[20];
   int start_vertex, destination_vertex, file_size;
};

struct input_data data;
struct in_addr ip_addr;
struct sockaddr_in server_address;

char buffer[100], recv_buffer[4000];

void clearBuffers();
void closeConnection(int);
void connectToServer(int);
void createServerAddress();
int createSocket();
void readData();
void receiveData(int);
void sendData(int);

int main(int argc, char* argv[]) {
   char *map_id;
   
   if(argc < 5) {
      printf("Please provide the Map ID, Source Vertex, Destination Vertex and file size to proceed");
      return -1;
   }
   
   if(argc > 5) {
      printf("Please provide only the Map ID, Source Vertex, Destination Vertex and file size!");
      return -1;
   }
   
   printf("The client is up and running\n");
   
   map_id = argv[1];
   strcpy(data.map_ID, map_id);
   data.start_vertex = atoi(argv[2]);
   data.destination_vertex = atoi(argv[3]);
   data.file_size = atoi(argv[4]);
  
   // serialize the data to send over the socket
   sprintf(buffer, "%s\t%d\t%d\t%d", data.map_ID, data.start_vertex, data.destination_vertex, data.file_size);
   
   int socket_Id = createSocket();
   createServerAddress();
   connectToServer(socket_Id);
   sendData(socket_Id); 
   receiveData(socket_Id);
   closeConnection(socket_Id);
   clearBuffers();
} // main

void clearBuffers() {
   // clear buffers after each query result is received
   memset(buffer, 0, sizeof(buffer));
   memset(recv_buffer, 0, sizeof(recv_buffer));
} // clearBuffers

void closeConnection(int socket_ID) {
  int status = close(socket_ID);
  
  if(status == -1) {
    
  }
} // closeConnection

void connectToServer(int socket_Id) {
   // set up a connection with the desired server 
   int status = connect(socket_Id, (struct sockaddr *)&server_address, sizeof(server_address));
   
   // check if the connection returns an error
   if(status == -1) {
      exit(-1);
   }
} // connectToServer

void createServerAddress() {
   // store the address of the server to which the connection is to be set
   ip_addr.s_addr = inet_addr("127.0.0.1");
   server_address.sin_family = AF_INET;
   server_address.sin_addr = ip_addr;
   server_address.sin_port = htons(SERVER_PORT);
} // createServerAddress

int createSocket() {
   // create a socket for TCP connection, TCP protocol
   int socket_Id = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   
   // if the socket is not created properly, exit from the code
   if(socket_Id == -1) {
      exit(-1);
   }
   
   return socket_Id;
} // createSocket

void readData() {
   char *d = recv_buffer;
   float trans_delay, prop_delay;
   
   // split the data and display calculated results
   int i = 0;
   d = strtok(d, "\n");
   while(d != NULL) {
      if(i == 3) {
         printf("%12.2f", trans_delay + prop_delay);
         printf("\n------------------------------------------------------------------------------\nShortest path: %s\n", d);
      }
      else if(i == 0){
         printf("%18.2f", atof(d));
      }
      else if(i == 1){
         trans_delay = atof(d);
         printf("%12.2f", trans_delay);
      }
      else {
         prop_delay = atof(d);
         printf("%12.2f", prop_delay);
      }
      i++;
      d = strtok(NULL, "\n");
   }
} // readData

void receiveData(int socket_Id) {
   // receive the data from AWS
   int count = recv(socket_Id, recv_buffer, 2048, 0);
   if(count == -1) {
       exit(-1);
   }
   
   // check if the data contains calculated results
   if(!strstr(recv_buffer, "No")) {
      printf("The client has received results from AWS:\n------------------------------------------------------------------------------\n");
      printf("Source    Destination    Min Length        Tt          Tp        Delay\n------------------------------------------------------------------------------\n%4d\t%7d\t", data.start_vertex, data.destination_vertex);
      readData();
   }
   else {
      if(strstr(recv_buffer, "Map")) {
         printf("No Map ID %s found\n", data.map_ID);
      }
      else {
         printf("No vertex ID %s found\n", strtok(recv_buffer, "\n"));
      }
   }
} // receiveData

void sendData(int socket_Id) {
   // send the data to server
   int sent_count = send(socket_Id, (void *) &buffer, sizeof(buffer), 0);
   
   if(sent_count == -1) {
      exit(-1);
   }
   
   printf("The client has sent query to AWS using TCP: start vertex %d, destination vertex %d, map %s, file size %d\n", data.start_vertex, data.destination_vertex, data.map_ID, data.file_size);
} // sendData
