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

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyAMA0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;
volatile int READ_STOP = FALSE;

 int main() {

	int fd,c, res;
	struct termios oldtio,newtio;
	char buf[255];
	fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );
	if (fd <0) {perror(MODEMDEVICE); exit(1); }

	tcgetattr(fd,&oldtio); /* save current port settings */
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non−canonical, no echo,...) */
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 0; /* inter−character timer unused */
	newtio.c_cc[VMIN] = 5; /* blocking read until 5 chars received */
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);
	char packageOn[50] = {0x7E, 0x00, 0x10, 0x17, 0x01, 0x00, 0x13, 0xA2, 0x00, 0x41, 0x6B, 0x89, 0x49, 0xFF, 0xFE, 0x02, 0x44, 0x38, 0x05, 0x34 };
	char packageOff[50] = { 0x7E, 0x00, 0x10, 0x17, 0x01, 0x00, 0x13, 0xA2, 0x00, 0x41, 0x6B, 0x89, 0x49, 0xFF, 0xFE, 0x02, 0x44, 0x38, 0x04, 0x35};
	
	char packageRead[50] = {};
	 char readTemp[1] = { };
	 



	while (STOP==FALSE) { /* loop for input */
		int w = write(fd, packageOn, 20);
		printf("%d\n", w);
		usleep(1000);
		STOP=TRUE;
	}

	 while (READ_STOP == FALSE) {
		 int r = read(fd, readTemp, 1);
		 packageRead[0] = readTemp[0];
		 if (packageRead[0] == 0x7e) {
			 READ_STOP = TRUE;

			 r = read(fd, readTemp, 1);
			 packageRead[1] = readTemp[0];
			 r = read(fd, readTemp, 1);
			 packageRead[2] = readTemp[0];
			 
			 int length = (packageRead[1] << 8) + packageRead[2];

			 for (int i = 0; i < length+1; i++) {
				 r = read(fd, readTemp, 1);
				 packageRead[i+3] = readTemp[0];
				 printf("0x%x ", packageRead[i+3]); 
				 }

		 }		
	 }
	 int tester = 123;
	tcsetattr(fd,TCSANOW,&oldtio);
}