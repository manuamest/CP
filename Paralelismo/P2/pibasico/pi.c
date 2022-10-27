#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>



int MPI_BinomialCast(void * buff, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
	
}

int MPI_FlattreeReduce(void * buff, void * recvbuff , int count , MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm ) {
	
}

int main(int argc, char *argv[])
{
    int i, done = 0, n, count, countdest;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z;
    
    double pilocal;
    int numprocs, rank, k;

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    while (!done)
    {
        if (rank == 0)
        {
            printf("Enter the number of points: (0 quits) \n");
            scanf("%d",&n);
            /*
            for (k = 1; k < numprocs ; k++) {
                MPI_Send(&n, 1, MPI_INT, k, 0, MPI_COMM_WORLD);
            }
            */
            
        } else {
            //MPI_Recv(&n, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            
        }
        
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        if (n == 0) break;

        count = 0;  

        for (i = 1 + rank; i <= n; i += numprocs) {
            // Get the random numbers between 0 and 1
        x = ((double) rand()) / ((double) RAND_MAX);
        y = ((double) rand()) / ((double) RAND_MAX);

        // Calculate the square root of the squares
        z = sqrt((x*x)+(y*y));

        // Check whether z is within the circle
        if(z <= 1.0)
                count++;
        }
        /*
        if (rank > 0) {
            MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        } else {
            for (k = 1; k < numprocs ; k++) {
            MPI_Recv(&countlocal, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            count += countlocal;
            }
            pi = ((double) count/(double) n)*4.0;
            printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
        }
        */
        MPI_Reduce(&count, &countdest, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        pi = ((double) countdest/(double) n)*4.0;
        if (rank == 0) {
	        printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
    	}
    }
    MPI_Finalize();
}
