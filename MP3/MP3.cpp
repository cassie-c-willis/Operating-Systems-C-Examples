/* Cassie Willis
 *
 * CS 4414 - Operating Systems
 * Spring 2018
 * 3/26/2018
 *
 * MP3 - Barrier and Synchronization Problem
 *
 * The purpose of this project is to become familiar with threads, barriers, 
 * and synchronization. This project recieves integer input from STDIN and 
 * finds the max value from the inputs, returning that value to STDOUT.
 * Refer to the writeup for complete details.
 *
 * Compile with MAKE
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <semaphore.h>
using namespace std;

int numberOfThreads = 0; //Number of threads needed for the given round
int numberOfNumbers = 0; //How many integers are given in STDIN before an empty line
int inputArray[8000];
int howManyRunThreads = 0; //Number of active threads in a given round
int newNum;

//Function declarations
int max_threading(int currThread, int currRound);
void* iterate(void* param);
void get_input(void);
void print_output(int max);
int find_max(int first, int second);


//This class uses three binary semaphores to create a barrier_wait function 
//so that the threads are synchronized based on the barrier
class CS {
	sem_t mutex;
	sem_t waitq;
	sem_t throttle;
	int bVal;
	int k;
	public:
		//Initialize the binary semaphore values and the barrier value to an existing barrier
		void inits(int s1, int s2, int s3, unsigned v1, unsigned v2, unsigned v3, int barrierVal) {
			sem_init(&mutex, s1, v1); //0,1
			sem_init(&waitq, s2, v2); //0,0
			sem_init(&throttle, s3, v3); //0,0
			bVal = barrierVal;
			k = barrierVal;
		}	

		//Initialize a barrier
 		CS(int val) {bVal = val;}

 		//Barrier wait function that synchronizes the threads
		void wait(void) {
			sem_wait(&mutex); //make the wait() function atomic
			bVal --;

			if (bVal != 0) { //still waiting
				sem_post(&mutex);
				sem_wait(&waitq);
				sem_post(&throttle);
			}

			else { //release
				int i;
				for(i = 0; i < k - 1; i++) {
					sem_post(&waitq);
					sem_wait(&throttle);
				}

				bVal = k;
				sem_post(&mutex);	
			}
		}
};

CS myBarrier(numberOfThreads); //Initialize a barrier

//Get the input from STDIN and add values to inputArray
void get_input() {
	char buf[8000];
	while(fgets(buf, sizeof(buf), stdin) != NULL) { //scan the input until the input given is not an integer, put input into a temporary array
		int x = sscanf(buf, "%d", &inputArray[numberOfNumbers]);
		if (x != 1) break;
		numberOfNumbers++;
	}
	numberOfThreads = numberOfNumbers / 2;
}

//Print the max number to STDOUT
void print_output(int max) {
	printf("%d\n", max);
}

//Compare two numbers and return the maximum of the two, if equal, return first
int find_max(int first, int second) {
	if (first >= second) return first;
	else return second;
}

//Handle finding the maximum numbers by sending the correct thread number and round number
//to the max_threading function, incrementing the rounds and activating the correct threads
void* iterate(void* param) {
	int threadNum = *(int*) param; //thread ID number
	int numRounds = 0;
	int number;
	int round = 0;
	int logOf = numberOfNumbers;
	howManyRunThreads = numberOfThreads; //how many threads are running actively in the given round

	while (logOf >>= 1) numRounds++; //determine the number of rounds to perform the comparisons using bit-wise version of logarithms
	while(numRounds != 0) {

		howManyRunThreads = (numberOfThreads / (1 << round));
		if (threadNum <= howManyRunThreads) {	//run the comparison algorithm only on the active threads
			number = max_threading(threadNum, (round+1));
			inputArray[number] = newNum;	
		}

		round++;
		numRounds--;
		myBarrier.wait();		
	}
	return NULL;
}

//Handle which two values from the input array are compared, based on the thread TID and the round number
int max_threading(int currThread, int currRound) {
	int threadNum = currThread; //tid number of the currently running thread (i.e. thread 0, thread 1, thread 2)
	int roundNum = currRound;
	int shift = 1 << roundNum; //2^(round number)
	int added = 1 << (roundNum-1); //2^(round number - 1)

	int arrCpy[numberOfNumbers-1];	//make a copy of the input array for safety purposes if mutilple threads in function at once for some reason
	int i;
	for(i = 0; i < numberOfNumbers; i++) {
		arrCpy[i] = inputArray[i];
	}

	int curr = (threadNum * shift) % numberOfNumbers;
	int next = ((threadNum * shift) + added) % numberOfNumbers;

	newNum = find_max(arrCpy[curr], arrCpy[next]);
	return curr;
}

int main() {
	get_input(); //get input from STDIN

	pthread_t tids[numberOfThreads]; //Initialize correct number of threads
	int t[numberOfThreads]; //Array of TIDs for each thread
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	myBarrier.inits(0, 0, 0, 1, 0, 0, numberOfThreads); //Initialize barrier

	int i;
	for (i = 0; i < numberOfThreads; i++) { //create threads running iterate()
		t[i] = i;
		pthread_create(&tids[i], &attr, iterate, &t[i]);
	}

	for (i = 0; i < numberOfThreads; i++) { //Remove threads
		pthread_join(tids[i], NULL);
	}

	inputArray[0] = newNum; //Set final max value
	print_output(inputArray[0]); //Print max value

	return 0;
}