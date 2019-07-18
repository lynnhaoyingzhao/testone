/*************************************************************************
#	 FileName	: server.c
#	 Author		: fengjunhui 
#	 Email		: 18883765905@163.com 
#	 Created	: 2018年12月29日 星期六 13时44分59秒
 ************************************************************************/

#ifndef _COMMON_H_
#define _COMMON_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sqlite3.h>
#include<sys/wait.h>
#include<signal.h>
#include<time.h>
#include<pthread.h>
#include<sys/stat.h>
#include<sqlite3.h>

#define STAFF_DATABASE 	 "staff_manage_system.db"  //数据库

#define USER_LOGIN 		0x00000000
#define USER_MODIFY 	0x00000001
#define USER_QUERY 		0x00000002

#define ADMIN_LOGIN 	0x10000000
#define ADMIN_MODIFY 	0x10000001
#define ADMIN_ADDUSER 	0x10000002
#define ADMIN_DELUSER 	0x10000004
#define ADMIN_QUERY 	0x10000008
#define ADMIN_HISTORY 	0x10000010

#define QUIT 			0x11111111

#define ADMIN 0
#define USER  1

#define NAMELEN 16
#define DATALEN 128

typedef struct staff_info{
    int  no;
    int  usertype;
    char name[NAMELEN];
    char passwd[8];
    int  age;
    char phone[NAMELEN];
    char addr[DATALEN];
    char work[DATALEN];
    char date[DATALEN];
    int level;
    double salary ;
	
}staff_info_t;

typedef struct {
    int  msgtype;
    int  usertype;
    char username[NAMELEN];
    char passwd[8];
    char recvmsg[DATALEN];
    int  flags;
	void *released;
    staff_info_t info;
}MSG;

typedef struct thread_data{
	int acceptfd;
	pthread_t thread;
    int state;
	MSG *msg; 
	void *prvi_data;
}thread_data_t;

typedef struct thread_node{
	thread_data_t data;
	struct thread_node *next;
}linklist, *plinklist;

#endif

