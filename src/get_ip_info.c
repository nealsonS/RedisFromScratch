#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]){
  if (argc < 3){
    return EXIT_FAILURE;
  }

  const char *node = argv[1];
  const char *service = argv[2];

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  struct addrinfo *res, *p;

  // hints.flags = 0;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  // hints.protocol = 0;
  // hints.addrlen = NULL;

  int status = getaddrinfo(node, service, &hints, &res);

  for (p = res; p != NULL; p= p->ai_next){
    char ipstr[INET6_ADDRSTRLEN];

    void *addr;
    void *ipver;
  
    if (p->ai_family == AF_INET) {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4 -> sin_addr);
      ipver = "IPv4";
    } else {
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)p->ai_addr;
      addr = &(ipv6->sin6_addr);
      ipver = "IPv6";
    }

    // convert ip address from binary to text form
    inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
    printf("node: %s\nservice: %s\nip version: %s\nip address: %s", node, service, (char*) ipver, ipstr);
  }

  
  return(EXIT_SUCCESS);
}