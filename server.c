/* server.c */
#include "wrap.h"
#include <strings.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define MAXLINE 80
#define SERV_PORT 8000

int main(int argc, char **argv)
{
	int i, maxi, maxfd, listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE];
	ssize_t n;
	fd_set rset, allset;
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN];
	socklen_t cliaddr_len;
	struct sockaddr_in	cliaddr, servaddr;

    /* 打开一个网络通讯接口
    对于IPv4，family参数指定为AF_INET。
    对于TCP协议，type参数指定为SOCK_STREAM，表示面向流的传输协议。
    如果是UDP协议，则type参数指定为SOCK_DGRAM，表示面向数据报的传输协议。 */
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT);

    /**
     * 将参数sockfd和myaddr绑定在一起,
     * 使sockfd这个用于网络通讯的文件描述符监听myaddr所描述的地址和端口号。
    */
	Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	/**
	 * listen 监听进来的连接，backlog的含义有点复杂，这里先简单的描述：
	 * 指定挂起的连接队列的长度，当客户端连接的时候，
	 * 服务器可能正在处理其他逻辑而未调用accept接受连接，
	 * 此时会导致这个连接被挂起，内核维护挂起的连接队列，
	 * backlog则指定这个队列的长度，accept函数从队列中取出连接请求并接收它，
	 * 然后这个连接就从挂起队列移除。
	 * 如果队列未满，客户端调用connect马上成功，
	 * 如果满了可能会阻塞等待队列未满。
	 * Linux的backlog默认是128，通常情况下，我们也指定为128即可。
	*/
	Listen(listenfd, 20);
    /**
     * 三方握手完成后，服务器调用accept()接受连接，
     * 如果服务器调用accept()时还没有客户端的连接请求，
     * 就阻塞等待直到有客户端连接上来。cliaddr是一个传出参数，
     * accept()返回时传出客户端的地址和端口号。
     * addrlen参数是一个传入传出参数（value-result argument），
     * 传入的是调用者提供的缓冲区cliaddr的长度以避免缓冲区溢出问题，
     * 传出的是客户端地址结构体的实际长度（有可能没有占满调用者提供的缓冲区）。
     * 如果给cliaddr参数传NULL，表示不关心客户端的地址。
    */

	maxfd = listenfd;		/* initialize */
	maxi = -1;			/* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;	/* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	for ( ; ; ) {
		rset = allset;	/* structure assignment */
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		if (nready < 0)
			perr_exit("select error");

		if (FD_ISSET(listenfd, &rset)) { /* new client connection */
			cliaddr_len = sizeof(cliaddr);
			connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);

			printf("received from %s at PORT %d\n",
			       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
			       ntohs(cliaddr.sin_port));

			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd; /* save descriptor */
					break;
				}
			if (i == FD_SETSIZE) {
				fputs("too many clients\n", stderr);
				exit(1);
			}

			FD_SET(connfd, &allset);	/* add new descriptor to set */
			if (connfd > maxfd)
				maxfd = connfd; /* for select */
			if (i > maxi)
				maxi = i;	/* max index in client[] array */

			if (--nready == 0)
				continue;	/* no more readable descriptors */
		}

		for (i = 0; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ( (n = Read(sockfd, buf, MAXLINE)) == 0) {
					/* connection closed by client */
					Close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} else {
					int j;
					for (j = 0; j < n; j++)
						buf[j] = toupper(buf[j]);
					Write(sockfd, buf, n);
				}

				if (--nready == 0)
					break;	/* no more readable descriptors */
			}
		}
	}
}
