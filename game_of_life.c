#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <omp.h>

/*
Name: James Denney
BlazerId: jdenney2
Course Section: CS 432
Homework #: 3
*/

/*
			IMPORTANT INTSRUCTIONS:

To Compile: Ensure that you have game_of_life and makefile in the same folder.
	Then type make into the terminal.

To Run: Type ./<program-name> <X-Dimension> <Y-Dimension> <Number-of-Generations>
*/

//Used to keep track of whether or not an iteration of the board has changed.
bool changed = false;

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

//initializes the temporary array, which stores whether a value has changed or not
bool **initTempArray(bool **a, int x, int y) {
	int i, j;

	for(i = 1; i < y - 1; i++) {
		for(j = 1; j < x - 1; j++) {
			a[i][j] = false;
		}
	}
	return a;
}

//Compares the main array with the temp array and makes the appropriate changes.
//Important note: The temp array does NOT store real values. Its purpose is to store whether or not a value should change.
bool **compareArrays(bool **realArr, bool **compArr, int x, int y) {
	int i, j;

	for(i = 0; i < y; i++) {
		for(j = 0; j < x; j++) {
			if (compArr[i][j]) {
				changed = true;
				if(realArr[i][j]) {
					realArr[i][j] = false;
				}
				else {
					realArr[i][j] = true;
				}
				compArr[i][j] = false;
			}
		}
	}
	return realArr;
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
bool **checkNeighbors(bool **arr,bool **tempArr, int x, int y, int numThreads) {
	int i, j, count;
	changed = false;

	#pragma omp parallel for num_threads(numThreads) private(i, j, count)

	for(i = 1; i < y - 1; i++) {
		for(j = 1; j < x - 1; j++) {
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
				tempArr[i][j] = true;
				changed = true;
			}

			//Die if overcrowded
			if(count >= 4 && arr[i][j]) {
				tempArr[i][j] = true;
				changed = true;
			}	

			//Return to life
			if(count == 3 && !arr[i][j]) {
				tempArr[i][j] = true;
				changed = true;
			}
		}
	}
	
	return copyGhostCells(tempArr, x, y);

}

//Queues up a set of tests, defined by user input.
int main (int argc, char **argv) {
	int x, y, ngen, i, j;
	double starttime, endtime;

	srand48(time(0));

	if (argc != 5) {
		printf("Usage: <%s> <X-Dimension> <Y-Dimension> <N-Generations> <N-Threads>\n", argv[0]);
		exit(-1);
	}

	x = atoi(argv[1]) + 2;
	y = atoi(argv[2]) + 2;
	ngen = atoi(argv[3]);

	bool **a = NULL, **b = NULL;		

	a = allocarray(x, y);
	b = allocarray(x, y);

	a = initarray(a, x, y);
	b = initTempArray(b, x, y);
	a = copyGhostCells(a, x, y);
	b = copyGhostCells(b, x, y);

	starttime = gettime();	
	//printarray(a, x, y);		
	
	for(j = 0; j < ngen; j++) {
		b = checkNeighbors(a, b, x, y, atoi(argv[4]));
		if(!changed) {
			break;
		}
		a = compareArrays(a, b, x, y);
		//printarray(a, x, y);
	}

	endtime = gettime();
	printf("Time taken for test = %lf seconds\n", endtime-starttime);

	return 0;
}
