#ifndef _CAMERA_H_
#define _CAMERA_H_

int camera_fd;//设备文件描述符

//记录映射后的物理地址和内存长度
typedef struct{
	void *start;//映射后的物理地址首地址
	size_t length;//映射后的物理地址长度
}Videobuf;

Videobuf *buffer;

//记录缓存数量
int bufs_num;

//计数器，记录当前捕获了多少张图像
int counter;
//已经刷新好的缓存索引号
int okindex;
//二级缓存地址
char *tmp_buf;
//设备打开关闭状态
int on_off;

//初始化设备
extern int init_dev(void);

//卸载设备
extern int uninit_dev(void);

//查找设备信息
extern int get_dev_info(void);

//打开摄像头
extern int cam_on(void);

//关闭摄像头
extern int cam_off(void);

//捕获图像
extern int get_frame(void);

#endif
