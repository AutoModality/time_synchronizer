/**
 * @file   testebbchar.c
 * @author Derek Molloy
 * @date   7 April 2015
 * @version 0.1
 * @brief  A Linux user space program that communicates with the ebbchar.c LKM. It passes a
 * string to the LKM and reads the response from the LKM. For this example to work the device
 * must be called /dev/ebbchar.
 * @see http://www.derekmolloy.ie/ for a full description and follow-up descriptions.
*/
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH] = "";     ///< The receive buffer from the LKM
static char last_receive[BUFFER_LENGTH] = "";

int main()
{
	int ret, fd;
	char stringToSend[BUFFER_LENGTH];
	char gpioPath[BUFFER_LENGTH];
	int gpio = 388;
	char c;

	sprintf(gpioPath, "/sys/ebb/gpio%d/lastTime", gpio);

	printf("Starting device test code example...\n");

	while(1)
	{		
		fd = open(gpioPath, O_RDONLY);             // Open the device with read only access
		if (fd < 0)
		{
			perror("Failed to open the device...");
			return errno;
		}
		
		c = getchar();
		if (c == ' ') break;
		
		printf("\nThe last received message is: %s\n", last_receive);

		printf("Reading from the device...\n");
		ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
		if (ret < 0)
		{
			perror("Failed to read the message from the device.");
			return errno;
		}
		printf("The received message is: %s\n", receive);

		//sleep(5);

		if(!strcmp(receive, last_receive) || !strcmp(last_receive, ""))
		{
			printf("No new interrupt.\n");
		}
		snprintf(last_receive, sizeof(last_receive), "%s", receive);

		close(fd);
	}

	printf("End of the program\n");
	return 0;
}
