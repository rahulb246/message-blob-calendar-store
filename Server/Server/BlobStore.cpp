#include"stdafx.h"
int Blob_get_free_space(FILE *fblob){
	char bitvector[1024];
	fseek(fblob, BitVector_Offset, 0);
	fread(&bitvector, 1024, 1, fblob);
	for (int i = 0; i < 1024; i++){
		if (bitvector[i] == 0){
			bitvector[i] = 1;
			fseek(fblob, BitVector_Offset, 0);
			fwrite(&bitvector, 1024, 1, fblob);
			return File_Offset + (i*File_Size);
		}
	}
	return -1;
}

void AddFile(int *csock, char *filename, int userid){
	system("fsutil file createnew blobstore.bin 1073741824");
	FILE *fblob = fopen("blobstore.bin", "rb+");
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	struct BlobFile bf;
	int offset;
	fseek(fblob, BlobFile_Size*userid, 0);
	fread(&bf, sizeof(struct BlobFile), 1, fblob);
	if (bf.userid == 0){
		bf.userid = userid;
		bf.firstfileoffset = Blob_get_free_space(fblob);
		offset = bf.firstfileoffset;
		bf.filecount = 1;
		fseek(fblob, BlobFile_Size*userid, 0);
		fwrite(&bf, sizeof(struct BlobFile), 1, fblob);
		fflush(fblob);
	}
	else
	{
		offset = bf.firstfileoffset;
		int prevoffset;
		while (offset != 0){
			fseek(fblob, offset + 1023, 0);
			prevoffset = offset;
			fread(&offset, sizeof(int), 1, fblob);
		}
		offset = Blob_get_free_space(fblob);
		fseek(fblob, prevoffset + 8, 0);
		fwrite(&offset, sizeof(int), 1, fblob);
		fflush(fblob);
		bf.filecount++;
		fseek(fblob, BlobFile_Size*userid, 0);
		fwrite(&bf, sizeof(struct BlobFile), 1, fblob);
		fflush(fblob);
	}
	struct File file;
	memset(&file, '\0', sizeof(file));
	fseek(fblob, offset, 0);
	fread(&file, sizeof(file), 1, fblob);
	file.fileid = bf.filecount;
	strcpy(file.filename, filename);
	file.startoffset = (((offset - File_Offset) / File_Size)*Data_size) + Data_offset;
	if (bf.filecount == 1){
		file.nextfile = 0;
		fseek(fblob, offset, 0);
		fwrite(&file, sizeof(file), 1, fblob);
		fflush(fblob);
	}
	else
	{
		struct File file1;
		int nextoffset = file.nextfile;
		int prevoffset;
		while (nextoffset != 0){
			struct File file2;
			prevoffset = nextoffset;
			fseek(fblob, nextoffset, 0);
			fread(&file2, sizeof(int), 1, fblob);
			nextoffset = file2.nextfile;
		}
		fseek(fblob, prevoffset, 0);
		fread(&file1, sizeof(file1), 1, fblob);
		file1.nextfile = offset;
		fseek(fblob, offset, 0);
		fwrite(&file, sizeof(file), 1, fblob);
		fflush(fblob);
	}
	//file.startoffset=
	//send_data(csock, "length", Buff_SIZE);
	if ((bytecount = send(*csock, "length", strlen("length"), 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	char buff[1024];
	//recv_data(csock, length, 1024);
	memset(recvbuf, 0, recvbuf_len);
	if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	int len = atoi(recvbuf), count = 0;
	fseek(fblob, file.startoffset, 0);
	while (count<len)
	{
		if ((bytecount = send(*csock, "send_data", strlen("send_data"), 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
			free(csock);
			return;
		}
		memset(recvbuf, '\0', recvbuf_len);
		if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			free(csock);
			return;
		}
		fwrite(recvbuf, recvbuf_len, 1, fblob);
		count += 1024;
		fflush(fblob);
	}
	fclose(fblob);
}

void AddFileToBlob(int *csock, int userid){
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	if ((bytecount = send(*csock, "SendFile", strlen("Enter file name:"), 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	memset(recvbuf, 0, recvbuf_len);
	if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	AddFile(csock, recvbuf, userid);
}

void List_Files(int *csock, int userid){
	FILE *fblob = fopen("blobstore.bin", "rb+");
	if (fblob == NULL)
		return;
	struct BlobFile bf;
	fseek(fblob, BlobFile_Size*userid, 0);
	fread(&bf, sizeof(struct BlobFile), 1, fblob);
	char filenames[1024];
	sprintf(filenames, "The files are:\n");
	int filecount = bf.filecount;
	int offset = bf.firstfileoffset;
	int previd = 0;
	while (filecount>0)
	{
		struct File file;
		fseek(fblob, offset, 0);
		fread(&file, sizeof(file), 1, fblob);
		if (previd == file.fileid){
			strcat(filenames, file.filename);
			strcat(filenames, "\n");
			int prevoffset;
			while (previd == file.fileid)
			{
				prevoffset = file.nextfile;
				fseek(fblob, file.nextfile, 0);
				fread(&file, sizeof(file), 1, fblob);
			}
			offset = prevoffset;
		}
		else
		{
			strcat(filenames, file.filename);
			strcat(filenames, "\n");
			offset = file.nextfile;
		}
		previd = file.fileid;
		filecount--;
	}
	strcat(filenames, "Enter any key..");
	if ((send(*csock, filenames, strlen(filenames), 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
}

void DownLoadFile(int *csock, int userid){
	List_Files(csock, userid);
	char ack[1024] = "\0";
	if ((recv(*csock, ack, 1024, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	int bytecount;
	char downfilename[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	if ((bytecount = send(*csock, "Download", strlen("Download"), 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	if ((recv(*csock, ack, 1024, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	char buf[1024] = "Enter The File name to download:";
	if ((bytecount = send(*csock, buf, strlen(buf), 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	printf("\nreplied to client: %s\n", buf);
	memset(downfilename, 0, recvbuf_len);
	if ((recv_byte_cnt = recv(*csock, downfilename, recvbuf_len, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return;
	}

	FILE *fblob = fopen("blobstore.bin", "rb+");
	if (fblob == NULL)
		return;
	struct BlobFile bf;
	fseek(fblob, BlobFile_Size*userid, 0);
	fread(&bf, sizeof(struct BlobFile), 1, fblob);
	char data[1024] = "\0";
	int filecount = bf.filecount;
	int offset = bf.firstfileoffset;
	int previd = 0;
	while (filecount>0)
	{
		struct File file;
		fseek(fblob, offset, 0);
		fread(&file, sizeof(file), 1, fblob);
		if (previd == file.fileid){
			if (strcmp(downfilename, file.filename) == 0){
				int prevoffset;
				while (previd == file.fileid)
				{
					memset(&data, '\0', 1024);
					fseek(fblob, file.startoffset, 0);
					fread(&data, 1024, 1, fblob);
					if ((send(*csock, data, strlen(data), 0)) == SOCKET_ERROR){
						fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
						free(csock);
						return;
					}
					if ((recv(*csock, ack, 1024, 0)) == SOCKET_ERROR){
						fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
						free(csock);
						return;
					}
					prevoffset = file.nextfile;
					fseek(fblob, file.nextfile, 0);
					fread(&file, sizeof(file), 1, fblob);
				}
				break;
				offset = prevoffset;
			}
		}
		else
		{
			if (strcmp(downfilename, file.filename) == 0){
				memset(&data, '\0', 1024);
				fseek(fblob, file.startoffset, 0);
				fread(&data, 1024, 1, fblob);
				if ((send(*csock, data, strlen(data), 0)) == SOCKET_ERROR){
					fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
					free(csock);
					return;
				}
				if ((recv(*csock, ack, 1024, 0)) == SOCKET_ERROR){
					fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
					free(csock);
					return;
				}
				break;
				offset = file.nextfile;
			}
		}
		previd = file.fileid;
		filecount--;
	}
	if ((send(*csock, "Completed", 1024, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	if ((recv(*csock, ack, 1024, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
}

void deleteFile(int *csock, int userid){
	int bytecount;
	char downfilename[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	if ((send(*csock, "Not avaliable sorry for the inconvenience.\n ENter something and press enter", 1024, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	if ((recv(*csock, ack, 1024, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
}

void blob(int *csock, int userid){
	int bytecount;
	char buf[1024] = "";
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	while (true)
	{
		strcpy(buf, "The Options Are : \n0- Exit\n1 - Add files\n2 - View files\n3 - Download File\n4 - Delete File");
		if ((bytecount = send(*csock, buf, strlen(buf), 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
			free(csock);
			return;
		}
		printf("\nreplied to client: %s\n", buf);
		memset(recvbuf, 0, recvbuf_len);
		if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			free(csock);
			return;
		}
		switch (recvbuf[0])
		{
		case '0':
			return;
			break;
		case '1':
			AddFileToBlob(csock, userid);
			break;
		case '2':
			List_Files(csock, userid);
			break;
		case '3':
			DownLoadFile(csock, userid);
			break;
		case '4':
			deleteFile(csock, userid);
			break;
		default:
			break;
		}
	}
}