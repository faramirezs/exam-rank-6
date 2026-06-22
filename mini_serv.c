#include <stdio.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>




int main ()
{
	int sockfd, connfd;
	unsigned int len;
	struct sockaddr_in servaddr, cli;
	int nfds = 0;
	char sendbuf[100000];
	char recvbuf[100000];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	// check socket creation
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(8081);

	bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
	// bind checks.

	// int socket(int domain, int type, int protocol);
	// int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
	// int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

	struct pollfd fds[FD_SETSIZE]; // More if you want to monitor more

    fds[0].fd = sockfd;          // Standard input
    fds[0].events = POLLIN; 	// Tell me when ready to read
	nfds = 1;

	printf("listening...\n");

	// void broadcast(int fd) {
	// 	for (int i = 0; i < nfds; i++){
	// 		if (fd != fds[i].fd)
	// 			send(fds[i].fd);
	// 	}
	// }

	while (1){
		int num_events = poll(fds, nfds, -1);

		if (nfds == -1) {
            perror("poll");
            exit(1);
        }

		for (int i = 0; i < nfds; i++)
		{
			if(fds[i].revents & (POLLIN | POLLHUP) )
				if (fds[i].fd == sockfd){
					//handle new connection. Add to fds.
					len = sizeof(cli);
					connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
					if (connfd == -1) {
        				perror("accept");
					fds[nfds].fd = connfd;
					fds[nfds].events = POLLIN;
					nfds++;
					sprintf(sendbuf, "server: client %d just arrived\n", nfds);
					printf("server: client %d just arrived\n", nfds);
				}
				else {
					int nbytes = recv(fds[i].fd, recvbuf, sizeof recvbuf, 0);
    				int sender_fd = fds[i].fd;
					if (nbytes <= 0) { // Got error or connection closed by client

					//handle client data. read and broadcast.
				}

		}


		if (num_events == 0) {
			printf("Poll timed out!\n");
		} else {
			int pollin_happened = fds[0].revents & POLLIN;

			if (pollin_happened) {
				printf("File descriptor %d is ready to read\n",
						fds[0].fd);
			} else {
				printf("Unexpected event occurred: %d\n",
						fds[0].revents);
			}

		}
    }

    return 0;

}
