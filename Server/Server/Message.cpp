#include"stdafx.h"
void create_user(FILE *f,int *csock){
	struct User user;
	fseek(f, bitvectorsize*sizeof(int), 0);
	int previd=0;
	do{
		user.id = previd + 1;
		fread(&previd, sizeof(int), 1, f);
		fseek(f, sizeof(struct User) - 4, 1);
	} while (previd != 0);
	char sendbuff[1024] = "";
	int bytecount;
	sprintf(sendbuff, "Enter name to register:");
	if ((bytecount = send(*csock, sendbuff, 1024, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	char recvbuf[1024];
	int recv_byte_cnt, recvbuf_len=1024;
	memset(recvbuf, 0, recvbuf_len);
	if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return;
	}
	//scanf(" %[^\n]s", name);
	strcpy(user.username, recvbuf);
	for (int i = 0; i < 5; i++)
		user.ctgs[i] = 0;
	fseek(f, bitvectorsize*sizeof(int)+(sizeof(struct User)*(user.id-1)), 0);
	fwrite(&user, sizeof(struct User), 1, f);
	fflush(f);
}

int print_users(FILE *f,char *buff){
	fseek(f, bitvectorsize*sizeof(int), 0);
	char buffer[128];
	int count = 0;
	for (int i = 0; i < 20; i++){
		struct User user;
		fread(&user, sizeof(struct User), 1, f);
		if (user.id != 0){
			sprintf(buffer,"%d\t%s\n", user.id, user.username);
			strcat(buff, buffer);
			count++;
		}
	}
	return count;
}

void create_category(FILE *f, int catogry,int userid,int catid,int *csock){
	char buff[1024] = "";
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	fseek(f, catogry, 0);
	struct Category_inode ctg;
	ctg.userid = userid;
	ctg.category_id = catid*userid;
	printf("\nEnter catogiry name:");
	strcat(buff, "\nEnter catogiry name:");
	if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
	//scanf(" %[^\n]s", ctg.name);
	strcpy(ctg.name, recvbuf);
	for (int i = 0; i < 50; i++){
		ctg.messages[i] = 0;
	}
	ctg.msg_single_indirect_block = NULL;
	ctg.size_of_indirect = NULL;
	fwrite(&ctg, sizeof(struct Category_inode), 1, f);
	fflush(f);
}

int print_catogiries(FILE *f, struct User user, int current_user_id,char *buff){
	int count = 0;
	char buffer[150];
	fflush(f);
	for (int i = 0; i < 5; i++){
		if (user.ctgs[i] != NULL){
			struct Category_inode cat;
			fseek(f, (long)user.ctgs[i], 0);
			fread(&cat, sizeof(struct Category_inode), 1, f);
			sprintf(buffer,"\n%d\t%s", cat.category_id, cat.name);
			strcat(buff, buffer);
			count++;
			//printf("\n%s", cat.name);
		}
	}
	return count;
}

int menu_catogiries(FILE *f, struct User user, int current_user_id,int *csock){
	char buff[1024] = "";
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	while (1)
	{
		strcat(buff, "\n1-View Catogiries\n2-Create Carogiry\n");
		if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
			free(csock);
			return -1;
		}
		//printf("\n1-View Catogiries\n2-Create Carogiry\n");
		int option;
		memset(recvbuf, 0, recvbuf_len);
		if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			free(csock);
			return -1;
		}
		option = atoi(recvbuf);//scanf("%d", &option);
		switch (option)
		{
		case 1:
			//printf("\nThe catogiries are:");
			//print_catogiries(f, user, current_user_id);
			return 1;
		case 2:
			for (int i = 0; i < 5; i++){
				if (user.ctgs[i] == NULL){
					fseek(f, ((bitvectorsize*sizeof(int)) + (current_user_id - 1)* sizeof(struct User)), 0);
					user.ctgs[i] = (int *)(bitvectorsize*sizeof(int)+20 * sizeof(struct User)) + ((user.id - 1) * 5 * sizeof(struct Category_inode)) + (i*sizeof(struct Category_inode));
					fwrite(&user, sizeof(struct User), 1, f);
					create_category(f, (int)user.ctgs[i], user.id,i+1,csock);
					//print_catogiries(f, user, current_user_id);
					break;
				}
			}
			return 2;
		default:
			break;
		}
	}
}

int search_category(FILE *f,struct User user, int catid){
	struct Category_inode cat;
	for (int i = 0; i < 5; i++){
		if (user.ctgs[i] != 0){
			struct Category_inode cat;
			fseek(f, (long)user.ctgs[i], 0);
			fread(&cat, sizeof(struct Category_inode), 1, f);
			if (cat.category_id==(catid*user.id)){
				return (int)user.ctgs[i];
			}
		}
	}
	return NULL;
}

int print_messages(FILE *f, struct Category_inode cat,int current_user_id,int *csock,char *buff){
	struct Message_inode msg;
	int j = 1, count = 0;
	char buffer[1024]="";
	//char sendbuff[1024] = "";
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	for (int i = 0; i < 50; i++){
		if (cat.messages[i] != NULL){
			fseek(f, cat.messages[i], 0);
			//printf("%ld", ftell(f));
			fread(&msg, sizeof(struct Message_inode), 1, f);
			sprintf(buffer,"\n%d:\nUser%d says:\t%s", j++,msg.userid,msg.msgtext);
			strcat(buff, buffer);
			count++;
		}
	}
	if (cat.msg_single_indirect_block != 0){
		do{
			strcat(buff, "\n\tNext->");
			strcat(buff, "\nEnter choice(n-Next,e-exit:");
			if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
				fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
				free(csock);
				return -1;
			}
			memset(recvbuf, 0, recvbuf_len);
			if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
				fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
				free(csock);
				return -1;
			}
			char choice;
			//scanf("%c", &choice);
			choice = recvbuf[0];
			if (choice == 'n'){
				strcpy(buff, "");
				int direct_offset = 0;
				struct single_indirect_block sib;
				int single_indir_blok[9];
				fseek(f, single_indirect_start_offset, 0);
				fseek(f, cat.category_id * cat.size_of_indirect, 1);
				fread(&single_indir_blok, sizeof(int)* 9, 1, f);
				for (int i = 0; i < cat.size_of_indirect; i++){
					if (single_indir_blok[i] != 0){
						direct_offset = single_indir_blok[i];
						fseek(f, direct_offset, 0);
						struct single_indirect_block sib;
						fread(&sib, sizeof(sib), 1, f);
						for (int i = 0; i < 50; i++){
							if (sib.directblocks[i] == NULL){
								fseek(f, sib.directblocks[i], 0);
								//printf("%ld", ftell(f));
								fread(&msg, sizeof(struct Message_inode), 1, f);
								sprintf(buffer,"\n%d:\nUser%d says:\t%s", j++, msg.userid, msg.msgtext);
								strcat(buff, buffer);
								count++;
							}
						}
						strcat(buff,"\n\tNext->");
						strcat(buff,"\nEnter choice(n-Next,e-exit:");
						if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
							fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
							free(csock);
							return -1;
						}
						memset(recvbuf, 0, recvbuf_len);
						if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
							fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
							free(csock);
							return -1;
						}
						//scanf("%c", &choice);
						choice = recvbuf[0];
						if (choice == 'e')
							break;
					}
				}
			}
			else if (choice == 'e')
			{
				return -1;
			}
		} while (1);
	}
	return count;
}

void create_message(FILE *f, struct Category_inode *cat, int userid,int *csock){
	char buff[1024];
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	struct Message_inode msg;
	msg.userid = userid; 
	sprintf(buff, "\nEnter Msg text:");
	if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
	//printf("\nEnter Msg text:");
	//fflush(stdin);
	strcpy(msg.msgtext,recvbuf);
	for (int i = 0; i < 50; i++){
		msg.replys[i] = 0;
	}
	msg.single_indirect_block = NULL;
	msg.size_of_indirect = NULL;
	fwrite(&msg, sizeof(struct Message_inode), 1, f);
	fflush(f);
}

int get_free_space(FILE *f){
	int bitvector[bitvectorsize];
	fseek(f, 0, 0);
	fread(&bitvector, bitvectorsize*sizeof(int), 1, f);
	int freespace;
	for (int i = 0; i < bitvectorsize; i++){
		if (bitvector[i] == 0)
		{
			bitvector[i] = 1;
			freespace= messegestartoffset + (i*sizeof(struct Message_inode));
			break;
		}
	}
	fseek(f, 0, 0);
	fwrite(&bitvector, bitvectorsize*sizeof(int), 1, f);
	fflush(f);
	return freespace;
}

int get_free_space_relpy(FILE *f){
	int bitvector[bitvectorsize/2];
	fseek(f, bitvectorsize / 2, 0);
	fread(&bitvector, bitvectorsize / 2 * sizeof(int), 1, f);
	int freespace;
	for (int i = 0; i < bitvectorsize; i++){
		if (bitvector[i] == 0)
		{
			bitvector[i] = 1;
			freespace = replystartoffset + ((bitvectorsize / 2+i) * sizeof(struct reply));
			break;
		}
	}
	fseek(f, bitvectorsize / 2, 0);
	fwrite(&bitvector, bitvectorsize / 2 * sizeof(int), 1, f);
	fflush(f);
	return freespace;
}

int get_free_space_direct(FILE *f){
	char bitvector[sbbitvectorsize];
	fseek(f, sbbitvectorstart, 0);
	fread(&bitvector, sbbitvectorsize, 1, f);
	int freespace;
	for (int i = 0; i < sbbitvectorsize; i++){
		if (bitvector[i] == 0){
			bitvector[i] = 1;
			freespace = directblocks_start_offset + (i*sizeof(struct single_indirect_block));
			break;
		}
	}
	fseek(f, sbbitvectorstart, 0);
	fwrite(&bitvector, sbbitvectorsize, 1, f);
	fflush(f);
	return freespace;
}

int menu_messages(FILE *f, struct Category_inode cat, int current_user_id,int offset,int *csock){
	char buff[1024];
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	while (1)
	{
		int option, count = 0;
		sprintf(buff,"\n1-View Messages\n2-Create Message");
		if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
			free(csock);
			return -1;
		}
		memset(recvbuf, 0, recvbuf_len);
		if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			free(csock);
			return -1;
		}
		option = atoi(recvbuf);//scanf("%d", &option);
		switch (option)
		{
		case 1:
			//printf("\nThe Messages are:\n");
			//print_messages(f, cat, current_user_id);
			return 1;
		case 2:
			for (int i = 0; i < 50; i++){
				if (cat.messages[i] == NULL){
					int space = get_free_space(f);
					cat.messages[i] = space;
					fseek(f, offset, 0);
					fwrite(&cat, sizeof(struct Category_inode), 1, f);
					fseek(f, space, 0);
					create_message(f, &cat, current_user_id,csock);
					count++;
					break;
				}
			}
			if (count == 0){
				int direct_offset = 0;
				struct single_indirect_block sib;
				int single_indir_blok[9];
				fseek(f, single_indirect_start_offset, 0);
				fseek(f, cat.category_id * cat.size_of_indirect, 1);
				if (cat.msg_single_indirect_block == NULL){
					cat.size_of_indirect = 9;
					fread(&single_indir_blok, sizeof(int)* 9, 1, f);
					single_indir_blok[0] = get_free_space_direct(f);
					direct_offset = single_indir_blok[0];
				}
				else
				{
					fread(&single_indir_blok, sizeof(int)* 9, 1, f);
					for (int i = 0; i < cat.size_of_indirect; i++){
						if (single_indir_blok[i] == 0){
							single_indir_blok[i] = get_free_space_direct(f);
							direct_offset = single_indir_blok[i];
							break;
						}
					}
				}
				fseek(f, direct_offset, 0);
				//struct single_indirect_block sib;
				fread(&sib, sizeof(sib), 1, f);
				for (int i = 0; i < 50; i++){
					if (sib.directblocks[i] == NULL){
						int space = get_free_space(f);
						sib.directblocks[i] = space;
						fseek(f, direct_offset, 0);
						fwrite(&sib, sizeof(struct single_indirect_block), 1, f);
						fseek(f, space, 0);
						create_message(f, &cat, current_user_id,csock);
						count++;
						break;
					}
				}
				fseek(f, offset, 0);
				fwrite(&cat, sizeof(struct Category_inode), 1, f);
				fflush(f);
			}
			return 2;
		default:
			break;
		}
	}
}

struct Category_inode delete_msg(FILE *f, int msgoffset, struct Category_inode cat, int offset){
	int bitvector[bitvectorsize / 2];
	fseek(f, 0, 0);
	fread(&bitvector, bitvectorsize / 2 * sizeof(int), 1, f);
	bitvector[(msgoffset - messegestartoffset) / sizeof(struct Message_inode)] = 0;
	int count = 0;
	for (int i = 0; i < 50; i++){
		if (cat.messages[i] == msgoffset){
			struct Message_inode msg;
			fseek(f, msgoffset, 0);
			fread(&msg, sizeof(struct Message_inode), 1, f);
			for (int j = 0; j < 50; j++){
				msg.replys[j] = 0;
			}
			cat.messages[i] = NULL;
			count++;
			break; 
		}
	}
	if (count == 0){
		int direct_offset = 0;
		struct single_indirect_block sib;
		int single_indir_blok[9];
		fseek(f, single_indirect_start_offset, 0);
		fseek(f, cat.category_id * cat.size_of_indirect, 1);
		fread(&single_indir_blok, sizeof(int)* 9, 1, f);
		for (int i = 0; i < cat.size_of_indirect; i++){
			if (single_indir_blok[i] != 0){
				direct_offset = single_indir_blok[i];
				fseek(f, direct_offset, 0);
				struct single_indirect_block sib;
				fread(&sib, sizeof(sib), 1, f);
				for (int i = 0; i < 50; i++){
					if (sib.directblocks[i] == msgoffset){
						struct Message_inode msg;
						fseek(f, msgoffset, 0);
						fread(&msg, sizeof(struct Message_inode), 1, f);
						for (int j = 0; j < 50; j++){
							msg.replys[j] = 0;
						}
						cat.messages[i] = NULL;
						count++;
						break;
					}
				}
				if (count!=0)
					break;
			}
		}
		fseek(f, offset, 0);
		fwrite(&cat, sizeof(struct Category_inode), 1, f);
		fflush(f);
	}
	fseek(f, 0, 0);
	fwrite(&bitvector, bitvectorsize / 2 * sizeof(int), 1, f);
	fseek(f, offset, 0);
	fwrite(&cat, sizeof(struct Category_inode), 1, f);
	fflush(f);
	return cat;
}

void print_replys(FILE *f,struct Message_inode msg,int current_user_id,int msgoffset,char *buff,int *csock){
	struct reply rly;
	int j = 1;
	char buffer[150];
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	for (int i = 0; i < 50; i++){
		if (msg.replys[i] != NULL){
			fseek(f, msg.replys[i], 0);
			//printf("%ld", ftell(f));
			fread(&rly, sizeof(struct reply), 1, f);
			sprintf(buffer,"\n%d:\nUser%d\t%s", j++,rly.userid, rly.rpltext);
			strcat(buff, buffer);
		}
	}
	strcat(buff,"\n\n1-reply\n2-back\n");
	if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
		return ;
	}
	memset(recvbuf, 0, recvbuf_len);
	if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return ;
	}
	int option, count = 0;
	option = atoi(recvbuf);//scanf("%d", &option);
	if (option == 1){
		struct reply rly;
		rly.userid = current_user_id;
		strcpy(buff,"\nEnter reply text:");
		if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
		//fflush(stdin);
		//scanf("%s", rly.rpltext);
		//scanf("%[^\n]s", rly.rpltext);
		//gets(rly.rpltext);
		strcpy(rly.rpltext, recvbuf);
		for (int i = 0; i < 50; i++){
			if (msg.replys[i] == NULL){
				int space = get_free_space_relpy(f);
				msg.replys[i] = space;
				fseek(f, msgoffset, 0);
				fwrite(&msg, sizeof(struct Message_inode), 1, f);
				fflush(f);
				fseek(f, space, 0);
				fwrite(&rly, sizeof(struct reply), 1, f);
				count++;
				fflush(f);
				break;
			}
		}
		if (count == 0){
			int direct_offset = 0;
			struct single_indirect_block sib;
			int single_indir_blok[9];
			fseek(f, single_indirect_start_offset+(900*4), 0);
			//fseek(f, msg. * msg.size_of_indirect, 1);
			if (msg.single_indirect_block == NULL){
				msg.size_of_indirect = 9;
				fread(&single_indir_blok, sizeof(int)* 9, 1, f);
				single_indir_blok[0] = get_free_space_direct(f);
				direct_offset = single_indir_blok[0];
			}
			else
			{
				fread(&single_indir_blok, sizeof(int)* 9, 1, f);
				for (int i = 0; i < msg.size_of_indirect; i++){
					if (single_indir_blok[i] == 0){
						single_indir_blok[i] = get_free_space_direct(f);
						direct_offset = single_indir_blok[i];
						break;
					}
				}
			}
			fseek(f, direct_offset, 0);
			//struct single_indirect_block sib;
			fread(&sib, sizeof(sib), 1, f);
			for (int i = 0; i < 50; i++){
				if (sib.directblocks[i] == NULL){
					int space = get_free_space_relpy(f);
					sib.directblocks[i] = space;
					fseek(f, msgoffset, 0);
					fwrite(&msg, sizeof(struct Message_inode), 1, f);
					fflush(f);
					fseek(f, space, 0);
					fwrite(&rly, sizeof(struct reply), 1, f);
					count++;
					fflush(f);
					break;
				}
			}
		}
		fflush(f);
		print_replys(f, msg, current_user_id, msgoffset,buff,csock);
	}
	//fclose(file);
}

void delete_replys(FILE *f, int rplyoffset, struct Message_inode msg, int msgoffset){
	int bitvector[bitvectorsize],count=0;
	fseek(f, 0, 0);
	fread(bitvector, sizeof(int), bitvectorsize, f);
	int index = ((rplyoffset - replystartoffset) / sizeof(struct reply));
	bitvector[index] = 0;
	for (int i = 0; i < 50; i++){
		if (msg.replys[i] == rplyoffset){
			msg.replys[i] = 0;
			count++;
			break;
		}
	}
	if (count == 0){
		int direct_offset = 0;
		struct single_indirect_block sib;
		int single_indir_blok[9];
		fseek(f, single_indirect_start_offset+(900*4), 0);
		//fseek(f, msg.category_id * cat.size_of_indirect, 1);
		fread(&single_indir_blok, sizeof(int)* 9, 1, f);
		for (int i = 0; i < msg.size_of_indirect; i++){
			if (single_indir_blok[i] != 0){
				direct_offset = single_indir_blok[i];
				fseek(f, direct_offset, 0);
				struct single_indirect_block sib;
				fread(&sib, sizeof(sib), 1, f);
				for (int i = 0; i < 50; i++){
					if (sib.directblocks[i] == rplyoffset){
						sib.directblocks[i] = 0;
						count++;
						break;
					}
				}
				if (count != 0)
					break;
			}
		}
	}
	fseek(f, 0, 0);
	fwrite(bitvector, sizeof(int), bitvectorsize, f);
	fseek(f, msgoffset, 0);
	fwrite(&msg, sizeof(struct Message_inode), 1, f);
	fflush(f);
}

void messages(FILE *f, int user_id,int current_user_id,struct Category_inode cat,int catogiry,int *csock){
	char buff[1024]="";
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	int menuoption = 1;
	if (user_id == current_user_id)
		menuoption = menu_messages(f, cat, current_user_id, catogiry,csock);
	if (menuoption == 1)
	{
		while (1)
		{
			sprintf(buff,"\n\nMessages are:");
			if (print_messages(f, cat, current_user_id,csock,buff) == 0){
				strcat(buff,"There are no messages. The sonething and press enter\n");
				if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
					fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
					free(csock);
					return;
				}
				int option;
				memset(recvbuf, 0, recvbuf_len);
				if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
					fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
					free(csock);
					return;
				}
				return;
			}
			strcat(buff,"\nEnter msg number to view msg. Enter 0 to Exit\n");
			if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
				fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
				free(csock);
				return;
			}
			int option;
			memset(recvbuf, 0, recvbuf_len);
			if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
				fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
				free(csock);
				return;
			}
			option = atoi(recvbuf);//scanf("%d", &option);
			if (option == 0)
				return;
			struct Message_inode msg;
			int j = 1, count = 0;
			int msgoffset;
			for (int i = 0; i < 50; i++){
				if (cat.messages[i] != NULL){
					if (option == j){
						msgoffset = cat.messages[i];
						fseek(f, cat.messages[i], 0);
						//printf("%ld", ftell(f));
						fread(&msg, sizeof(struct Message_inode), 1, f);
						count++;
						break;
					}
					j++;
				}
			}
			if (count == 0){
				int direct_offset = 0;
				struct single_indirect_block sib;
				int single_indir_blok[9];
				fseek(f, single_indirect_start_offset, 0);
				fseek(f, cat.category_id * cat.size_of_indirect, 1);
				fread(&single_indir_blok, sizeof(int)* 9, 1, f);
				for (int i = 0; i < cat.size_of_indirect; i++){
					if (single_indir_blok[i] != 0){
						direct_offset = single_indir_blok[i];
						fseek(f, direct_offset, 0);
						struct single_indirect_block sib;
						fread(&sib, sizeof(sib), 1, f);
						for (int i = 0; i < 50; i++){
							if (sib.directblocks[i] != NULL){
								if (option == j){
									msgoffset = sib.directblocks[i];
									fseek(f, sib.directblocks[i], 0);
									//printf("%ld", ftell(f));
									fread(&msg, sizeof(struct Message_inode), 1, f);
									count++;
									break;
								}
								j++;
							}
						}
					}
				}
			}
			sprintf(buff,"\n\nThe message:\t%d:\t%s\n", msg.userid, msg.msgtext);
			strcat(buff,"The replys are:\n");
			print_replys(f, msg, current_user_id, msgoffset,buff,csock);
			strcat(buff,"\nChoose:reply number to delete reply\n-1-To exit\n0-Delete Msg :");
			if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
			option = atoi(recvbuf);//scanf("%d", &option);
			if (option == 0)
				cat=delete_msg(f, msgoffset, cat, catogiry);
			else if (option == -1)
			{
				return;
			}
			else
			{
				struct reply rly;
				int j = 1;
				int rlyoffset;
				count = 0;
				for (int i = 0; i < 50; i++){
					if (msg.replys[i] != NULL){
						if (option == j){
							rlyoffset = msg.replys[i];
							fseek(f, rlyoffset, 0);
							//printf("%ld", ftell(f));
							fread(&rly, sizeof(struct reply), 1, f);
							count++;
							break;
						}
						j++;
					}
				}
				delete_replys(f, rlyoffset, msg, msgoffset);
			}
		}
	}
	else
	{
		printf("\n\n--No messages--Enter Something and press enter");
	}
}

int Login(FILE *f,int *csock){
	int current_user_id;
	char sendbuff[1024] = "";
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	sprintf(sendbuff, "The Avaliable users are:\n");
	if (print_users(f,sendbuff) == 0){
		strcat(sendbuff, "No users to login\n");
		//printf("No users to login\n");
		return -1;
	}
	strcat(sendbuff, "\nEnter your userid:");
	printf("%s", sendbuff);
	if ((bytecount = send(*csock, sendbuff, 1024, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		free(csock);
		return -1;
	}
	memset(recvbuf, 0, recvbuf_len);
	if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
		fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
		free(csock);
		return -1;
	}
	current_user_id = atoi(recvbuf);//scanf("%d", &current_user_id);
	return current_user_id;
}

void messagestore(int *csock, int current_user_id){
	while (1)
	{
		system("cls");
		FILE *f = fopen("message.bin", "r+b");
		fflush(f);
		char buff[1024];
		int bytecount;
		char recvbuf[1024];
		int recvbuf_len = 1024;
		int recv_byte_cnt;
		sprintf(buff,"\n\nchoose option:\n1-search by usernames\n2-search by catogiries\n0-Back\n");
		if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
			free(csock);
			return;
		}
		int option;
		memset(recvbuf, 0, recvbuf_len);
		if ((recv_byte_cnt = recv(*csock, recvbuf, recvbuf_len, 0)) == SOCKET_ERROR){
			fprintf(stderr, "Error receiving data %d\n", WSAGetLastError());
			free(csock);
			return;
		}
		option = atoi(recvbuf);//scanf("%d", &option);
		int userid;
		int catid, menuoption = 1, count = 0;
		struct User user;
		struct Category_inode cat;
		switch (option)
		{
		case 0:
			fclose(f);
			return;
		case 1:
			memset(buff, 0, 1024);
			print_users(f,buff);
			strcat(buff,"\nchoose user to view catogiries:");
			if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
			userid = atoi(recvbuf);//scanf("%d", &userid);
			system("cls");
			fseek(f, (bitvectorsize*sizeof(int)+(userid - 1) * sizeof(struct User)), 0);
			fread(&user, sizeof(struct User), 1, f);
			if (user.id == current_user_id)
				menuoption = menu_catogiries(f, user, current_user_id,csock);
			memset(buff, 0, 1024);
			if (print_catogiries(f, user, current_user_id,buff) <= 0){
				sprintf(buff,"There are no catogiries available.\nEnter something and press enter");
				if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
				break;
			}
			int catogiry;
			if (menuoption == 1){
				strcat(buff,"\nchoose catogiry id:");
				if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
				catid = atoi(recvbuf);//scanf("%d", &catid);
				catogiry = search_category(f, user, catid);
				if (catogiry != NULL){
					fseek(f, catogiry, 0);
					fread(&cat, sizeof(struct Category_inode), 1, f);
				}
				else
				{
					memset(buff, 0, 1024);
					strcat(buff, "\nCatogiry Not found");
					if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
					break;
				}
				system("cls");
				messages(f, user.id, current_user_id, cat, catogiry,csock);
			}
			break;
		case 2:
			memset(buff, 0, 1024);
			sprintf(buff,"The catogiries of All users:\n");
			count = 0;
			for (int i = 0; i < 20; i++){
				fseek(f, (bitvectorsize*sizeof(int)+(i* sizeof(struct User))), 0);
				fread(&user, sizeof(struct User), 1, f);
				count += print_catogiries(f, user, current_user_id,buff);
			}
			if (count == 0){
				strcat(buff,"There are no catogiries available.");
				if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
				break;
			}
			strcat(buff, "\nchoose catogiry id:");
			if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
			catid = atoi(recvbuf);//scanf("%d", &catid);
			system("cls");
			count = 0;
			for (int i = 0; i < 20; i++){
				fseek(f, (bitvectorsize*sizeof(int)+(i* sizeof(struct User))), 0);
				fread(&user, sizeof(struct User), 1, f);
				catogiry = search_category(f, user, catid);
				if (catogiry != NULL){
					fseek(f, catogiry, 0);
					fread(&cat, sizeof(struct Category_inode), 1, f);
					count++;
					break;
				}
			}
			if (count>0){
				system("cls");
				messages(f, user.id, current_user_id, cat, catogiry,csock);

			}
			else
			{
				memset(buff, 0, 1024);
				strcat(buff, "\nCatogiry Not found");
				if ((bytecount = send(*csock, buff, 1024, 0)) == SOCKET_ERROR){
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
				break;
			}
			break;
		default:

			break;
		}
	}
}