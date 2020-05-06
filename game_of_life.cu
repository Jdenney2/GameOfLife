#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>

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
bool *copyGhostCells(bool *a, int x, int y) {
	int i;

        for(i = 1; i < x - 1; i++) {
                a[0 * x + i] = a[(y - 2) * x + i];
                a[(y - 1) * x + i] = a[1 * x + i];
        }
        for(i = 0; i < y; i++) {
                a[i * x + 0] = a[i * x + (x - 2)];
                a[i * x + (x - 1)] = a[i * x + 1];
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

__global__
void checkNeighbors(bool *arr,bool *tempArr, int x, int y) {
	int i = blockIdx.y, j = blockIdx.x, count;

		count = 0;
		//printf("Checking (%d,%d):\n", j, i);
	
		count += arr[(i-1) * x + (j-1)];
		count += arr[(i) * x + (j-1)];
		count += arr[(i+1) * x + (j-1)];
		count += arr[(i-1) * x + (j)];
		count += arr[(i+1) * x + (j)];
		count += arr[(i-1) * x + (j+1)];
		count += arr[(i) * x + (j+1)];
		count += arr[(i+1) * x + (j+1)];			
   
		//Die if lonely
		if(count <= 1) {
			tempArr[i * x + j] = false;
		}

		//Die if overcrowded
		if(count >= 4) {
			tempArr[i * x + j] = false;
		}	

		//Return to life
		if(count == 3) {
			tempArr[i * x + j] = true;
		}
}

bool *squashify(bool **src, bool *dest, int x, int y) {
	int i, j;
	
	for (i = 0; i < x; i++) {
		for (j = 0; j < y; j++) {
			dest[i * x + j] = src[i][j];
		}
	}

	return dest;
}

bool **desquashify(bool *src, bool **dest, int x, int y) {
	int i, j;
	
	for (i = 0; i < x; i++) {
		for (j = 0; j < y; j++) {
			dest[i][j] = src[i * x + j];
		}
	}

	return dest;
}

//Queues up a set of tests, defined by user input.
int main (int argc, char **argv) {
	int x, y, ngen, j;
	double starttime, endtime;

	//srand48(time(0));

	if (argc != 4) {
		printf("Usage: <%s> <X-Dimension> <Y-Dimension> <N-Generations>\n", argv[0]);
		exit(-1);
	}

	x = atoi(argv[1]) + 2;
	y = atoi(argv[2]) + 2;
	ngen = atoi(argv[3]);

	bool **a = NULL;		
	bool *b, *dA, *dB;

	dim3 multiBlockArray(x, y);

	a = allocarray(x, y);
	b = (bool *)malloc(sizeof(bool) * x * y);

	a = initarray(a, x, y);
	b = squashify(a, b, x, y);
	b = copyGhostCells(b, x, y);


	starttime = gettime();	
	//printarray(a, x, y);		
	
	cudaMalloc(&dA, sizeof(bool) * x * y);
	cudaMalloc(&dB, sizeof(bool) * x * y);

	for(j = 0; j < ngen; j++) {
		cudaMemcpy(dA, b, sizeof(bool) * x * y, cudaMemcpyHostToDevice);
		checkNeighbors<<<multiBlockArray,1>>>(dA, dB, x, y);
		cudaMemcpy(b, dB, sizeof(bool) * x * y, cudaMemcpyDeviceToHost);
		b = copyGhostCells(b, x, y);
		//a = desquashify(b, a, x, y);
		//printarray(a, x, y);
	}

	endtime = gettime();
	printf("Time taken for test = %lf seconds\n", endtime-starttime);

	cudaFree(dA);
	cudaFree(dB);
	free(b);

	return 0;
}
