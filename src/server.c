#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<netdb.h>
#include<unistd.h>
#include<pthread.h>
#include<fcntl.h>

#include"server.h"
#include"device.h"
#include"print.h"
#define BUFFER_LEN 1024

void out_addr_port(struct sockaddr_in *addr){

	char ip[16];
	int port;
	port=ntohs(addr->sin_port);
	memset(ip, 0, 16);
	inet_ntop(AF_INET, &addr->sin_addr.s_addr, ip, sizeof(ip));

	printf("%s %d connected!\n", ip, port);
}

//初始化服务器
int init_socket(int port){
	
	int res;
	s_sockfd=socket(AF_INET, SOCK_STREAM, 0);
	if(s_sockfd<0){
		fprintf(stderr, "Socket:%s!\n", strerror(errno));
		exit(1);
	}
	struct sockaddr_in saddr;
	socklen_t slen=sizeof(saddr);
	memset(&saddr, 0, slen);
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(port);//端口号
	saddr.sin_addr.s_addr=INADDR_ANY;
	memset(&saddr.sin_zero, 0, sizeof(saddr.sin_zero));
	int val=1;
	setsockopt(s_sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));//?

	if(bind(s_sockfd, (struct sockaddr *)&saddr, slen)<0){
		fprintf(stderr, "bind:%s\n", strerror(errno));
		exit(1);
	}
/*	listen(s_sockfd, 10);//开始接受客户端请求，等待队列长度为10}*/
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	pthread_attr_init(&attr);
	return 0;
}

void *th_getframe(void *arg){

	while(1){
		if(clients>0){
			//已连接的客户端数
			////printf("clients:%d\n", clients);
			get_frame();
			pthread_mutex_lock(&mutex);
			memcpy(tmp_buf, buffer[okindex].start, buffer[okindex].length);
			pthread_mutex_unlock(&mutex);
			pthread_cond_broadcast(&cond);
			//usleep(80000);
		}else{
			if(!on_off){
				sleep(1);
				if(clients>0){
					cam_on();
				}else{
					//printf("camera is off, noclients\n");
					continue;
				}
			}else{
				int off_rest=5;//摄像头关闭剩余时间
				printf("there is no clients connected\n");
				printf("the camera will be trun off in %d secondes\n", off_rest);
				while(off_rest){
					printf("%d...\n", off_rest);
					sleep(1);
					if(clients>0){
						printf("client connected, camera trun off cancel\n");
						break;
					}else{
						off_rest--;//剩余时间减1
						if(off_rest==0){
							cam_off();
						}
					}
				}
			if(clients>0){
				continue;
			}
			if(on_off){
				get_frame();
			}
			}
		}
	}
}

static void th_exit(int sockfd){
	printf("-->client offline\n");
	printf("clients:%d\n", clients);
	close(sockfd);
	pthread_exit(NULL);
}

void *th_service(void *arg){
	pthread_t tid=pthread_self();
	int sockfd=(int)arg;

	struct sockaddr_in caddr;
	socklen_t clen=sizeof(caddr);
	memset(&caddr, 0, clen);
	getpeername(sockfd, (struct sockaddr*)&caddr, &clen);

	char head[BUFFER_LEN];
	
	size_t size;
	int action;
	size=read(sockfd, head, BUFFER_LEN);//head?
	if(size<0){
		fprintf(stderr, "read:%s\n", strerror(errno));
		return;
	}

	printf("---------------cilent_head----------------\n");
	fflush(stdout);
	if(write(STDOUT_FILENO, head, size)!=size){
		fprintf(stderr, "write1:%s\n", strerror(errno));
	}
	printf("----------------head_end------------------\n");
	if(strstr(head, "action=snap")!=NULL){
		printf("action:sanpshot\n");
		clients++;//已连接客户端线程数+1
		printf("clients:%d\n", clients);
		action=1;
	}else if(strstr(head, "action=stream")!=NULL){
		printf("action:stream\n");
		clients++;
		printf("clients:%d\n", clients);
		action=0;
	}else{
		printf("unknown action\n");

	}
	memset(head, 0, BUFFER_LEN);
	sprintf(head, "HTTP/1.0 200 OK\r\n"    //状态行
			"Connection:Keep-Alive\r\n"
			"Server: Network camera\r\n"
			"Cache-Control: no-store, must-revalidate, pre-check=0, max-age=0\r\n"
			"Pragma: no-cache\r\n"
			"Content-Type: multipart/x-mixed-replace;boundary=briup\r\n");
	printf("----------------server_head---------------\n");
	write(STDOUT_FILENO, head, strlen(head));
	if(write(sockfd, head, strlen(head))!=strlen(head)){
		fprintf(stderr, "write2: %s\n", strerror(errno));
	}
	printf("-----------------head_end-----------------\n");
	while(1){
		sleep(1);
	
	sprintf(head, "\r\n--briup\r\n"\
			"Content-Type:image/jpeg\n"\
			"Content-Length: %d\n\n", buffer[okindex].length+432);
	size=write(sockfd, head, strlen(head));
	if(size!=strlen(head)){
		break;
	}
	print_picture(sockfd, buffer[okindex].start, buffer[okindex].length);
	pthread_mutex_unlock(&mutex);

	if(!action){
		continue;
	}
	out_addr_port(&caddr);
	clients--;
	th_exit(sockfd);

	}
}
/*
void* th_fn(void *arg){
	pthread_detach(pthread_self());
	int sockfd=*(int *)arg;
	do_server(sockfd);
	close(sockfd);
}
void do_server(int sockfd){
	char head[BUFFER_LEN]={0};
	size_t size;
	size=read(sockfd, head, BUFFER_LEN);//head?
	if(size<0){
		fprintf(stderr, "read:%s\n", strerror(errno));
		return;
	}

	printf("---------------cilent_head----------------\n");
	fflush(stdout);
	if(write(STDOUT_FILENO, head, size)!=size){
		fprintf(stderr, "write1:%s\n", strerror(errno));
	}
	printf("----------------head_end------------------\n");
	memset(head, 0, BUFFER_LEN);
	sprintf(head, "HTTP/1.0 200 OK\r\n"    //状态行
			"Connection:Keep-Alive\r\n"
			"Server: Network camera\r\n"
			"Cache-Control: no-store, must-revalidate, pre-check=0, max-age=0\r\n"
			"Pragma: no-cache\r\n"
			"Content-Type: multipart/x-mixed-replace;boundary=briup\r\n");
	printf("----------------server_head---------------\n");
	write(STDOUT_FILENO, head, strlen(head));
	if(write(sockfd, head, strlen(head))!=strlen(head)){
		fprintf(stderr, "write2: %s\n", strerror(errno));
	}
	printf("-----------------head_end-----------------\n");
	while(1){
	usleep(100);
	sprintf(head, "\r\n--briup\r\n"\
			"Content-Type:image/jpeg\n"\
			"Content-Length: %d\n\n", buffer[okindex].length+432);
	size=write(sockfd, head, strlen(head));
td’
	print_picture(sockfd, buffer[okindex].start, buffer[okindex].length);

	}
}
*/
int lis_acc(int max_lis){

	listen(s_sockfd, 10);

	pthread_t th;
	int err;
	err=pthread_create(&th, &attr, th_getframe, NULL);
	if(err<0){
		fprintf(stderr, "pthread_create:%s\n", strerror(err));
		return -1;
	}

	struct sockaddr_in caddr;
	socklen_t clen=sizeof(caddr);
	int sockfd;
	while(1){
	memset(&caddr, 0, clen);
	//char head[BUFFER_LEN]={0};
	sockfd=accept(s_sockfd, (struct sockaddr *)&caddr, &clen);//获取连接请求并建立连接
	if(sockfd<0){
		fprintf(stderr, "accept:%s \n", strerror(errno));
	}else{
		out_addr_port(&caddr);
		printf("connected\tsockfd:%d clients:%d\n", sockfd, clients);
		fflush(stdout);
		err=pthread_create(&th, &attr, th_service, (void*)sockfd);
		if(err<0){
			fprintf(stderr, "pthread_create:%s\n", strerror(err));
			continue;
		}
	}
		//pthread_t tid;
	//	pthread_create(&tid, NULL, th_fn, (void*)&sockfd);
//		close(sockfd);

	/*	size_t size;
		size=read(sockfd, head, BUFFER_LEN);//head?
		if(size<0){
			fprintf(stderr, "read:%s\n", strerror(errno));
			return;
		}
	
		printf("---------------cilent_head----------------\n");
		fflush(stdout);
		if(write(STDOUT_FILENO, head, size)!=size){
			fprintf(stderr, "write:%s\n", strerror(errno));
		}
		printf("----------------head_end------------------\n");
		memset(head, 0, BUFFER_LEN);
		sprintf(head, "HTTP/1.0 200 OK\r\n"    //状态行
				"Connection:Keep-Alive\r\n"
				"Server: Network camera\r\n"
				"Cache-Control: no-store, must-revalidate, pre-check=0, max-age=0\r\n"
				"Pragma: no-cache\r\n"
				"Content-Type: multipart/x-mixed-replace;boundary=briup\r\n");
		printf("----------------server_head---------------\n");
		write(STDOUT_FILENO, head, strlen(head));
		if(write(sockfd, head, strlen(head))!=strlen(head)){
			fprintf(stderr, "write: %s\n", strerror(errno));
		}
		printf("-----------------head_end-----------------\n");
		while(1){
			sleep(1);
		sprintf(head, "\r\n--briup\r\n"\
				"Content-Type:image/jpeg\n"\
				"Content-Length: %d\n\n", buffer[okindex].length+432);
		size=write(sockfd, head, strlen(head));

		print_picture(sockfd, buffer[okindex].start, buffer[okindex].length);

		get_frame();
		}*/

	}
}


//卸载服务器
int uninit_socket(void){
	
	close(s_sockfd);//关闭服务端连接
	return 0;
}





