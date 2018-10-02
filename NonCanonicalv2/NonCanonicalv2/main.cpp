//Includes
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

//Defines
#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyAMA0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

//Global Variables
volatile int STOP = FALSE;
volatile int READ_STOP = FALSE;
volatile int READ_ERROR = FALSE;
volatile int fd, c, res;
volatile char buf[255];
struct termios oldtio, newtio;

//Functions
int receive();
int initUART();


int main()
{
	initUART();

	char packageOn[50] = { 0x7E, 0x00, 0x10, 0x17, 0x01, 0x00, 0x13, 0xA2, 0x00, 0x41, 0x6B, 0x89, 0x49, 0xFF, 0xFE, 0x02, 0x44, 0x38, 0x05, 0x34 };
	char packageOff[50] = { 0x7E, 0x00, 0x10, 0x17, 0x01, 0x00, 0x13, 0xA2, 0x00, 0x41, 0x6B, 0x89, 0x49, 0xFF, 0xFE, 0x02, 0x44, 0x38, 0x04, 0x35 };

	while (STOP == FALSE) { /* loop for input */
		int w = write(fd, packageOff, 20);
		printf("%d\n", w);
		usleep(1000);
		STOP = TRUE;
	}

	receive();

	tcsetattr(fd, TCSANOW, &oldtio);
}

//Init UART communication
int initUART()
{
	fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY);
	if (fd < 0) { perror(MODEMDEVICE); exit(1); }

	tcgetattr(fd, &oldtio); /* save current port settings */
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non?canonical, no echo,...) */
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 0; /* inter?character timer unused */
	newtio.c_cc[VMIN] = 5; /* blocking read until 5 chars received */
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);
}

//Recive return Package
int receive()
{
	printf("\n\nStart Reading... \n");
	
	char packageRead[50] = {};
	char readTemp[2] = {};
	int r = 0;
	
	while (READ_STOP == FALSE) {
		//Read 1. byte
		r = read(fd, readTemp, 1);
		if (r != 1) READ_ERROR = TRUE;
		packageRead[0] = readTemp[0];


		//If 1. byte = startbyte 0x7e
		if (packageRead[0] == 0x7e)
		{
			printf("Package found! \n");
			
			READ_STOP = TRUE;

			//Read length of package
			r = read(fd, readTemp, 1);
			if (r != 1) READ_ERROR = TRUE;
			packageRead[1] = readTemp[0];

			r = read(fd, readTemp, 1);
			if (r != 1) READ_ERROR = TRUE;
			packageRead[2] = readTemp[0];

			//Calculating length of package
			int length = (packageRead[1] << 8) + packageRead[2];
			printf("Package length: %d\n\n", length);
			//Reading rest of package
			for (int i = 0; i < (length + 1); i++) {
				r = read(fd, readTemp, 1);
				if (r != 1) READ_ERROR = TRUE;
				packageRead[i + 3] = readTemp[0];
			}
			
			//testing for READ ERROR
			if (READ_ERROR == TRUE) printf("READ ERROR OCCURED IN THE PROGRAM \n");

			//calculating checksum
			char ChkSum = 0xFF;
			for(int i = 0; i < length; i++) {
				ChkSum -= packageRead[i+3];
			}
			if (ChkSum != packageRead[(length + 3)]) printf("Checksum Error: Checksum do not match: \nCal: %x \nPac: %x\n\n", ChkSum, packageRead[length +3]);
			else printf("Checksum: OK!\n");

			//printing package
			printf("Package [Hexa]: \n");
			for(int i = 0; i < length + 4; i++)
			{
				printf("%x ", packageRead[i]);
				fflush(stdout);
			}
			printf("\n");

			//Finding status from package:
			switch (packageRead[length+2])
			{
			case 0: 
				printf("Status: OK \n");
				break;
			case 1:
				printf("Status: ERROR \n");
					break;
			case 2:
				printf("Status: INVALID COMMAND \n");
					break;
			case 3:
				printf("Status: INVALID PARAMETER \n");
					break;
			case 4:
				printf("Status: TX FALURE \n");
					break;
			default:
				printf("STATUS ERROR, No sutch option \n");
					break;
			}
		}
	}
}
