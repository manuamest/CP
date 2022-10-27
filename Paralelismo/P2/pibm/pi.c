#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>


/*
int MPI_BinomialCast(void * buff, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    int error, k;
    int numprocs, rank;
    int receiver, sender;

    MPI_Status status;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (k = 1; k <= ceil(log2(numprocs)); k++)
        if (rank < ipow(2, k-1))
        {
            receiver = rank + ipow(2, k-1);
            if (receiver < numprocs)
            {
                error = MPI_Send(buf, count, datatype, receiver, 0, comm);
                if (error != MPI_SUCCESS) return error;
            }
        } else
        {
            if (rank < ipow(2, k))
            {
                sender = rank - ipow(2, k-1);
                error = MPI_Recv(buf, count, datatype, sender, 0, comm, &status);
                if (error != MPI_SUCCESS) return error;
            }
        }
    return MPI_SUCCESS;
}
FALTA IMPLEMENTAR TODA LA PARTE DEFINIDA EN EL PDF EN FORMA DE ARBOL
*/
int MPI_BinomialCast(void * buff, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    int numprocs, rank, k, error;
    int envia, recibe, pot;
    MPI_Status status;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    for ( k = 1; k <= ceil(log2(numprocs)); ++k) {
        pot = pow(2, k-1);
        if (rank < pot) {
            recibe = rank + pot;
            if (recibe < numprocs) {
                error = MPI_Send(buff, count, datatype, recibe, 0, comm);
                if (error != MPI_SUCCESS) return error;
            }
        } else {
            if (rank < pow(2, k)) {
                envia = rank-pot;
                error = MPI_Recv(buff, count, datatype, envia, MPI_ANY_TAG, comm, &status);
                if (error != MPI_SUCCESS) return error;
            }
        }
    }
    return MPI_SUCCESS;
}


int MPI_FlattreeReduce(void * buff, void * recvbuff , int count , MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm ) {
    int error, k;
    int numprocs, rank;

    MPI_Status status;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int *countlocal = buff, *countdest = recvbuff;
    
        if (rank != root) {
            error = MPI_Send(buff, 1, MPI_INT, root, 0, MPI_COMM_WORLD);
            if (error != MPI_SUCCESS) return error;
        } else {
            for (k = 1; k < numprocs ; k++) {
            	if(k != root) {
            	    error = MPI_Recv(recvbuff, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            	    if (error != MPI_SUCCESS) return error;
                    *countlocal += *countdest;
            	}
            }
        }
    return MPI_SUCCESS;
}

int main(int argc, char *argv[])
{
    int i, done = 0, n, count, countlocal;
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
        }
        
        //MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_BinomialCast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
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
        MPI_Reduce(&count, &countlocal, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);pi = ((double) countlocal/(double) n)*4.0;
        //MPI_FlattreeReduce(&count, &countlocal, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);pi = ((double) count/(double) n)*4.0;
        if (rank == 0) {
	        printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
    	}
    }
    MPI_Finalize();
}
