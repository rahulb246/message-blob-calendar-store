#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
struct node
{
	char msg[128];
	int msg_id;
	node *next;
}*flist, *alist, *printid;

struct bufserv{

	int userId;
	int forumId;
	int msgId;
	int commentId;
	int choice;
	char *forumname;
	char msg[128];
}buf1;

bool flag = true;
int mid = 0;
int count1 = 0;
char *Data[100];
int count = 1;
int values[100];
DWORD WINAPI SocketHandler(void*);
void replyto_client(char *buf, int *csock);

void socket_server() {

	//The port you want the server to listen on
	int host_port = 1101;

	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0 || (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2)) {
		fprintf(stderr, "No sock dll %d\n", WSAGetLastError());
		goto FINISH;
	}

	//Initialize sockets and set options
	int hsock;
	int * p_int;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if (hsock == -1){
		printf("Error initializing socket %d\n", WSAGetLastError());
		goto FINISH;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if ((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1) ||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1)){
		printf("Error setting options %d\n", WSAGetLastError());
		free(p_int);
		goto FINISH;
	}
	free(p_int);

	//Bind and listen
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	/* if you get error in bind
	make sure nothing else is listening on that port */
	if (bind(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1){
		fprintf(stderr, "Error binding to socket %d\n", WSAGetLastError());
		goto FINISH;
	}
	if (listen(hsock, 10) == -1){
		fprintf(stderr, "Error listening %d\n", WSAGetLastError());
		goto FINISH;
	}

	//Now lets do the actual server stuff

	int* csock;
	sockaddr_in sadr;
	int	addr_size = sizeof(SOCKADDR);

	while (true){
		printf("waiting for a connection\n");
		csock = (int*)malloc(sizeof(int));

		if ((*csock = accept(hsock, (SOCKADDR*)&sadr, &addr_size)) != INVALID_SOCKET){
			printf("Received connection from %s", inet_ntoa(sadr.sin_addr));
			CreateThread(0, 0, &SocketHandler, (void*)csock, 0, 0);
		}
		else{
			fprintf(stderr, "Error accepting %d\n", WSAGetLastError());
		}
	}

FINISH:
	;
}

void send_data(int *csock, char *buf, int length){
	int bytecount;
	if ((bytecount = send(*csock, buf, length, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
	}
}

int recv_data(int *csock, char *recvbuf, int recvbuf_len){
	int recv_byte_cnt;
	memset(recvbuf, 0, recvbuf_len);
	if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return 0;
	}
	return recv_byte_cnt;
}

void process_input(char *recvbuf, int recv_buf_cnt, int* csock, int current_user_id)
{
	int bytecount;
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	while (1)
	{
		char buf[1024] = "Menu:\n0-Exit\n1-Blob Store\n2-Message Store\n3-CalenderStore\n\nChoose Option:";
		if ((bytecount = send(*csock, buf, strlen(buf), 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
			free(csock);
			return;
		}
		if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			free(csock);
			return;
		}
		if (recv_byte_cnt == 0)
			return;
		int switchon = atoi(recvbuf);
		switch (switchon)
		{
		case 0:
			return;
		case 1:
			blob(csock, current_user_id);
			break;
		case 2:
			messagestore(csock, current_user_id);
			break;
		case 3:
			calendarstore(csock, current_user_id);
			break;
		default:
			break;
		}
	}
	//replyto_client(replybuf, csock);
	//replybuf[0] = '\0';
}

void replyto_client(char *buf, int *csock) {
	int bytecount;

	if ((bytecount = send(*csock, buf, strlen(buf), 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
	}
	printf("\nreplied to client: %s\n", buf);
}

DWORD WINAPI SocketHandler(void* lp){
	int *csock = (int*)lp;
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	int current_user_id;
	int i = system("fsutil file createnew message.bin 104857600");
	FILE *f = fopen("message.bin", "r+b");
	while (1)
	{
		char buff[1024] = "", sendbuff[1024] = "";
		sprintf(sendbuff, "The Options are:\n");
		strcat(sendbuff, "0-Exit\n1-Register User\n2-Login\n");
		if ((bytecount = send(*csock, sendbuff, 1024, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
			//free(csock);
			return 0;
		}
		int opt;
		//scanf("%d", &opt);
		memset(recvbuf, 0, recvbuf_len);
		if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			//free(csock);
			return 0;
		}
		opt = atoi(recvbuf);
		switch (opt)
		{
		case 0:
			return 0;
			break;
		case 1:
			create_user(f,csock);
			break;
		case 2:
			current_user_id=Login(f,csock);
			if(current_user_id != -1){
				process_input(recvbuf, recv_byte_cnt, csock, current_user_id);
			}
			break;
		default:
			break;
		}
	}
	return 0;
}