#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<signal.h>
#include<fcntl.h>
#include<unistd.h>

#include"device.h"
#include"print.h"

void sig_handler(int signo){
	if(signo==SIGINT){
		if(on_off){
			cam_off();
		
		uninit_socket();
		uninit_dev();
		exit(0);
		}
	}
}


int main(int argc, char **argv){
	
	if(argc<3){
		fprintf(stderr, "-usage:%s[dev][port]\n", argv[0]);
		exit(1);
	}

	if(signal(SIGINT, sig_handler)==SIG_ERR){
		fprintf(stderr, "signal:%s\n", strerror(errno));
		exit(1);
	}
	if(signal(SIGPIPE, sig_handler)==SIG_ERR){
		fprintf(stderr, "signal:%s\n", strerror(errno));
		exit(1);
	}

	//清屏
	system("clear");

	fflush(stdout);

	camera_fd=open(argv[1], O_RDWR | O_NONBLOCK, 0);
	get_dev_info();
	init_dev();//初始化设备
	get_dev_info();

	cam_on();
	get_frame();
/*	
	int fd;
	fd=open("a.jpg", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd<0){
		perror("open");
	}
//	write(fd, buffer[okindex].start, buffer[okindex].length);
	print_picture(fd, buffer[okindex].start, buffer[okindex].length);
	close(fd);
*/
	init_socket(atoi(argv[2]));
	
	lis_acc(20);
	uninit_socket();
	cam_off();

	return 0;
}
