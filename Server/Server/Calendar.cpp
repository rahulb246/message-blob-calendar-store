#include "stdafx.h"
#define bitvectorsize 100000
#define service_start_offset 100000
#define service_people_bit_vector_offset 100640
#define service_people_bit_vector_size 500
#define service_people_offset 101140
#define s_i_bitvector_start 140140
#define s_i_bitvector_size 550 //90+450
#define single_indirect_offset 140690
#define direct_offset 141050
using namespace std;
#include <iostream>
#include <ctime>
struct service_people
{
	int id;
	char name[32];
	char role[32];
	char phone_no[10];
};

struct service
{
	int ID;
	char servicename[32];
	int service_people[5];
	int single_indirect;
	int double_indirect;
};

char calbitvector[20000];
int service_peoples_count = 0;

struct datebooked
{
	int userid;
	char date[11];
};
struct sin_indirect_block{
	int direct[5];
};
char services[10][20] =
{
	"Doctors",
	"House Keeping",
	"Baby Sitting",
	"Electricians",
	"Plumbers",
	"Teachers",
	"Beauty Parlour",
	"Cable Operator",
	"Courier",
	"Psychiatrists"
};
int leap(int y)
{
	return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}




void get_booked_appointments(FILE *f, struct service ser, struct service_people ser_pep, char *buff){
	fseek(f, ser.ID * 50 * 180, 0);
	fseek(f, (ser_pep.id - 1) * 180, 1);
	char bitvec[180];
	char buffer[32];
	fread(&bitvec, 180, 1, f);
	time_t now = time(0);
	tm *ltm = localtime(&now);
	int i = (ltm->tm_yday % 180);
	while (i < 180){
		if (bitvec[i] == 1){
			sprintf(buffer, "%d-%d-%d", ltm->tm_mday, ltm->tm_mon, ltm->tm_yday);
			strcpy(buff, buffer);
		}
		if (i == 179)
			i = 0;
		else
			i++;
		if (i == ltm->tm_yday % 180){
			break;
		}
	}
}

int get_difference(int d1,int m1,int y1,int d2,int m2,int y2){
	tm a = { 0, 0, 0, d1, m1, y1 }; /* June 24, 2004 */
	tm b = { 0, 0, 0, d2, m2, y2 }; /* July 5, 2004 */
	
	time_t x = mktime(&a);
	time_t y = mktime(&b);
	if (x != (time_t)(-1) && y != (time_t)(-1))
	{
		double difference = difftime(y, x) / (60 * 60 * 24);
		return difference;
	}
	return 0;
}

void book_appointment(FILE *f, struct service ser, struct service_people ser_pep, int *csock){
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	char bitvec[180];
	char buff[1024];
	sprintf(buff, "\nEnter date to book appointment:(dd-mm-yyyy):");
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
	int date, mon, year;
	sscanf(recvbuf, "%d-%d-%d", &date,&mon,&year);
	fseek(f, ser.ID * 50 * 180, 0);
	fseek(f, (ser_pep.id - 1) * 180, 1);
	fread(&bitvec, 180, 1, f);
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	timeinfo->tm_year = year - 1900;
	timeinfo->tm_mon = mon - 1;
	timeinfo->tm_mday = date;
	mktime(timeinfo);
	int yday = timeinfo->tm_yday;
	time_t now = time(0);
	tm *ltm = localtime(&now);
	int i = (ltm->tm_yday % 180);
	if ((yday - ltm->tm_yday > 0) && (yday - ltm->tm_yday <= 180)){
		bitvec[yday % 180] = 1;
	}
	else
	{
		printf("Invalid Date");
	}
	fseek(f, (ser.ID) * 50 * 180, 0);
	fseek(f, (ser_pep.id - 1) * 180, 1);
	fwrite(&bitvec, 180, 1, f);
	fflush(f);
}

void view_services(char *buff){
	char buffer[5];
	for (int i = 0; i < 10; i++){
		sprintf(buffer, "%d:", i + 1);
		strcat(buff, services[i]);
		strcat(buff, "\n");
	}
}

int print_service_people(FILE *f,struct service ser,char *buff){
	struct service_people ser_pep;
	int j = 1, count = 0;
	char buffer[1024] = "";
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;

	for (int i = 0; i < 5; i++){
		if (ser.service_people[i] != NULL){
			fseek(f, ser.service_people[i], 0);
			//printf("%ld", ftell(f));
			fread(&ser_pep, sizeof(struct service_people), 1, f);
			sprintf(buffer, "\n%d:\t%s\t%s", j++, ser_pep.name, ser_pep.role);
			strcat(buff, buffer);
			count++;
		}
	}
	if (ser.single_indirect != 0){
		int dir_offset = 0;
		struct sin_indirect_block sib;
		int single_indir_blok[5];
		fseek(f, single_indirect_offset, 0);
		fseek(f, ser.ID * 9, 1);
		fread(&single_indir_blok, sizeof(int)* 9, 1, f);
		for (int i = 0; i < 9; i++){
			if (single_indir_blok[i] != 0){
				dir_offset = single_indir_blok[i];
				fseek(f, dir_offset, 0);
				struct sin_indirect_block sib;
				fread(&sib, sizeof(sib), 1, f);
				for (int i = 0; i < 50; i++){
					if (sib.direct[i] == NULL){
						fseek(f, sib.direct[i], 0);
						//printf("%ld", ftell(f));
						fread(&ser_pep, sizeof(struct service_people), 1, f);
						sprintf(buffer, "\n%d:\t%s\t%s", j++, ser_pep.name, ser_pep.role);
						strcat(buff, buffer);
						count++;
					}
				}
			}
		}
	}
	return count;
}

int get_free_space_for_people(FILE *f){
	char bitvector[service_people_bit_vector_size];
	fseek(f, service_people_bit_vector_offset, 0);
	fread(&bitvector, service_people_bit_vector_size*sizeof(char), 1, f);
	int freespace;
	for (int i = 0; i < service_people_bit_vector_size; i++){
		if (bitvector[i] == 0)
		{
			bitvector[i] = 1;
			freespace = service_people_offset + (i*sizeof(struct service_people));
			break;
		}
	}
	fseek(f, service_people_bit_vector_offset, 0);
	fwrite(&bitvector, service_people_bit_vector_size*sizeof(char), 1, f);
	fflush(f);
	return freespace;
}

int get_free_space_direct_people(FILE *f){
	char bitvector[s_i_bitvector_size];
	fseek(f, s_i_bitvector_start, 0);
	fread(&bitvector, s_i_bitvector_size, 1, f);
	int freespace;
	for (int i = 0; i < s_i_bitvector_size; i++){
		if (bitvector[i] == 0){
			bitvector[i] = 1;
			freespace = direct_offset + (i*sizeof(struct sin_indirect_block));
			break;
		}
	}
	fseek(f, s_i_bitvector_start, 0);
	fwrite(&bitvector, s_i_bitvector_size, 1, f);
	fflush(f);
	return freespace;
}

void add_service_people(FILE *f, struct service *ser, int *csock,int id){
	char buff[1024];
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	struct service_people ser_pep;
	ser_pep.id = id;
	sprintf(buff, "\nEnter Name,Role,Phone Number:(seperated by ',')");
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
	//scanf("%32[^,],%32[^,],%10[^\n]", ser_pep.name, ser_pep.role, ser_pep.phone_no);
	sscanf(recvbuf, "%32[^,],%32[^,],%10[^\n]", ser_pep.name, ser_pep.role, ser_pep.phone_no);
	fwrite(&ser_pep, sizeof(struct service_people), 1, f);
	fflush(f);
}

int menu_services(FILE *f, struct service ser,int offset, int *csock){
	char buff[1024];
	int bytecount;
	char recvbuf[1024];
	int recvbuf_len = 1024;
	int recv_byte_cnt;
	while (1)
	{
		int option, count = 0;
		sprintf(buff, "\n1-View Service People\n2-Add New one");
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
		int j = 0;
		switch (option)
		{
		case 1:
			return 1;
		case 2:
			for (int i = 0; i < 5; i++){
				if (ser.service_people[i] == NULL){
					int space = get_free_space_for_people(f);
					ser.service_people[i] = space;
					fseek(f, offset, 0);
					fwrite(&ser, sizeof(struct service), 1, f);
					fseek(f, space, 0);
					add_service_people(f, &ser, csock,i+1);
					count++;
					break;
				}
				j++;
			}
			if (count == 0){
				int dir_offset = 0;
				struct sin_indirect_block sib;
				int single_indir_blok[9];
				fseek(f, single_indirect_offset, 0);
				fseek(f, ser.ID * 9, 1);
				if (ser.service_people == NULL){
					fread(&single_indir_blok, sizeof(int)* 9, 1, f);
					single_indir_blok[0] = get_free_space_direct_people(f);
					dir_offset = single_indir_blok[0];
				}
				else
				{
					fread(&single_indir_blok, sizeof(int)* 9, 1, f);
					for (int i = 0; i < 9; i++){
						if (single_indir_blok[i] == 0){
							single_indir_blok[i] = get_free_space_direct_people(f);
							dir_offset = single_indir_blok[i];
							break;
						}
						j += 9;
					}
				}
				fseek(f, dir_offset, 0);
				//struct sin_indirect_block sib;
				fread(&sib, sizeof(sib), 1, f);
				for (int i = 0; i < 5; i++){
					if (sib.direct[i] == NULL){
						int space = get_free_space_for_people(f);
						sib.direct[i] = space;
						fseek(f, dir_offset, 0);
						fwrite(&sib, sizeof(struct sin_indirect_block), 1, f);
						fseek(f, space, 0);
						add_service_people(f, &ser, csock, j + 1);
						count++;
						break;
					}
					j++;
				}
				fseek(f, offset, 0);
				fwrite(&ser, sizeof(struct service), 1, f);
				fflush(f);
			}
			//print_service_people(f, ser, buff);
			return 2;
		default:
			break;
		}
	}
}



void calendarstore(int *csock, int current_user_id){
	while (1)
	{
		system("cls");
		char buff[1024];
		int bytecount;
		char recvbuf[1024];
		int recvbuf_len = 1024;
		int recv_byte_cnt;
		int err = system("fsutil file createnew calendar.bin 104857600");
		FILE* f = fopen("calendar.bin", "r+b");
		if (err == 0){
			fseek(f, service_start_offset, 0);
			for (int i = 0; i < 10; i++){
				struct service ser;
				memset(&ser, 0, sizeof(ser));
				ser.ID = i + 1;
				strcpy(ser.servicename, services[i]);
				for (int j = 0; j < 5; j++){
					ser.service_people[j] = NULL;
				}
				ser.single_indirect = 0;
				fwrite(&ser, sizeof(struct service), 1, f);
			}
		}

		sprintf(buff, "The Services are:\n");
		view_services(buff);
		strcat(buff, "\nSelect Services:");
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
		int offset = service_start_offset + (sizeof(struct service)*(option-1));
		fseek(f, offset, 0);
		struct service ser;
		fread(&ser, sizeof(struct service), 1, f);
		memset(&buff, 0, 1024);
		if(menu_services(f, ser,offset, csock)==2)
			continue;
		sprintf(buff, "\n\People are:");
		if (print_service_people(f, ser, buff) == 0){
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
		}
		strcat(buff, "\nEnter 0 to back\nEnter service People number to view appointments.\n");
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
			continue;
		struct service_people ser_pep;
		int j = 1, count = 0;
		int msgoffset;
		for (int i = 0; i < 5; i++){
			if (ser.service_people[i] != NULL){
				if (option == j){
					msgoffset = ser.service_people[i];
					fseek(f, ser.service_people[i], 0);
					//printf("%ld", ftell(f));
					fread(&ser_pep, sizeof(struct service_people), 1, f);
					count++;
					break;
				}
				j++;
			}
		}
		if (count == 0){
			int dir_offset = 0;
			struct sin_indirect_block sib;
			int single_indir_blok[9];
			fseek(f, single_indirect_offset, 0);
			fseek(f, ser.ID * 9, 1);
			fread(&single_indir_blok, sizeof(int)* 9, 1, f);
			for (int i = 0; i < 9; i++){
				if (single_indir_blok[i] != 0){
					dir_offset = single_indir_blok[i];
					fseek(f, dir_offset, 0);
					struct sin_indirect_block sib;
					fread(&sib, sizeof(sib), 1, f);
					for (int i = 0; i < 50; i++){
						if (sib.direct[i] != NULL){
							if (option == j){
								msgoffset = sib.direct[i];
								fseek(f, sib.direct[i], 0);
								//printf("%ld", ftell(f));
								fread(&ser_pep, sizeof(struct service_people), 1, f);
								count++;
								break;
							}
							j++;
						}
					}
				}
			}
		}
		printf("%s", ser_pep.name);
		if (count == 0){
			sprintf(buff, "Invalid selection");
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
			continue;
		}
		get_booked_appointments(f, ser, ser_pep, buff);
		strcat(buff, "\nEnter 0 to back\nEnter 1 to book appointment.\n");
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
			continue;
		if (option == 1)
			book_appointment(f, ser, ser_pep, csock);
	}
}