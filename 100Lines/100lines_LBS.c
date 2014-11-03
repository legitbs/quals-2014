#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

unsigned char __randpad[] = {
  0xfc, 0x8a, 0x45, 0x51, 0x67, 0x8c, 0xa9, 0xc0, 0xb0, 0xfd, 0xf7, 0x6f,
  0xb8, 0x50, 0xf1, 0x2f, 0x7a, 0x62, 0x66, 0xe3, 0xd3, 0xc3, 0x6e, 0xbe,
  0x37, 0x39, 0x33, 0x68, 0x3b, 0xc6, 0x76, 0x1e, 0xae, 0xaa, 0x83, 0xed,
  0x57, 0x1a, 0xf1, 0x29, 0xe6, 0xc1, 0xb9, 0x9e, 0xdd, 0xa2, 0x86, 0x2c,
  0x1a, 0xdc, 0x49, 0x9d, 0x82, 0x01, 0xd5, 0x3a, 0xb5, 0xd3, 0x33, 0x12,
  0x1c, 0xce, 0x94, 0x2b, 0xc3, 0xb0, 0x6c, 0xbc, 0x46, 0x73, 0x39, 0x5e,
  0x7b, 0xc7, 0xb4, 0x9e, 0x56, 0xf0, 0xad, 0x72, 0x5e, 0x83, 0xc7, 0x05,
  0xc5, 0xe9, 0x2e, 0x85, 0x88, 0x79, 0x94, 0xf7, 0xe7, 0xac, 0x34, 0xfe,
  0x5c, 0xce, 0x2e, 0x13, 0xf1, 0xcc, 0x8e, 0xea, 0x60, 0x83, 0xbe, 0xdc,
  0x4a, 0xbb, 0xe8, 0xdf, 0x65, 0x20, 0xef, 0x44, 0xad, 0xfa, 0xd6, 0x12,
  0x83, 0xd5, 0xdc, 0x94, 0xad, 0x1f, 0xe1, 0x5f, 0xe8, 0xfa, 0x7e, 0x3f,
  0xda, 0x61, 0xe3, 0xdf, 0xab, 0x5b, 0x4f, 0x2a, 0x6c, 0x24, 0x82, 0xad,
  0x17, 0x89, 0xba, 0x29, 0xb9, 0x46, 0x34, 0x74, 0x64, 0xf7, 0x45, 0x22,
  0x8d, 0xaf, 0x33, 0xd6, 0x52, 0xb5, 0xde, 0x10, 0xe4, 0x53, 0x5d, 0x96,
  0xb7, 0xe2, 0x2e, 0xcb, 0xb1, 0x75, 0xbc, 0x74, 0x5a, 0x21, 0x29, 0x8c,
  0x57, 0xb3, 0x16, 0x5e, 0xc7, 0xc8, 0xc2, 0x26, 0x35, 0x48, 0x2d, 0x3c,
  0x60, 0x7b, 0x5d, 0xdd, 0xa8, 0x29, 0x61, 0x19, 0xd0, 0xef, 0xee, 0x6d,
  0x04, 0xdd, 0x20, 0x51, 0x95, 0x1d, 0x01, 0xe1, 0xda, 0xda, 0xb4, 0xa5,
  0x46, 0xd9, 0xcb, 0xaf, 0x56, 0xb5, 0x20, 0x05, 0xd0, 0x6b, 0xd2, 0x22,
  0x21, 0x2f, 0x2d, 0xd3, 0x73, 0x97, 0x56, 0x89, 0xae, 0xac, 0x02, 0xb6,
  0x35, 0xd2, 0x14, 0x87, 0xc6, 0x49, 0xdf, 0x0e, 0x17, 0x85, 0x64, 0xe5,
  0xaf, 0x6e, 0x93, 0x61
};
unsigned int __randpad_len = 256;

uint32_t calc(uint32_t tempVal,unsigned char *buf, unsigned long long int i, unsigned long long int l){
	tempVal = tempVal | ((((buf[((i/8)+l)]&0xff)<<(i%8))& 0xff)|(((buf[((i/8)+(l+1))]&0xff)>>(8-(i%8)))& 0xff))<<(24-(l*8));
	return tempVal;
}

void loop(unsigned long long int size, unsigned char *pad, unsigned char *buf){
	uint32_t tempValInner,tempValOuter;
	tempValOuter =0;
	tempValInner =0;
	unsigned long long lastBit = size-32;
	unsigned long long int i,j,l,m,n;
	for (i=0;i<lastBit;i++){
		tempValOuter = 0;
		for (l=0;l<4;l++) {
			tempValOuter = calc(tempValOuter, pad, i, l);
			}
		for (j=0;j<lastBit;j++) {
			tempValInner = 0;
			for (m=0;m<4;m++) {
			tempValInner = calc(tempValInner, pad, j, m);
			}
			tempValInner = tempValInner ^ tempValOuter;
			for (n=0;n<4;n++){
				buf[(((lastBit*i+j)*4)+n)] = tempValInner>>(24-(n*8));
			}
		}
	}
	return;
}
/*
unsigned char getByte(unsigned long long int byteToGet,unsigned long long int bufsize,unsigned char *buf) {
	uint64_t padBitLen, padBitLen2, bufSize2;
	unsigned char retval;
	unsigned char *temparray;
	padBitLen = (bufsize-32)*(bufsize-32)*4;
	bufSize2 = padBitLen*sizeof(unsigned char);
	temparray = (unsigned char*)malloc(bufSize2);
	if (temparray== NULL){exit(0);}
	loop(bufsize, buf, temparray);
	retval = temparray[byteToGet];
	delete [] temparray;
	return retval;
}
/**/

unsigned char getByte(unsigned long long int byteToGet,unsigned long long int bufsize,unsigned char *buf) {
	unsigned long long int byteNum,firstIndex,secondIndex,wordNum;
	uint32_t inner, outer;
	unsigned char retByte;
	int l,t;
	wordNum = byteToGet/4;
	byteNum = byteToGet%4;
	firstIndex = wordNum/(bufsize-32);
	secondIndex = wordNum%(bufsize-32);
	inner=0;
	outer=0;
	for (l=0;l<4;l++) {
		outer = calc(outer, buf, firstIndex, l);
		inner = calc(inner, buf, secondIndex, l);
	}
	outer=inner^outer;
	retByte = outer>>(24-(byteNum*8));
	return retByte;
}

int main(void) {
	FILE *fp = NULL;
	int x,y,randFP,strSize,passed;
	unsigned char blah;
	unsigned char *keyBuf = NULL;
	unsigned char *temparray = NULL;
	unsigned char *xorArray = NULL;
	unsigned long long padBitLen, padBitLen2, temparraySize, randData, finalArraySize;
	unsigned long long *OTPLocations;
	padBitLen =  (__randpad_len*8)-32;
	padBitLen2 = (padBitLen*padBitLen*4)-32;
	temparraySize = padBitLen*padBitLen*4;
	temparray = new unsigned char[padBitLen*padBitLen*4];
	xorArray = new unsigned char[38];
	finalArraySize = padBitLen2*padBitLen2*4; 
	loop(padBitLen, __randpad, temparray);
	OTPLocations = new unsigned long long [38];
	randFP = open ("/dev/urandom", O_RDONLY);
	puts("OTP locations:");
	for (x=0;x<38;x++) {
		read (randFP, &randData, sizeof(randData));
		randData = randData%(finalArraySize);
		printf("0x%016llx ",randData);
		OTPLocations[x] = randData;
		fflush(stdout);
	}
	printf("\n");
	passed = 0;
	fflush(stdout);
	for (y=0;y<8;y++){
		blah = fgetc(stdin);
		//if (blah = EOF){y=8;passed=0;}
		if (blah == (getByte(OTPLocations[y], temparraySize, temparray)%93)+32){passed=passed+1;}
		else{
			passed=0;
			//printf("else |%c|",(getByte(OTPLocations[y], temparraySize, temparray)%93)+32);
		}
	}
	printf("\n");
	if(passed>6){
		fp = fopen("/home/100lines/flag", "r");
		if (fp){
			fseek(fp,0,SEEK_END);
			strSize = ftell(fp);
			rewind(fp);
			keyBuf = (unsigned char*) malloc(sizeof(char) *(strSize+1));
			memset(keyBuf,0,strSize+1);
			fread(keyBuf, sizeof(char),strSize,fp);
			for (x=0;x<38;x++) {
				xorArray[x] = keyBuf[x] ^ getByte(OTPLocations[x], temparraySize, temparray);
				printf("0x%02x",xorArray[x]);
				if (x<37){printf(",");}
				fflush(stdout);
			}
			puts("\n");
		}
		else {puts("O-shit! Flag file is missing\n Please contact technical support.\n");}
	}
	delete [] OTPLocations;
	delete [] temparray;
	delete [] xorArray;
	return 0;
}