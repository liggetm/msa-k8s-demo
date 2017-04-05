#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

const char HTMLBEGIN[] = "<html><body><H1>v1.0</H1>";
const char HTMLEND[] = "</body></html>";
const int bufsize = 1024;

int main( int argc, char *argv[] ) {
  int create_socket, new_socket;
  socklen_t addrlen;
  char *buffer = malloc(bufsize);
  struct sockaddr_in address;
  int n;
  struct ifreq ifr;
  const int HTMLBEGINSIZ = strlen(HTMLBEGIN);
  const int HTMLENDSIZ = strlen(HTMLEND);

  if ( argc != 2 ){
    printf("ERROR: No interface name specified!!");
    return -1;
  }

  n = socket(AF_INET, SOCK_DGRAM, 0);
  //Type of address to retrieve - IPv4 IP address
  ifr.ifr_addr.sa_family = AF_INET;
  //Copy the interface name in the ifreq structure
  strncpy(ifr.ifr_name , argv[1] , IFNAMSIZ - 1);
  ioctl(n, SIOCGIFADDR, &ifr);
  close(n);

  char *ip_addr = inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr);

  char response[ HTMLBEGINSIZ + strlen(ip_addr) + HTMLENDSIZ];
  strcpy(response, HTMLBEGIN);
  strcat(response, ip_addr);
  strcat(response, HTMLEND);

  if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0){
    printf("The socket was created\n");
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);

  if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0){
    printf("Binding Socket\n");
  } else {
    printf("ERROR: Failed to bind socket!!!\n");
    close(create_socket);
    return -1;
  }

  while (1) {
    if (listen(create_socket, 10) < 0) {
      perror("server: listen");
      exit(1);
    }

    if ((new_socket = accept(create_socket, (struct sockaddr *) &address, &addrlen)) < 0) {
      perror("server: accept");
      exit(1);
    }

    if (new_socket > 0){
      printf("The Client is connected...\n");
    }

    recv(new_socket, buffer, bufsize, 0);
    printf("%s\n", buffer);
    write(new_socket, "HTTP/1.1 200 OK\n", 16);
    write(new_socket, "Content-length: 46\n", 19);
    write(new_socket, "Content-Type: text/html\n\n", 25);
    write(new_socket, response, strlen(response));
    fprintf(stderr,"%s\n", response);
    close(new_socket);
  }
  close(create_socket);
  return 0;
}
