/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdbool.h>


char buffer[256];
int num_clients = 0;
int client_list[5];

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

bool SetSocketBlockingEnabled(int fd, bool blocking)
{
   if (fd < 0) return false;
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags < 0) return false;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}

int listen_for_new_clients(const int* sock_fd, const struct sockaddr_in *cli_addr){
    printf("Listening for new client\n");
    if (listen(*sock_fd, 5) == 0){
        socklen_t len = sizeof(*cli_addr);
        int client_fd = accept(*sock_fd,(struct sockaddr*) cli_addr, &len);
        if (client_fd < 0){
            error("ERROR on accept");
            return -1;
        }
        else{
            printf("New client registered \n");
            client_list[num_clients] = client_fd;
            num_clients++;
            return client_fd;
        }
    }
    return -1;
}

void send_and_receive_data(const int client_fd){
    char str[4] = "END";

    for (int i = 0 ; i<10; i++){
      bzero(buffer,256);
      int n = read(client_fd,buffer,255);
      if (n < 0) error("ERROR reading from socket");

      memcpy(str, buffer, strlen(str));
      str[3] = 0;
      if (strcmp(str, "END") == 0){
        printf("END received, exiting loop \n");
      }
      printf("str: %s \nstrlen: %lu\n",str, strlen(str));
      printf("Message: %s\nstrlen: %lu\n",buffer, strlen(buffer));
      printf("-------------------------------\n");

      n = write(client_fd,"I got your message",18);
      if (n < 0) error("ERROR writing to socket");
     }
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     //sockfd = SetSocketBlockingEnabled(sockfd, true);
     if (sockfd < 0)
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
              error("ERROR on binding");

     listen_for_new_clients(&sockfd, &cli_addr);

     for (int cnt = 0 ; cnt < 5 ; cnt++){
         for (int i = 0 ; i<num_clients ; i++){
             send_and_receive_data(client_list[i]);
         }
     }

     for (int i = 0 ; i<num_clients ; i++){
         printf("Closing client num :%d, id: %d", i, client_list[i]);
         close(client_list[i]);
     }

     close(sockfd);
     return 0;
}
