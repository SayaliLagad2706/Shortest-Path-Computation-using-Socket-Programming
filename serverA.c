#include<arpa/inet.h>
#include<ctype.h>
#include<math.h>
#include<netdb.h>
#include<netinet/in.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include <unistd.h> 

#define PORT 30953
#define CLIENT_PORT 33953

// structure to store map information
struct data {
      char line[1000];
};

struct data arr[40];
struct in_addr ip_addr;
struct sockaddr_in address, client_addr;

int count, start_vertex, destination_vertex;
char *map_ID, buffer[1024], data_buffer[4000], trans_speed[10], prop_speed[10];

void bindSocketToPort(int);
void checkAddrUse(int);
void clearBuffers();
void closeConnection(int);
void createAddresses();
int createSocket();
void formDataBuffer(bool);
char * readBufferData();
bool readFile();
bool readFileContent(FILE *, char *);
void receiveFromClient(int);
void sendDataToClient(bool, int);

int main() {
   printf("The server A is up and running using UDP on port %d\n", PORT);
   
   bool flag; 
   int socket_ID = createSocket();
   checkAddrUse(socket_ID);
   createAddresses();
   bindSocketToPort(socket_ID);
   
   while(true) {
      receiveFromClient(socket_ID);
      bool found_map_ID = readFile();
      
      // check if the required Map ID is present with Server A
      if(!found_map_ID) {
         printf("The Server A does not have the required graph id %s\n", map_ID);
         flag = false;
      }
      else {
         flag = true;
      }
      
      formDataBuffer(flag);
      sendDataToClient(flag, socket_ID);
      clearBuffers();
   }
   closeConnection(socket_ID);
}

void bindSocketToPort(int socket_ID) {
   int bind_status = bind(socket_ID, (struct sockaddr *) &address, sizeof(address));
   
   // check if binding is done successfully
   if(bind_status == -1) {
      exit(-1);
   }
} // bindSocketToPort

void checkAddrUse(int socket_ID) {
  int opt = 1;
  if(setsockopt(socket_ID, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
      exit(-1);
   }
} // checkAddrUse

void clearBuffers() {
   memset(data_buffer, 0, sizeof(data_buffer));
   memset(trans_speed, 0, sizeof(trans_speed));
   memset(prop_speed, 0, sizeof(prop_speed));
   memset(buffer, 0, sizeof(buffer));
   memset(map_ID, 0, sizeof(map_ID));
   memset(arr, 0, sizeof(arr));
} // clearBuffers

void closeConnection(int socket_ID) {
  int status = close(socket_ID);
  
  // check if connection is closed successfully
  if(status == -1) {
  }
} // closeConnection

void createAddresses() {
   // create an address for self
   ip_addr.s_addr = inet_addr("127.0.0.1");
   address.sin_family = AF_INET;
   address.sin_addr = ip_addr;
   address.sin_port = htons(PORT);
   
   // create client address
   client_addr.sin_family = AF_INET;
   client_addr.sin_addr = ip_addr;
   client_addr.sin_port = htons(CLIENT_PORT);
} // createAddresses

int createSocket() {
   // create a socket for connection with Server A over UDP
   int socket_ID = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   
   // if the socket isn't created properly, return an error
   if(socket_ID == -1) {
      exit(-1);
   }
   
   return socket_ID;
} // createSocket

void formDataBuffer(bool found) {
  // if map ID is found, store map data in buffer
  if(found) {
    strcat(data_buffer, prop_speed);
    strcat(data_buffer, trans_speed);
    for(int i = 0; i < count; i++) {
      strcat(data_buffer, arr[i].line);
    }
  }
  else {
    sprintf(data_buffer,"%s", "Graph Not Found\n");
  }
} // formDataBuffer

char* readBufferData() {
   char* recv_data = strtok(buffer, "\t"); // split the data to get individual information
   char* op[90]; 
   
   // store the split data into array 
   int i = 0;
   while(recv_data != NULL) {
      op[i] = recv_data;
      i++;
      recv_data = strtok(NULL, "\t");
   }
   
   map_ID = op[0];
   printf("The Server A has received input for finding graph of map %s\n", op[0]);
   return op[0];
} // readBufferData

bool readFile() {
   // open the map file and read its content
   char *op = readBufferData();
   
   FILE *fptr;
   char* filename = "map1.txt";
   fptr = fopen(filename, "r");

   if(fptr == NULL) {
      exit(-1);
   }
   
   bool res = readFileContent(fptr,  op);
   
   fclose(fptr);
   return res;
} // readFile

bool readFileContent(FILE *fptr, char *op) {
   char str[1000] = "";
   bool flag = false, found;
   int k = 0;
   count = 0;
   
   // read file content
   while(fgets(str, 1000, fptr) != NULL) {  
      // checks for Map ID 
      if(flag && isalpha(str[0])) {
         flag = false;
      }
      
      if(isalpha(str[0]) && strcmp(strtok(&str[0], "\n"), op) == 0) {
         flag = true;
         found = true;
         continue;
      }
      
      // if required map ID is found, store its data
      if(flag) {
         if(k == 0) {
            sprintf(prop_speed, "%s", str);
         }
         else if(k == 1) {
            sprintf(trans_speed, "%s", str);
         }
         else {
            sprintf(arr[count].line, "%s", str);
            count++;
         }
         k++;
      }
   }
   return found;
} // readFileContent

void receiveFromClient(int socket_ID) {
  int len = sizeof(client_addr);
  int n = recvfrom(socket_ID, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &len);
  
  // check if data is received successfully
  if(n == -1) {
    exit(-1);
  }
} // receiveFromClient

void sendDataToClient(bool flag, int socket_ID) {  
   int n = sendto(socket_ID, (char *) &data_buffer, sizeof(data_buffer), MSG_CONFIRM, (const struct sockaddr *) &client_addr, sizeof(client_addr));
   
   // check if data is sent successfully
   if(n == -1) {
    exit(-1);
   }
   
   if(flag) {
     printf("The Server A has sent Graph to AWS\n");
   }
   else {
     printf("The Server A has sent \"Graph Not Found\" to AWS\n");
   }
} // sendDataToClient
