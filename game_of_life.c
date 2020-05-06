#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <mpi.h>

/*
Name: James Denney
BlazerId: jdenney2
Course Section: CS 432
Homework #: 4
*/

/*
			IMPORTANT INTSRUCTIONS:

To Compile: Ensure that you have game_of_life and makefile in the same folder.
	Then type make into the terminal.

To Run: Type ./<program-name> <X-Dimension> <Y-Dimension> <Number-of-Generations>
*/

//Used to keep track of whether or not an iteration of the board has changed.

double gettime(void) {
	struct timeval tval;

	gettimeofday(&tval, NULL);

	return ((double)tval.tv_sec + (double)tval.tv_usec/1000000.0);
}

//Allocates the memory for a 2d array
bool **allocarray(int Q, int P) {
	int i;
	bool *p, **a;

	p = (bool *)malloc(P*Q*sizeof(bool));
	a = (bool **)malloc(P*sizeof(bool*));

	if (p == NULL || a == NULL)
		printf("Error allocating memory\n");

	/* For row major storage */
	for (i = 0; i < P; i++) {
		a[i] = &p[i*Q];
	}

	return a;
}

//initializes the main array
bool **initarray(bool **a, int x, int y) {
	int i,j;

	//set up the "real" cells
	for (i = 1; i < y - 1; i++) {
		for (j = 1; j < x - 1; j++) {
			if (drand48() > 0.5) {
				a[i][j] = true;
			}
			else {
				a[i][j] = false;
			}
		}
	}

	return a;
}

//Sets up the "ghost" cells
bool **copyGhostCells(bool **a, int x, int y) {
	int i;

        for(i = 1; i < x - 1; i++) {
                a[0][i] = a[y - 2][i];
                a[y - 1][i] = a[1][i];
        }
        for(i = 0; i < y; i++) {
                a[i][0] = a[i][x - 2];
                a[i][x - 1] = a[i][1];
	}
	return a;
}

//Prints a 2d array
void printarray(bool **a, int x, int y) {
	int i, j;
		
	for (i = 0; i < y; i++) {
		for (j = 0; j < x; j++) {
			printf("%d ", a[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

//Check all of the "real" cell's neighbors, and stores whether or not they should live in a temp array
bool **checkNeighbors(bool **arr,bool **tempArr, int size, int start, int end) {
	int i, j, count;

	for(i = start; i < end; i++) {
		for(j = 1; j < size - 1; j++) {
			count = 0;
			//printf("Checking (%d,%d):\n", j, i);
	
			count += arr[i-1][j-1];
			count += arr[i][j-1];
			count += arr[i+1][j-1];
			count += arr[i-1][j];
			count += arr[i+1][j];
			count += arr[i-1][j+1];
			count += arr[i][j+1];
			count += arr[i+1][j+1];			
   
			//Die if lonely
			if(count <= 1 && arr[i][j]) {
				tempArr[i][j] = false;
			}

			//Die if overcrowded
			if(count >= 4 && arr[i][j]) {
				tempArr[i][j] = false;
			}	

			//Return to life
			if(count == 3 && !arr[i][j]) {
				tempArr[i][j] = true;
			}
		}
	}
	
	return tempArr;
}

bool **consolodateArrs(bool **a, bool **b, int size, int start, int end) {
	int i, j;
	for (i = start; i < end; i++) { 
		for (j = 1; j < size - 1; j++) {
			a[i][j] = b[i][j];
		}
	}
	return a;	
}

//Queues up a set of tests, defined by user input.
int main (int argc, char **argv) {
	int size, ngen, i, j;
	double starttime, endtime;

	srand48(time(0));

	if (argc != 3) {
		printf("Usage: <%s> <Size> <N-Generations>\n", argv[0]);
		exit(-1);
	}

	size = atoi(argv[1]) + 2;
	ngen = atoi(argv[2]);	 	

	bool **a = NULL, **b = NULL;		

	a = allocarray(size, size);
	b = allocarray(size, size);

	a = initarray(a, size, size);
	a = copyGhostCells(a, size, size);

	starttime = gettime();	
	//printarray(a, size, size);		

	MPI_Init(NULL, NULL);

	int worldRank, worldSize;	
	MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

	int start = (size/worldSize*worldRank);
	if (start == 0) {
		start = 1;
	}

	int end = (size/worldSize*(worldRank+1));
	if (end >= size - 1) {
		end = size - 1;
	}

	for(j = 0; j < ngen; j++) {
		b = checkNeighbors(a, b, size, start, end);
		if (worldRank != 0) {
			MPI_Send(&(b[0][0]), size, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD);
			MPI_Recv(&(a[0][0]), size, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
		}
		else {
			a = consolodateArrs(a, b, size, start, end);
			for (i = 1; i < worldSize; i++) {
				MPI_Recv(&(b[0][0]), size, MPI_C_BOOL, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				
				int iStart = (size/worldSize*i);
			        if (iStart == 0) {
                			iStart = 1;
        			}
				int iEnd = (size/worldSize*(i+1));
				if (iEnd > size - 1) {
					iEnd = size - 1;
				}
 				
				a = consolodateArrs(a, b, size, iStart, iEnd);
			}
			
			a = copyGhostCells(a, size, size);
			//printarray(a, size, size);			

			for (i = 1; i < worldSize; i++) {
				MPI_Send(a, size, MPI_C_BOOL, i, 0, MPI_COMM_WORLD);
			}
		}		
	}
	if (worldRank == 0) { 
		endtime = gettime();
		printf("Time taken for test = %lf seconds\n", endtime-starttime);
	}
	
	MPI_Finalize();
}
