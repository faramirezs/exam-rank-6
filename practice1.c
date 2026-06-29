// And the main
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

int extract_message(char **buf, char **msg) {
  char *newbuf;
  int i;

  *msg = 0;
  if (*buf == 0)
    return (0);
  i = 0;
  while ((*buf)[i]) {
    if ((*buf)[i] == '\n') {
      newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
      if (newbuf == 0)
        return (-1);
      strcpy(newbuf, *buf + i + 1);
      *msg = *buf;
      (*msg)[i + 1] = 0;
      *buf = newbuf;
      return (1);
    }
    i++;
  }
  return (0);
}

char *str_join(char *buf, char *add) {
  char *newbuf;
  int len;

  if (buf == 0)
    len = 0;
  else
    len = strlen(buf);
  newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
  if (newbuf == 0)
    return (0);
  newbuf[0] = 0;
  if (buf != 0)
    strcat(newbuf, buf);
  free(buf);
  strcat(newbuf, add);
  return (newbuf);
}

typedef struct t_client {
  int id;
  char *buf;
} s_client;

struct pollfd fds[1024];
struct t_client clients[1024];
int id = 0;

int sockfd, connfd;
int nfds = 0;

char sendbuf[100000];
char recvbuf[100000];

void broadcast(int ownfd) {
  for (int i = 0; i < nfds; i++) {
    int clifd = fds[i].fd;
    if (clifd != ownfd && sockfd != clifd)
      send(clifd, sendbuf, sizeof sendbuf, 0);
  }
}
void fatal() { write(2, "Fatal error", 11); }

int main(int ac, char **av) {
  if (ac != 2)
    exit(1);
  // int sockfd, connfd;
  unsigned int len;
  struct sockaddr_in servaddr, cli;

  // socket create and verification
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("socket creation failed...\n");
    exit(0);
  } else
    printf("Socket successfully created..\n");
  bzero(&servaddr, sizeof(servaddr));

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
  servaddr.sin_port = htons(atoi(av[1]));

  // Binding newly created socket to given IP and verification
  if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) !=
      0) {
    printf("socket bind failed...\n");
    exit(0);
  } else
    printf("Socket successfully binded..\n");
  if (listen(sockfd, 120) != 0) {
    printf("cannot listen\n");
    exit(0);
  }
  fds[0].fd = sockfd;
  fds[0].events = POLLIN;
  nfds++;
  printf("Listening...\n");

  while (1) {
    int pollnum = poll(fds, nfds, -1);
    if (pollnum < 0)
      fatal();
    else {
      for (int j; j < nfds; j++) {
        if (fds[j].revents & (POLLIN | POLLHUP)) {
          // we have something
          if (fds[j].fd == sockfd) {
            // there is a new client
            len = sizeof(cli);
            connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
            if (connfd < 0) {
              printf("server acccept failed...\n");
              fatal();
            } else {
              fds[nfds].fd = connfd;
              fds[nfds].events = POLLIN;
              nfds++;
              clients[connfd].id = ++id;
              sprintf(sendbuf, "server: client %d just arrived\n",
                      clients[connfd].id);
              broadcast(connfd);
            }
          } else {
            int sender = fds[j].fd;
            int nbytes = recv(sender, recvbuf, sizeof recvbuf, 0);
            if (nbytes <= 0) {
              // Error or desconection
              close(fds[j].fd);
              fds[j] = fds[nfds - 1];
              nfds--;
              j--;
              sprintf(sendbuf, "server: client %d left", clients[sender].id);
              broadcast(sender);
            } else {
              // there is something to read.
              recvbuf[nbytes] = '\0';
              clients[sender].buf = str_join(clients[sender].buf, recvbuf);
              char *msg;
              while (extract_message(&clients[sender].buf, &msg)) {
                sprintf(msg, "client %d:", clients[sender].id);
                broadcast(sender);
                free(msg);
              }
            }

            // a client wants to send something.
          }
        }
      }
    }
  }
}
