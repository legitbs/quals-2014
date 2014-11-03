#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
//#define NUMBER 500000
#define NUMBER 500000000
#define MODNUMBER 500
//5GB? files, thas large
//hashfile will be 79M?

int main(void) {
	signal(SIGINT,exit);
	alarm(10);
	FILE *fp1 = NULL;
	FILE *fp2 = NULL;
	FILE *fp3 = NULL;
	int x,y;
	int flagPositions[] = {58,63,141,186,404,553,568,1042,1655,1694,1997,2174,
		2819,2919,3183,3460,3914,4256,4470,4522,5062,5238,5627,5977,6002,6163};	
	unsigned long int strSize1, strSize2;
	char *tempStr1 = NULL;
	char *tempStr5 = NULL;
	char *tempStr2 = NULL;
	char *tempStr4 = NULL;
	unsigned char *tempStr3 = NULL;
	char *tempChar = NULL;
	unsigned char *shaStr = NULL;

	fp1 = fopen("uselesstempfileone","w");
	fp2 = fopen("uselesstempfiletwo","w");
	fp3 = fopen("uselesstempfilethree","w");
	tempStr1 = new char[14];
	tempChar = new char[2];
	printf("'dis is sooo bad!\n");
	for (x=0;x<NUMBER;x++) {
		sprintf(tempStr1,"%08x",x);
		printf("Writing %s to uselesstempfileone (this code makes me feel dirty)\n",tempStr1);
		fgets(tempChar,2,stdin);
		fprintf(fp1,"0x%08x",x);
		printf("Done Writing %s to uselesstempfileone\nI shouldn't have outsourced this.\n",tempStr1);
		fflush(fp1);
	}
	fclose(fp1);
	puts("well that was slow");
	for (y=0;y<NUMBER;y++){
		sprintf(tempStr1,"%08x",y);
		printf("Writing %s to uselesstempfiletwo\n",tempStr1);
		fgets(tempChar,2,stdin);
		fprintf(fp2,"0x%08x",y);
		printf("Done Writing %s to uselesstempfiletwo\n",tempStr1);
		fflush(fp2);
	}
	fclose(fp2);
	fp1 = fopen("uselesstempfileone", "r");
	fp2 = fopen("uselesstempfiletwo", "r");
	if (fp1){
		fseek(fp1,0,SEEK_END);
		strSize1 = ftell(fp1);
		rewind(fp1);
		tempStr5 = (char*) malloc(sizeof(char) *(strSize1+10));
		if (tempStr5==NULL){exit(2);}
		memset(tempStr5,0,strSize1+10);
		fread(tempStr5, sizeof(char),strSize1,fp1);
	}
	else{exit(1);}

	if (fp2){
		fseek(fp2,0,SEEK_END);
		strSize2 = ftell(fp2);
		rewind(fp2);
		tempStr2 = (char*) malloc(sizeof(char) *(strSize2+10));
		if (tempStr2==NULL){exit(3);}
		memset(tempStr2,0,strSize2+10);
		fread(tempStr2, sizeof(char),strSize2,fp2);
	}
	else{exit(1);}

	tempStr3 = new unsigned char[101];
	shaStr = new unsigned char[20];	
	for (x=0;(x<strSize1-50)||(x<strSize2-50);x++) {
		if ((x%MODNUMBER)==0){
			memset(tempStr3, 0, 101);
			fseek(fp1,x,SEEK_SET);
			fread(tempStr3,sizeof(char),50,fp1);
			fseek(fp2,((strSize2-50)-x),SEEK_SET);
			fread(tempStr3+50,sizeof(char),50,fp2);
			SHA1(tempStr3, 100, shaStr);
			printf("x: %i tempStr3: %s shaStr: %s\n",x,tempStr3,shaStr);
			fprintf(fp3,"%s",shaStr);
			fflush(fp3);
		}
	}
	fflush(stdout);
	fclose(fp3);
	fp3 = fopen("uselesstempfilethree","r");
	printf("The flag is: ");
	tempStr4 = new char[2];
	for (x=0;x<27;x++) {
		fseek(fp3,flagPositions[x]-1,SEEK_SET);
		fread(tempStr4,sizeof(char),1,fp3);	
		printf("%c",tempStr4[0]);
		fflush(stdout);
	}
	printf("\n");
	delete [] tempStr4;
	delete [] tempStr1;
	delete [] tempStr2;
	delete [] tempStr3;
	delete [] shaStr;
	delete [] tempChar;
	exit(0);
}
