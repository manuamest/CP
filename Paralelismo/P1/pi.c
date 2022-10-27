#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int i, done = 0, n, count, countlocal;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z;
    
    int numprocs, rank, k;

    MPI_Status status;

    MPI_Init(&argc, &argv);							//Initialize MPI enviroment
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);					//get number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);					//get self rank
    
    while (!done)
    {
        if (rank == 0)
        {
            printf("Enter the number of points: (0 quits) \n");
            scanf("%d",&n);
            for (k = 1; k < numprocs ; k++) {
                MPI_Send(&n, 1, MPI_INT, k, 0, MPI_COMM_WORLD);			//Sends buffer from 0
            }
        } else {
            MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);		//Every process receive buffer
        }
        
        if (n == 0) break;

        count = 0;  

        for (i = 1 + rank; i <= n; i += numprocs) {				//Every process starts in its own point
            // Get the random numbers between 0 and 1
        x = ((double) rand()) / ((double) RAND_MAX);
        y = ((double) rand()) / ((double) RAND_MAX);

        // Calculate the square root of the squares
        z = sqrt((x*x)+(y*y));

        // Check whether z is within the circle
        if(z <= 1.0)
                count++;
        }
        
        if (rank > 0) {
            MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);			//Sends local count to process 0
        } else {
            for (k = 1; k < numprocs ; k++) {
            	MPI_Recv(&countlocal, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);	//Process 0 receives all local counts
            	count += countlocal;
            }
            pi = ((double) count/(double) n)*4.0;
            printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
        }
    }
    MPI_Finalize();								//Finalize MPI enviroment
}
