#include <winsock2.h>
#include "stdafx.h"
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#define Buff_SIZE 1024
//#include <errno.h>
#include <stdio.h>
#include <conio.h>
int loadBuff(FILE *file, char *buff){
	memset(buff, 0, Buff_SIZE);
	return fread(buff, Buff_SIZE, 1, file);
}
int sendFILE(int hsock){
	char buffer[1024];
	int buffer_len = 1024;
	int bytecount;
	int c = 0;
	memset(buffer, '\0', 1024);
	char filename[32];
	printf("Enter File name:");
	gets(filename);
	if ((bytecount = send(hsock, filename, Buff_SIZE, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		return 0;
	}
	printf("Sent bytes %d\n", bytecount);
	FILE *file = fopen(filename, "r");
	fseek(file, 0, 2);
	int len = ftell(file);
	char length[10];
	itoa(len, length, 10);
	memset(buffer, '\0', Buff_SIZE);
	if ((bytecount = recv(hsock, buffer, Buff_SIZE, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		return 0;
	}
	if ((bytecount = send(hsock, length, Buff_SIZE, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		return 0;
	}
	fseek(file, 0, 0);
	int count = -1;
	char buff[Buff_SIZE];

	while (count != 0)
	{
		memset(buffer, '\0', Buff_SIZE);
		if ((bytecount = recv(hsock, buffer, Buff_SIZE, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			return 0;
		}
		memset(buff, 0, Buff_SIZE);
		/*while ((count < len) && (c < Buff_SIZE))
		{
		buff[c] = fgetc(file);
		c++;
		count++;
		}
		if (c < len)
		c = 0;
		buff[c] = '/0';*/
		//count += strlen(buff);
		count = fread(buff, Buff_SIZE, 1, file);
		if ((bytecount = send(hsock, buff, Buff_SIZE, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
			return 0;
		}
		printf("Sent bytes %d\n", bytecount);
	}

	return 1;
}

int downloadFile(int hsock){
	char buffer[1024];
	int buffer_len = 1024;
	int bytecount;
	int c = 0;
	memset(buffer, '\0', 1024);
	char filename[32] = "\0";
	char ack[32] = "";
	//printf("Enter File name:");
	if ((bytecount = send(hsock, ack, Buff_SIZE, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		return 0;
	}
	memset(buffer, '\0', Buff_SIZE);
	if ((bytecount = recv(hsock, buffer, Buff_SIZE, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		return 0;
	}
	puts(buffer);
	gets(filename);
	if ((bytecount = send(hsock, filename, Buff_SIZE, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		return 0;
	}
	printf("Sent bytes %d\n", bytecount);
	FILE *file = fopen(filename, "w");
	int count = -1;
	char buff[Buff_SIZE];

	while (true)
	{
		memset(buffer, '\0', Buff_SIZE);
		if ((bytecount = recv(hsock, buffer, Buff_SIZE, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			return 0;
		}
		if (strcmp(buffer, "Completed") == 0){
			if ((bytecount = send(hsock, ack, Buff_SIZE, 0)) == SOCKET_ERROR){
				fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
				return 0;
			}
			break;
		}
		else
		{
			fwrite(&buffer, sizeof(buffer), 1, file);
			fflush(file);
		}
		if ((bytecount = send(hsock, ack, Buff_SIZE, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
			return 0;
		}
	}
	fclose(file);
	return 1;
}
int getsocket()
{
	int hsock;
	int * p_int;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if (hsock == -1){
		printf("Error initializing socket %d\n", WSAGetLastError());
		return -1;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if ((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1) ||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1)){
		printf("Error setting options %d\n", WSAGetLastError());
		free(p_int);
		return -1;
	}
	free(p_int);

	return hsock;
}

void socket_client()
{

	//The port and address you want to connect to
	int host_port = 1101;
	char* host_name = "127.0.0.1";

	//Initialize socket support WINDOWS ONLY!
	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0 || (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2)) {
		fprintf(stderr, "Could not find sock dll %d\n", WSAGetLastError());
		goto FINISH;
	}

	//Initialize sockets and set any options

	//Connect to the server
	struct sockaddr_in my_addr;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = inet_addr(host_name);

	//if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == SOCKET_ERROR ){
	//	fprintf(stderr, "Error connecting socket %d\n", WSAGetLastError());
	//	goto FINISH;
	//}

	//Now lets do the client related stuff
	char buffer[1024];
	int buffer_len = 1024;
	int bytecount;
	int c;
	int hsock = getsocket();
	//add error checking on hsock...
	if (connect(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == SOCKET_ERROR){
		fprintf(stderr, "Error connecting socket %d\n", WSAGetLastError());
		goto FINISH;
	}
	while (true)
	{
		system("cls");
		memset(buffer, '\0', buffer_len);
		if ((bytecount = recv(hsock, buffer, buffer_len, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			goto FINISH;
		}
		//printf("Recieved bytes %d\nReceived string \"%s\"\n", bytecount, buffer);
		if (strcmp(buffer, "SendFile") == 0){
			if ((sendFILE(hsock)) == 0)
				goto FINISH;
			continue;
		}
		if (strcmp(buffer, "Download") == 0){
			if ((downloadFile(hsock)) == 0)
				goto FINISH;
			continue;
		}
		printf("%s\n", buffer);
		memset(buffer, '\0', buffer_len);
		//printf("Enter your message to send here\n");
		fflush(stdin);
		gets(buffer);
		if ((bytecount = send(hsock, buffer, strlen(buffer), 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
			goto FINISH;
		}
		printf("Sent bytes %d\n", bytecount);
		if (buffer[0] == '0'){
			goto FINISH;
		}
	}
	if ((bytecount = recv(hsock, buffer, buffer_len, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		goto FINISH;
	}
	//closesocket(hsock);
FINISH:
	;
	closesocket(hsock);
}