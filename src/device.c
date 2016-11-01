#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/mman.h>

#include<linux/videodev2.h>

#include"device.h"

//判断iotcl执行的结果并打印成功或失败信息
static void suc_err(int res, char *cmd_str){
	if(res<0){
		fprintf(stderr, "%s:%s\n", cmd_str, strerror(errno));
		exit(EXIT_FAILURE);
	}else{
		//printf("%s success\n", cmd_str);
	}
}

//初始化视频格式
static int init_format(void){
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;//数据流的类型
	fmt.fmt.pix.width=320;//图像的宽度
	fmt.fmt.pix.height=240;//图像的高度
	fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV;//彩色空间
//	fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_MJPEG;//彩色空间
	fmt.fmt.pix.field=V4L2_FIELD_INTERLACED;
	int res=ioctl(camera_fd, VIDIOC_S_FMT, &fmt);
	suc_err(res, "S_fmt");
	return 0;
}

//初始化内存映射
static int init_mmap(void){
	int res;
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.count=4;//缓存数量
	req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory=V4L2_MEMORY_MMAP;
	res=ioctl(camera_fd, VIDIOC_REQBUFS, &req);
	suc_err(res, "Req_bufs");

	buffer=calloc(req.count, sizeof(Videobuf));
	struct v4l2_buffer buf;
	for(bufs_num=0;bufs_num<req.count;bufs_num++){
		memset(&buf, 0, sizeof(buf));
		buf.index=bufs_num;//设置缓存索引号
		buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.field=V4L2_FIELD_INTERLACED;
		buf.memory=V4L2_MEMORY_MMAP;
		//读取缓存信息
		res=ioctl(camera_fd, VIDIOC_QUERYBUF, &buf);
		suc_err(res, "Query_buf");
		//设置缓存大小
		buffer[bufs_num].length=buf.length;
		//在堆空间中动态分配二级缓存空间
		tmp_buf=(char*)calloc(buffer[okindex].length, sizeof(char));
		//将设备文件的地址映射到用户空间的物理地址
		buffer[bufs_num].start=mmap(NULL, buf.length, 
				PROT_READ | PROT_WRITE, MAP_SHARED, camera_fd, buf.m.offset);
		if(buffer[bufs_num].start==MAP_FAILED){
			suc_err(-1, "Mmap");
		}else{
			suc_err(0, "Mmap");
		}
	}
	return 0;	
}

//初始化设备
int init_dev(void){
	init_format();//初始化格式
	init_mmap();//初始化内存映射
}

//卸载设备
int uninit_dev(void){
	int i;
	int res;
	for(i=0;i<bufs_num;i++){
		//断开内存映射
		res=munmap(buffer[i].start, buffer[i].length);
		suc_err(res, "Munmap");
	}
	free(buffer);//释放动态分配的内存空间
	free(tmp_buf);//释放二级缓存
	close(camera_fd);//关闭打开的设备文件描述符
}

//查找设备信息
int get_dev_info(void){
	int res;
	struct v4l2_capability cap;
	struct v4l2_format fmt;

	memset(&cap, 0, sizeof(cap));
	//获取当前设备的属性
	res=ioctl(camera_fd, VIDIOC_QUERYCAP, &cap);
	suc_err(res, "Query_cap");

	memset(&fmt, 0, sizeof(fmt));
	fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	//获取当前设备的输出格式
	res=ioctl(camera_fd, VIDIOC_G_FMT, &fmt);
	suc_err(res, "Gfmt");

	//获取当前设备的帧率
	struct v4l2_streamparm parm;
	memset(&parm, 0, sizeof(parm));
	parm.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	res=ioctl(camera_fd, VIDIOC_G_PARM, &parm);
	suc_err(res, "G_parm");

	printf("------------dev_info--------------\n");
	printf("driver:%s\n", cap.driver);
	printf("card:%s\n", cap.card);//摄像头的设备名
	printf("bus:%s\n", cap.bus_info);
	printf("width:%d\n", fmt.fmt.pix.width);//当前的图像输出宽度
	printf("heigth:%d\n", fmt.fmt.pix.height);//当前的图像输出宽度
	printf("FPS:%d\n", parm.parm.capture.timeperframe.denominator);
	printf("----------------------------------\n");
	return 0;
}

//打开摄像头
int cam_on(void){
	int i, res;
	enum v4l2_buf_type type;
	type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	res=ioctl(camera_fd, VIDIOC_STREAMON, &type);
	suc_err(res, "Stream_on");

	//进行一次缓存刷新
	struct v4l2_buffer buf;
	for(i=0;i<bufs_num;i++){
		memset(&buf, 0, sizeof(buf));
		buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory=V4L2_MEMORY_MMAP;
		buf.index=i;
		res=ioctl(camera_fd, VIDIOC_QBUF, &buf);
		suc_err(res, "Q_buf_init");
	}
	on_off=1;
	printf("cammera trun on\n");
	return 0;
}

//关闭摄像头
int cam_off(void){
	enum v4l2_buf_type type;
	type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int res=ioctl(camera_fd, VIDIOC_STREAMOFF, &type);
	suc_err(res, "Stream_off");
	on_off=0;
	printf("camera trun off\n");

	return 0;
}

//捕获图像
int get_frame(void){
	struct v4l2_buffer buf;
	int i=0, res;
	counter++;
	memset(&buf, 0, sizeof(buf));
	buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory=V4L2_MEMORY_MMAP;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(camera_fd, &readfds);
	struct timeval tv;//设置设备响应时间
	tv.tv_sec=2;//秒
	tv.tv_usec=0;//微秒
	while(select(camera_fd+1, &readfds, NULL, NULL, &tv)<=0){
		fprintf(stderr, "camera busy, Dq_buf time out\n");
		FD_ZERO(&readfds);
		FD_SET(camera_fd, &readfds);
		tv.tv_sec=1;
		tv.tv_usec=0;
	}
	res=ioctl(camera_fd, VIDIOC_DQBUF, &buf);
	suc_err(res, "Dq_buf");

	//buf.index表示已经刷新好的可用的缓存索引号
	okindex=buf.index;
	//更新缓存已用大小
	buffer[okindex].length=buf.bytesused;
	//第n次捕获图片：（第n回刷新整个缓存队列-第n个缓存被刷新）
	////printf("Image_%03d:(%d-%d)\n", counter, counter /bufs_num, kindex);
	
	//把图像放入缓存队列中（入队）
	res=ioctl(camera_fd, VIDIOC_QBUF, &buf);
	suc_err(res, "Q_buf");

	return 0;
}







