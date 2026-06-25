#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
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
int nfds = 0;
int id = 0;
char sendbuf[100000];
char recvbuf[100000];

typedef struct t_client {
  int id;
  char *buf;
} s_client;
struct pollfd fds[1024];
struct t_client clients[1034];

void broadcast(int clientfd) {
  for (int i = 0; i < nfds; i++) {
    int fd = fds[i].fd;
    if (fd != clientfd)
      send(fd, sendbuf, sizeof sendbuf,
           0); // It seems that it can't send to server.
  }
}

int main() {
  int sockfd, connfd;
  unsigned int len;
  struct sockaddr_in servaddr, cli;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  // check socket creation
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(2130706433);
  servaddr.sin_port = htons(8081);

  if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof servaddr) < 0) {
    close(sockfd);
    printf("Bad bind.\n");
    write(2, "Fatal error\n", 12);
    exit(1);
  }

  if (listen(sockfd, 128) != 0) {
    write(2, "Fatal error\n", 12);
    exit(1);
  }

  // int socket(int domain, int type, int protocol);
  // int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
  // int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

  fds[0].fd = sockfd;     // Standard input
  fds[0].events = POLLIN; // Tell me when ready to read
  nfds = 1;

  printf("listening...\n");

  while (1) {
    int num_events = poll(fds, nfds, -1);
    printf("num_events from poll: %d\n", num_events);
    if (num_events == -1) {
      perror("poll");
      exit(1);
    }

    for (int i = 0; i < nfds; i++) {
      // Check if someone's ready to read
      if (fds[i].revents & (POLLIN | POLLHUP))
        // We got one!!

        if (fds[i].fd == sockfd) {
          // If we're the listener, it's a new connection
          // handle new connection. Add to fds.
          len = sizeof(cli);
          connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
          if (connfd == -1) {
            perror("accept");
            exit(1);
          } else {
            fds[nfds].fd = connfd;
            fds[nfds].events = POLLIN;
            nfds++;
            clients[connfd].id = ++id;
            sprintf(sendbuf, "server: client %d just arrived\n",
                    clients[connfd].id);
            printf("server debug: client %d just arrived\n",
                   clients[connfd].id);
            broadcast(connfd);
          }
        } else { // There is a client sending.
          int nbytes = recv(fds[i].fd, recvbuf, sizeof recvbuf, 0);
          int sender_fd = fds[i].fd;
          if (nbytes <= 0) { // Got error or connection closed by client
            printf("server debug: client %d just left\n",
                   clients[sender_fd].id);
            sprintf(sendbuf, "server: client %d just left\n",
                    clients[sender_fd].id);
            broadcast(sender_fd);
            close(fds[i].fd);
            fds[i] = fds[nfds - 1];
            nfds--;
            i--;
          } else {
            // There is something to read.
            recvbuf[nbytes] = '\0';
            clients[sender_fd].buf = str_join(clients[sender_fd].buf, recvbuf);
            printf("server debug: recv from fd %d: %.*s", sender_fd, nbytes,
                   recvbuf);

            // Send to everyone!
            char *msg;
            while (extract_message(&clients[sender_fd].buf, &msg)) {
              sprintf(sendbuf, "client %d: %s", clients[sender_fd].id, msg);
              printf("server debug: recv from fd\n");
              broadcast(sender_fd);
              free(msg);
            }
            // for (int i = 0; i < nfds; i++) {
            //   int fd = fds[i].fd;
            //   if (fd != sender_fd && fd != sockfd) {
            //
            //     send(fd, sendbuf, strlen(sendbuf), 0);
          }
        }
    }
  }

  return 0;
}
