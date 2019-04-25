#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

//Define Global Variables
int globalClock;
float throughput;
float averageTurnAround;
float averageWaitTime;
float cpuUtilization;
int frames = 0;							// Variable for Number of Frames
int frameSize = 0;						// Variable for Size of Frames
int Memory = 0;							// Variable for available memory
int toggle = 0;							// Toggle For Memory check
int total = 0;							// Calculate Total Burst
int processCompleted = 0;				// Process Completed Counter

typedef struct pcb {  /*structure for PCBs */
	char str[30];
	int arrivalTime;
	int burst;
	int MemoryUnits;
} PCB;

typedef struct result {
	int WaitingInReady;
	int completionTime;
	int turnAroundTime;
	int frameAllocated;
	int fragmentation;
	int error;
	int MemoryBlocksUsed;
	int MemoryBlocksRemaining;
	int FinalBlockAddress;
} Results;

typedef struct node {  /*Nodes stored in the linked list*/

	struct pcb elements;

	struct node *next;

} Node;

typedef struct queue { /*A struct facilitates passing a queue as an argument*/

	Node *head;       /*Pointer to the first node holding the queue's data*/

	Node *tail;       /*Pointer to the last node holding the queue's data*/

	int sz;           /*Number of nodes in the queue*/

} Queue;

int size(Queue *Q) {

	return Q->sz;

}

int isEmpty(Queue *Q) {

	if (Q->sz == 0) return 1;

	return 0;

}

void enqueue(Queue *Q, struct pcb elements) {

	Node *v = (Node*)malloc(sizeof(Node));/*Allocate memory for the Node*/

	if (!v) {

		printf("ERROR: Insufficient memory\n");

		return;

	}

	v->elements = elements;

	v->next = NULL;

	if (isEmpty(Q)) Q->head = v;

	else Q->tail->next = v;

	Q->tail = v;

	Q->sz++;

}

PCB dequeue(Queue *Q) {

	PCB temp;
	Node *oldHead;



	if (isEmpty(Q)) {

		printf("ERROR: Queue is empty\n");

		return temp;

	}

	oldHead = Q->head;
	temp = Q->head->elements;
	Q->head = Q->head->next;
	free(oldHead);
	Q->sz--;
	return temp;
}

PCB first(Queue *Q) {
	PCB temp;
	if (isEmpty(Q)) {
		printf("ERROR: Queue is empty\n");
		return temp;
	}
	printf("Head of the list is:%s\n", Q->head->elements.str);
}

PCB firstMemory(Queue *Q) {
	PCB temp;
	int tempMemory;
	if (isEmpty(Q)) {
		printf("ERROR: Queue is empty\n");
		return temp;
	}
	printf("Head of the list is: %s\n", Q->head->elements.str);
	tempMemory = Q->head->elements.MemoryUnits;					// Make Variable hold the memory units required in head
	if (tempMemory > Memory) {
		toggle = 1;
	}
	else {
		toggle = 0;
	}

}

void destroyQueue(Queue *Q) {

	while (!isEmpty(Q)) dequeue(Q);

}

/*A different visit function must be used for each different datatype.*/
/*The name of the appropriate visit function is passed as an argument */
/*to traverseQueue.                                                   */
void visitNode(PCB elements) {

	printf("\nPCB name: %s", elements.str);
	printf("\nPCB Arrival Time: %d", elements.arrivalTime);
	printf("\nBurst size: %d", elements.burst);
	printf("\n Memory Unit size: %d", elements.MemoryUnits);
	printf("\n");
}

int visitNodeMemory(PCB elements) {
	// Declare Structure
	Results result;
	
	// Variable Declarations
	int fdOUT;
	result.error = 1;

	// Diminish Time
	total = total - elements.burst;
	// Send through private FIFO right here
	fdOUT = open(elements.str, O_WRONLY);             //Open privFIFO
	write(fdOUT, &result, sizeof(result));          //Write to privFIFO
}

///  This function calculates results and sends them
void visitResultNode(PCB elements) {
	// Variable Declarations
	int fdOUT;                                                                                      // Variable to open FIFO
	int completitionTime;																			// Completition Time
	int turnAroundTime;																				// TurnAround Time
	int waitingTime;																				// Waiting Time
	int burst = elements.burst;																		// Burst Time 
	int arrivalTime = elements.arrivalTime;															// Arival Time

	// Increment Program Counter
	processCompleted++;																				// Increment Process Counter

	// Structure declaration
	Results result;

	// Send information back to the client
		// Add time to the clock
	globalClock = globalClock + burst;
	//New Clock time is the Completition time
		/* Completition Time = Time at which the process its execution*/
	completitionTime = globalClock;
	result.completionTime = completitionTime;
	// Turn Around Time
		/* Time Difference between completition time and arrival time*/
	turnAroundTime = completitionTime - arrivalTime;
	averageTurnAround += turnAroundTime;
	result.turnAroundTime = turnAroundTime;
	// Waiting Time
		/* Time difference between turn around time and burst time*/
	waitingTime = turnAroundTime - burst;
	averageWaitTime += waitingTime;
	result.WaitingInReady = waitingTime;

	// Fragmentation
	int MemoryBlocksUsed;
	int MemoryBlocksRemaining;
	int FinalBlockAddress;
	MemoryBlocksUsed = elements.MemoryUnits / frameSize;							// Calculate how many frames i am using
	result.MemoryBlocksUsed = MemoryBlocksUsed;
	result.MemoryBlocksRemaining = Memory-elements.MemoryUnits;						//Total Memory Blocks Remaining  
	result.FinalBlockAddress = elements.MemoryUnits - (MemoryBlocksUsed*frames);
	

	// Send through private FIFO right here
	fdOUT = open(elements.str, O_WRONLY);             //Open privFIFO
	write(fdOUT, &result, sizeof(result));          //Write to privFIFO
}

/*The following function isn't part of the Queue ADT, however*/
/*it can be useful for debugging.                            */
void traverseQueue(Queue *Q) {

	Node *current = Q->head;

	while (current) {

		visitNode(current->elements);

		current = current->next;

	}

	printf("\n");

}

/*Sample code that demonstrates the queue*/
int main(int argc, char *argv[]) {

	/*Declare a queue and initialize its fields*/
	Queue Q;
	Q.head = NULL;
	Q.tail = NULL;
	Q.sz = 0;

	// Declare Variables
	int fdIN;			// Common FIFO
	int fdOUT;			//private FIFOs' file desriptors
	int count = 0;		//Total number of clients, set by user
	int i = 0;			//Iterative counter used for receiving section
	int j = 0;			// Iterative Counter for Sending Section
	int b = 0;			// Variable to input the burst

	// Declare Process
	PCB process;
	Results result;

	// Create common_FIFO
	if ((mkfifo("common_FIFO", 0666) < 0 && errno != EEXIST))         /*Creating common_FIFO*/
	{
		perror("Can't create Common FIFO\n");
		exit(-1);
	}


	// All variables defined and commFIFO open for clients UI & Enqueueing //
	//User Promts
	// Prompt for Number of Clients
	printf("How many clients do you have?: ");
	scanf("%d", &count);
	// Prompt for Initial Number of Frames
	printf("What is the initial number of frames: ");
	scanf("%d", &frames);
	//Prompt for Size of Frames
	printf("What is the size of each frame: ");
	scanf("%d", &frameSize);
	//Display total number of frames available
	Memory = frames * frameSize;
	printf("Available Memory: %d", Memory);
	// Open common_FIFO to read
	if ((fdIN = open("common_FIFO", O_RDONLY)) < 0)
		printf("Can't open common FIFO to read\n");

	//Read from clients
	while (i < count)                                                                                                        // For each process
	{
		printf("\nWaiting for client request...\n\n");

		read(fdIN, &process, sizeof(process));
		printf("The Private FIFO name is %s.\n", process.str);
		printf("The Prcess burst is: %d\n", process.burst);
		printf("The Arrival Time is: %d\n", process.arrivalTime);
		printf("The Memory Units requested is: %d\n", process.MemoryUnits);

		//calculate total burst time
		b = process.burst;
		total = total + b;                                                                                              // Added for reporting

		//Enque Burst Time
		enqueue(&Q, process);
		i++;                                                                                                                    // Increase Counter
	}
	printf("\nTotal Bursts: %d\n", total);

	//////////////////////////////////////////// All user requests queued ///////////////////////////////////
	while (j < count) {
		printf("\n------- Queue Properties -------\n");
		printf("Size = %d\n", size(&Q));
		printf("isEmpty = %d\n", isEmpty(&Q));

		/*When we traverse the queue we see all the elements and their properties of the queue*/
		printf("\n------- Traversing the queue --------- ");
		traverseQueue(&Q);

		// Perform Memory Check
		printf("\n--------- Checking Memory -----------\n");
		firstMemory(&Q);

		// Dequeue PCB
		if (toggle == 1) {
			printf("Error Memory Could not be allocated to Process");
			visitNodeMemory(dequeue(&Q));
			j++;
			continue;
		}
		printf("\n--------- Dequeuing PCB ------------");
		visitResultNode(dequeue(&Q));
		toggle == 0;									// Toggle Back to Allow arithmetics
		j++;
	}// close return while loop

	// Server Print Statements
	// Throughput
	if (toggle == 0) {
		throughput = total / count;					// # of processes per time unit
		printf("\nThe server completes %d processes in %d ms", processCompleted, total);
		printf("\nThe Throughput: %0.2f", throughput);
		// CPU Utilization Time
		cpuUtilization = total / globalClock;
		printf("\nThe CPU Utilization Time is: %0.2f", cpuUtilization);
		// Average Turn Around TIme
		averageTurnAround = averageTurnAround / 3;
		printf("\nThe Average Turn Around Time is: %0.2f", averageTurnAround);
		//Average Wait Time
		averageWaitTime = averageWaitTime / 3;
		printf("\nThe Average Waiting Time is: %0.2f", averageWaitTime);
	}
	//Close common_FIFO
	printf("\n--------------- Done --------------------\n");
	close(fdIN);
	unlink("common_FIFO");
	printf("common_FIFO closed and unlinked.\n\n");
}