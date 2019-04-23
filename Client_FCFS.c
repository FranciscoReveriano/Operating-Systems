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
	int MemoryUnits;					//used for number of memory units the Process requires
} Process;								/*Datatype of the elements in the queue*/

typedef struct result {
	int WaitingInReady;
	int completionTime;
	int turnAroundTime;
	int frameAllocated;					// Frames allocated to the process
	int fragmentation;					// Fragmentation 
	int error;
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
	// Prompt for Memory Request
	printf("Enter Memory Request(Number of Units): ");
	scanf("%d", &process.MemoryUnits);

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

	if (result.error == 1) {
		printf("\n------------ Results from Server ------------------");
		printf("\nServer did not have the necessary memory");
	}
	else {
		// Results Read Back From Server
		printf("\n------------ Results from Server ------------------");
		printf("\nCompletition Time: %d", result.completionTime);
		printf("\nTurnAround Time: %d", result.turnAroundTime);
		printf("\nWaiting in Ready Time: %d", result.WaitingInReady);
		printf("\nFragmentation : %d", result.fragmentation);
	}
	printf("\n");
	unlink("common_FIFO");
	unlink(process.str);

	close(fdIN);
	close(fdOUT);

}