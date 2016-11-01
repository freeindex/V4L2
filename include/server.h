#ifndef	_SERVER_H
#define _SERVER_H

//服务器的sockfd
int s_sockfd;

//已连接的客户端数量
int clients;

pthread_mutex_t mutex;
pthread_cond_t cond;
pthread_attr_t attr;
//初始化服务器
extern int init_socket(int port);

//卸载服务器
extern int uninit_socket(void);
extern int lis_acc(int max_lis);


extern void do_service(int sockfd);

#endif

