#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

typedef struct values {
	char str[30];
	int arrivalTime;
	int burst;							//used for both the initial size of the process and to send the completion time
} Process;								/*Datatype of the elements in the queue*/

typedef struct result {
	int WaitingInReady;
	int completionTime;
	int turnAroundTime;
} Results;

int main(int argc, char *argv[]) {

	Process process;
	Results result;

	int fdIN;																// to write to character server
	int fdOUT;																// to read from character server
	int clientID;															// Get Client ID

	// Structure Information
		// Get Client ID
	clientID = getpid();													// GET ID
	sprintf(process.str, "FIFO_%d", clientID);
	printf("\nFIFO name is %s ", process.str);								// Print Process Name
	// Prompt for Arrival time
	printf("\nEnter Arrival Time: ");
	scanf("%d", &process.arrivalTime);
	// Prompt for Burst Time
	printf("Enter burst: ");
	scanf("%d", &process.burst);

	// Make Private FIFO
	if ((mkfifo(process.str, 0666) < 0 && errno != EEXIST))
	{
		perror("Can't create private FIFO\n");
		exit(-1);

	}

	// Open Common_FIFO
	if ((fdIN = open("common_FIFO", O_WRONLY)) < 0)
	{
		printf("cant open fifo to write");
	}
	// Write to common_FIFO
	write(fdIN, &process, sizeof(process));

	// Read from Private_FIFO
	if ((fdOUT = open(process.str, O_RDONLY)) < 0) {
		printf("cant open fifo to read");
	}
	// Read from Private_FIFO
	read(fdOUT, &result, sizeof(result));

	// Results Read Back From Server
	printf("\n------------ Results from Server ------------------");
	printf("\nCompletition Time: %d", result.completionTime);
	printf("\nTurnAround Time: %d", result.turnAroundTime);
	printf("\nWaiting in Ready Time: %d", result.WaitingInReady);

	printf("\n");
	unlink("common_FIFO");
	unlink(process.str);

	close(fdIN);
	close(fdOUT);

}