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

#define PORT 32953
#define CLIENT_PORT 33953
#define INFINITE 400001

// srucure to store map data
struct lines {
   char line[1000];
};

// structire to store map data in terms of edges and vertices
struct data {
    int start_node, dest_node;
    float distance;
};

struct data arr[40];
struct lines edges[40];
struct in_addr ip_addr;
struct sockaddr_in address, client_addr;

int count, num_of_nodes, num_of_hops, start_vertex, destination_vertex, file_size, vertex_mapping[100], *vertex_to_index;
float trans_delay, prop_delay, path_length, **cost;
char map_ID[100], buffer[1024], path[10], data_buffer[4000], trans_speed[10], prop_speed[10];

void bindSocketToPort(int);
void calculateDelay(float []);
void checkAddrUse(int);
void clearBuffers();
void closeConnection(int);
void createAddresses();
int createSocket();
int dijkstrasAlgo();
void formDataBuffer();
void formGraph();
void formNodeMapping();
int getIndexWithLeastCost(float [], bool []);
void initialDataSetUp();
bool isQueueEmpty(bool []);
void readBufferData();
void receiveFromClient(int);
void sendDataToClient(int);

int main() {
   printf("The server C is up and running using UDP on port %d\n", PORT);
 
   bool flag;  
   int socket_ID = createSocket();
   checkAddrUse(socket_ID);
   createAddresses();
   bindSocketToPort(socket_ID);
 
   while(true) {
      receiveFromClient(socket_ID);
      readBufferData();
      formNodeMapping();
      initialDataSetUp();
      int pathLen = dijkstrasAlgo();
      formDataBuffer();
      sendDataToClient(socket_ID);
      clearBuffers();
   }
   closeConnection(socket_ID);
} // main

void bindSocketToPort(int socket_ID) {
   int bind_status = bind(socket_ID, (struct sockaddr *) &address, sizeof(address));
   
   // check if binding is done successfully
   if(bind_status == -1) {
      exit(-1);
   }
} // bindSocketToPort

void calculateDelay(float dist[]) {
   // calculate transmisson and propagation delays
   trans_delay = (num_of_hops)*(file_size/atof(trans_speed));
   prop_delay = dist[vertex_mapping[destination_vertex]]/atof(prop_speed);
   path_length = dist[vertex_mapping[destination_vertex]];
   
   printf("The Server C has finished the calculation:\nShortest Path: ");
   for(int i = num_of_hops; i > 0; i--) {
      printf("%d -- ", path[i]);
   }
   
   printf("%d\n\tShortest distance: %.2f km\n\tTransmission delay: %.2f s\n\tPropagation delay: %.2f s\n", path[0], dist[vertex_mapping[destination_vertex]], trans_delay, prop_delay);
   
} // calculateDelay

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
} // clearBuffers

void closeConnection(int socket_ID) {
  int status = close(socket_ID);
  
  // check is connection is closed successfully
  if(status == -1) {
  
  }
} // closeConnection

void createAddresses() {
   // create an address for self
   ip_addr.s_addr = inet_addr("127.0.0.1");
   address.sin_family = AF_INET;
   address.sin_addr = ip_addr;
   address.sin_port = htons(PORT);
   
   // create an address for client
   client_addr.sin_family = AF_INET;
   client_addr.sin_addr = ip_addr;
   client_addr.sin_port = htons(CLIENT_PORT);
} // createAddresses

int createSocket() {
   // create a socket for connection with Server AWS over UDP
   int socket_ID = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   
   // if the socket isn't created properly, return an error
   if(socket_ID == -1) {
      exit(-1);
   }
   
   return socket_ID;
} // createSocket

/* the code implemented below is taken from the algorithm mentioned on:
 * https://www.codewithc.com/dijkstras-algorithm-in-c/
 */
int dijkstrasAlgo() {  
   int prev[num_of_nodes], start, m;
   float dist[num_of_nodes], min, d;
   bool visited[num_of_nodes];
   
   memset(visited, false, sizeof(bool)*num_of_nodes);
   
   dist[vertex_mapping[start_vertex]] = 0;
   
   // initialize all nodes with distance of infintiy
   for(int i = 0; i < num_of_nodes; i++) {
      if(vertex_mapping[start_vertex] != i) {
         prev[i] = -1;
         dist[i] = INFINITE;
      }
   }

   // visit each vertex in the queue
   while(!isQueueEmpty(visited)) {
      start = getIndexWithLeastCost(dist, visited);
      visited[vertex_mapping[start]] = true;
      
      // visit each neighbour of the popped vertex
      for(int i = 0; i < num_of_nodes; i++) {
         if(cost[vertex_mapping[start]][i] != 0) {
            d = dist[vertex_mapping[start]] + cost[vertex_mapping[start]][i];
            if(d < dist[i]) {
               dist[i] = d;
               prev[i] = start;
            }
         }
      }
   }
   
   int node = destination_vertex;
   num_of_hops = 0;
   
   // backtrack to get source to destination path
   while(node != start_vertex) {
      path[num_of_hops] = node;
      node = prev[vertex_mapping[node]];
      num_of_hops++;
   }
   path[num_of_hops] = start_vertex;
  
   calculateDelay(dist);
   //printf("Number of nodes: %d\n", num_of_nodes);
   /*for(int i = 0; i < num_of_nodes; i++) {
      printf("dist[%d]: %f\t prev[%d]: %d\n", vertex_to_index[i], dist[i], vertex_to_index[i], prev[i]);
   }*/
} // dijkstrasAlgo

void formDataBuffer() {
   char shortest_dist[10], trans_del[10], prop_del[10];
   
   sprintf(shortest_dist, "%f\n", path_length);
   sprintf(trans_del, "%f\n", trans_delay);
   sprintf(prop_del, "%f\n", prop_delay);
   
   for(int i = num_of_hops; i > 0; i--) {
      char p[10];
      snprintf(p, sizeof(p), "%d -- ", path[i]);
      strcat(data_buffer, p);
   }
   char p[10];
   snprintf(p, sizeof(p), "%d\n", path[0]);
   
   strcat(data_buffer, p);
   strcat(data_buffer, shortest_dist);
   strcat(data_buffer, trans_del);
   strcat(data_buffer, prop_del);
} // formDataBuffer

void formGraph() {
   // store the graph vertices and edges in structure
   for(int i = 0; i < count; i++) {
         char *line = edges[i].line;
         line = strtok(line, " ");
         int k = 0;
         
         while(line != NULL) {
            if(k == 0) {
               arr[i].start_node = atoi(line);
            }
            else if(k == 1) {
               arr[i].dest_node = atoi(line);
            }
            else {
               arr[i].distance = atof(line);
            }
            k++;
            line = strtok(NULL, " ");
         }
   }
} // formGraph

void formNodeMapping() {
   memset(vertex_mapping, -1, 100*sizeof(int));
   num_of_nodes = 0;
   
   // index map vertices in sequential order
   for(int i = 0; i < count; i++) {
      int s = arr[i].start_node, d = arr[i].dest_node;
      
      if(vertex_mapping[s] == -1) {
         vertex_mapping[s] = num_of_nodes;
         num_of_nodes++;
      }
      
      if(vertex_mapping[d] == -1) {
         vertex_mapping[d] = num_of_nodes;
         num_of_nodes++;
      }
   }
   
   vertex_to_index = malloc(sizeof(int*)*num_of_nodes);
   for(int i = 0; i < 100; i++) {
      if(vertex_mapping[i] > -1) {
         vertex_to_index[vertex_mapping[i]] = i;
      }
   }
} // formNodeMapping

int getIndexWithLeastCost(float dist[], bool visited[]) {
   float min = INFINITE;
   int index = -1;
   
   // search for unvisited vertex with least cost
   for(int i = 0; i < num_of_nodes; i++) {
      if(min > dist[i] && !visited[i]) {
         min = dist[i];
         index = vertex_to_index[i];
      }
   }
   return index;
} // getIndexWithLeastCost

void initialDataSetUp() {
 
   cost = malloc(sizeof(int*) * num_of_nodes);
   for(int i = 0; i < num_of_nodes; i++) {
      cost[i] = malloc(sizeof(int*) * num_of_nodes);
   }
   
   // store edge weights for each vertex
   for(int i = 0; i < count; i++) {
      int node1 = vertex_mapping[arr[i].start_node];
      int node2 = vertex_mapping[arr[i].dest_node];
      cost[node1][node2] = cost[node2][node1] = arr[i].distance;
   }
} // initialDataSetup

bool isQueueEmpty(bool visited[]) {
   for(int i = 0; i < num_of_nodes; i++) {
      if(!visited[i]) {
         return false;
      }
   }
   return true;
} // isQueueEmpty

void readBufferData() {
   char* recv_data = buffer; 
   
   // store the split data into array 
   int i = 0, j = 0;
   recv_data = strtok(recv_data, "\n");
   while(recv_data != NULL) {
      if(i == 0) {
         sprintf(map_ID, "%s", recv_data);
      }
      else if(i == 1) {
         start_vertex = atoi(recv_data);
      }
      else if(i == 2) {
         destination_vertex = atoi(recv_data);
      }
      else if(i == 3) {
         file_size = atoi(recv_data);
      }
      else if(i == 4) {
         sprintf(prop_speed, "%s", recv_data);
      }
      else if(i == 5) {
         sprintf(trans_speed, "%s", recv_data);
      }
      else {
         sprintf(edges[j].line, "%s", recv_data);
         j++;
      }
      i++;
      recv_data = strtok(NULL, "\n");
   }
   count = j;
  
   printf("The Server C has received data for calculation:\n\t * Propagation speed: %s km/s\n\t * Transmission speed: %s KB/s\n\t * Map ID: %s\n\t * Source ID: %d\t Destination ID: %d\n", prop_speed, trans_speed, map_ID, start_vertex, destination_vertex);
   
   formGraph();
} // readBufferData

void receiveFromClient(int socket_ID) {
  int len = sizeof(client_addr);
  int n = recvfrom(socket_ID, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &len);
  
  // check if data is received successfully
  if(n == -1) {
    exit(-1);
  }
} // receiveFromClient

void sendDataToClient(int socket_ID) {  
   int n = sendto(socket_ID, (char *) &data_buffer, sizeof(data_buffer), MSG_CONFIRM, (const struct sockaddr *) &client_addr, sizeof(client_addr));
   
   // check if data is sent successfully
   if(n == -1) {
    exit(-1);
   }
   
   printf("The Server C has finished sending the output to AWS\n");
} // sendDataToClient
