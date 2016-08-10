// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <errno.h>

#define bitvectorsize 102400 //51200 messages + 51200 replys 
#define messegestartoffset 435712 //102400*4 + 1120 userssize + 24800 cat size
#define replystartoffset 17408000  //435712+ sizeof(Message_inode) i.e 340*51200
#define sbbitvectorstart 8499200//1740800 + 6758400 replys
#define sbbitvectorsize 461700//900+51200*9 direct blocks
#define single_indirect_start_offset 8960900
#define directblocks_start_offset 10807700


struct Message_inode
{
	char msgtext[128];
	int userid;
	int replys[50];
	int single_indirect_block;
	int size_of_indirect;
};
struct single_indirect_block{
	int directblocks[50];
};

struct Category_inode
{
	int category_id;
	char name[32];
	int userid;
	int messages[50];
	int msg_single_indirect_block;
	int size_of_indirect;
};
struct reply
{
	char rpltext[128];
	int userid;
};

struct User{
	int id;
	char username[32];
	int *ctgs[5];
};

struct BlobFile
{
	int userid;
	int firstfileoffset;
	int filecount;
};
struct File
{
	char unused[4];
	int fileid;
	int nextfile;
	int startoffset;
	char filename[32];
};

#define BlobFile_Size 12
#define BitVector_Offset 1024
#define File_Offset 2048
#define File_Size 48
#define Data_offset 51200
#define Data_size 1048576


void create_user(FILE *f, int *csock);
int Login(FILE *f, int *csock);
void messagestore(int *csock, int current_user_id);
void blob(int *csock, int userid);
void calendarstore(int *csock, int current_user_id);
//void replyto_client(char *buf, int *csock);
//void process_input(char *recvbuf, int recv_buf_cnt, int* csock, int current_user_id);
//void print_users(FILE *f, int *csock);


// TODO: reference additional headers your program requires here
