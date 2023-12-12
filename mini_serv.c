#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct s_client {
	int id;
	char msg[1024];
} t_client;

t_client clients[1024];
char to_write[120000], to_read[120000];
int next = 0, max = 0;
fd_set write_set, read_set, active_set;




void err(char *str) {
	while(*str) 
		write(2, str++, 1);
	write(2, "\n", 1);
	exit(1);
}

void send_all(int fd) {
	for (int i = 0; i <= max; i++) {
		if (FD_ISSET(i, &write_set) && i != fd)
			send(i, to_write, strlen(to_write), 0);
	
	}
}

int main(int ac, char **av) {
	if (ac != 2) 
		err("Wrong number of arguments");

	FD_ZERO(&active_set);

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		err("Fatal error");

	FD_SET(sockfd, &active_set);
	bzero(&clients, sizeof(clients));
	struct sockaddr_in servaddr;
	max = sockfd;
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(av[1]));

	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		err("Fatal error");
	if (listen(sockfd, 10) != 0)
		err("Fatal error");

	socklen_t len;

	while(1) {
		read_set = write_set = active_set;

		if (select(max + 1, &read_set, &write_set, NULL, NULL) < 0)
			continue;
		for(int fd = 0; fd <= max; fd++) {
			if (FD_ISSET(fd, &read_set) && fd == sockfd) {
				int newsockfd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
				if (newsockfd < 0)
					err("Fatal error");
				max = max > newsockfd ? max : newsockfd;
				clients[newsockfd].id = next++;
				FD_SET(newsockfd, &active_set);
				sprintf(to_write, "server: client %d just arrived\n", clients[newsockfd].id);
				send_all(newsockfd);
				break;
			}
			if (FD_ISSET(fd, &read_set) && fd != sockfd) {
				int buf = recv(fd, to_read, 120000, 0);
				if (buf <= 0) {
					sprintf(to_write, "server: client %d just left\n", clients[fd].id);
					send_all(fd);
					FD_CLR(fd, &active_set);
					close(fd);
				} else {
					for (int i = 0, j = strlen(clients[fd].msg); i < buf; i++, j++) {
						clients[fd].msg[j] = to_read[i];
						if (clients[fd].msg[j] == '\n') {
							clients[fd].msg[j] = '\0';
							sprintf(to_write, "client %d: %s\n", clients[fd].id, clients[fd].msg);
							send_all(fd);
							bzero(&clients[fd].msg, sizeof(clients[fd].msg));
							j = -1;
						}
					}
					break;
				}
			}
		}
	}
	return 0;
}
