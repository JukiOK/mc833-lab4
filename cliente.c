#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <math.h>

#define MAXLINE 4096

int Socket(int family, int type, int flags){
	int sockfd;
	if((sockfd = socket(family, type, flags)) < 0){
		perror("socket error");
		exit(1);
	}
	return sockfd;
}

void Inet_pton(int family, char *src, void *dst){
	if(inet_pton(family, src, &dst) <= 0){
		perror("inet_pton error");
		exit(1);
	}
}

void Connect(int sockfd, struct sockaddr *addr, socklen_t addrlen){
	if(connect(sockfd, addr, addrlen) < 0){
		perror("connect error");
		exit(1);
	}
}

void Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout){
	select(nfds, readfds, writefds, errorfds, timeout);
}

int main(int argc, char **argv) {
	int    sockfd, n;
	char   recvline[MAXLINE + 1];
	char   error[MAXLINE + 1];
	struct sockaddr_in servaddr;
	struct sockaddr_in servaddr_returned;
	socklen_t servaddr_len = sizeof(servaddr_returned);

	if (argc != 3) {
		strcpy(error,"uso: ");
		strcat(error,argv[0]);
		strcat(error," Server's IP and/or port missing!");
		perror(error);
		exit(1);
	}

	// cria um endpoint para comunicação e retorna um file descriptor (sockfd) que referencia esse socket.
	sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	// printf("LOL");

	bzero(&servaddr, sizeof(servaddr)); // Apaga sizeof(servaddr) bytes de servaddr colocando '\0' em cada um dos bytes da estrutura do servidor que será preechida
	servaddr.sin_family = AF_INET; // Protocolo IPv4
	short port = atoi(argv[2]);
	servaddr.sin_port   = htons(port); // Recebe porta que vai conectar
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr); // Converte a string passada como argumento do programa na estrutura de formato IPv4 e preenche essa estrutura no campo sinaddr de servaddr

	// Conecta o socket referenciado por sockfd no endereco especificado por servaddr
	Connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	// Código adicionado
	// Atribui o endereco ao qual o sockfd esta associado ao buffer apontado por servaddr_returned de tamanho servaddr_len
	if (getsockname(sockfd, (struct sockaddr *) &servaddr_returned, &servaddr_len) < 0) {
		perror("get socket name error");
		exit(1);
	}

	char file[MAXLINE];
	fd_set rset;
	FD_ZERO(&rset);
	int maxfd1, send_end = 0;
	while (1) {
		if (!send_end)
		   FD_SET(fileno(stdin), &rset);
		FD_SET(sockfd, &rset);
		maxfd1 = fileno(stdin) > sockfd ? fileno(stdin) + 1 : sockfd + 1;
		Select(maxfd1, &rset, NULL, NULL, NULL);
		// Atividade no socket
		if (FD_ISSET(sockfd, &rset)){
			if ((n = read(sockfd , recvline , MAXLINE)) == 0 && !send_end) {
				perror("str_cli: server terminated prematurely");
				exit(0);
			} else if (n == 0 && send_end) {
				break; //success
			}
			recvline[n] = 0;
			fputs(recvline, stdout);
		}
		// Atividade na entrada padrão
		if (FD_ISSET(fileno(stdin), &rset) && !send_end) {
			if (fgets(file, MAXLINE, stdin) == NULL) {
				send_end = 1;
				FD_CLR(fileno(stdin), &rset);
				shutdown(sockfd, SHUT_WR);
				continue;
			}
			
			write(sockfd, file, strlen(file));
		}
	}

	exit(0);
}
