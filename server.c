/* server.c */
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "wrap.h"
#include <strings.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define MAXLINE 80
#define SERV_PORT 8000

int main(void)
{
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddr_len;
	int listenfd, connfd;
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN];
	int i, n;

    /* 打开一个网络通讯接口
    对于IPv4，family参数指定为AF_INET。
    对于TCP协议，type参数指定为SOCK_STREAM，表示面向流的传输协议。
    如果是UDP协议，则type参数指定为SOCK_DGRAM，表示面向数据报的传输协议。 */
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    /**
     * 使用setsockopt()设置socket描述符的选项SO_REUSEADDR为1，
     * 表示允许创建端口号相同但IP地址不同的多个socket描述符。
    */
    int opt = 1;
    setsockopt(listenfdm SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
    /**
     * 将参数sockfd和myaddr绑定在一起,
     * 使sockfd这个用于网络通讯的文件描述符监听myaddr所描述的地址和端口号。
    */
	Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

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
	printf("Accepting connections ...\n");
	while (1) {
		cliaddr_len = sizeof(cliaddr);
		connfd = Accept(listenfd,
				(struct sockaddr *)&cliaddr, &cliaddr_len);
        n = fork();
        if (n == -1) {
            perror("call to fork");
            exit(1);
        } else if ( n==0 ) {
            close (listenfd);
            while (1) {
				n = Read(connfd, buf, MAXLINE);
				if (n == 0) {
					printf("the other side has been closed.\n");
					break;
				}
				printf("received from %s at PORT %d\n",
				       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
				       ntohs(cliaddr.sin_port));

				for (i = 0; i < n; i++)
					buf[i] = toupper(buf[i]);
				Write(connfd, buf, n);
			}
			Close(connfd);
            exit(0);
        } else {
            Close(connfd);
        }
	}
}