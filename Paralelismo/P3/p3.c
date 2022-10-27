#include <stdio.h>
#include <sys/time.h>
#include <mpi.h>

// 0 -> Tempos
// 1 -> Resultado vector

#define DEBUG 1

#define N 1024    // Dimensión

int main(int argc, char *argv[] ) {

  int i, j;
  int numprocs, rank, k;
  struct timeval  tcomp1, tcomp2, tcomm1, tcomm2;
  int dimension = N;    // Variable da redimensión no caso de que non sexa múltiplo do núm de procesos
  int filas_proc, elem_proc;
  float vector[N];    // Vector dado
  float result[N];    // Vector resultado


  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);   // Núm de procesos
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   // Proceso no que estamos
  
  while (dimension % numprocs != 0) {   // Redimensionamos a matriz se é preciso ata que sexa múltiplo de numprocs
  	dimension++;
  }


  float matrix[dimension][N];   // Declaramos a matriz dada
  

  if (rank == 0){   

  
    // O proceso 0 inicializa a matriz e o vector
    for(i=0; i < N; i++) {
      vector[i] = i;
      for(j=0; j < N; j++) {
        matrix[i][j] = i+j;
      }
    }

    // Se se aumentaron o núm de filas inicializámolas a 0 (se non se aumentou non entramos no bucle)
    for(int a = i; a < dimension; a++){   
      for(j=0; j < N; j++){
        matrix[a][j] = 0;
      }
    }
  }

  filas_proc = dimension / numprocs;    // Calculamos o núm de filas/proceso
  elem_proc = filas_proc * N;           // Calculamos o núm de elementos/proceso

  float matrix_proc [filas_proc][N];    // Declaramos a matriz coa que traballará cada proceso

  gettimeofday(&tcomm1, NULL);          // Tempo inicial da COMUNICACIÓN 1

  MPI_Scatter(matrix, elem_proc, MPI_FLOAT, matrix_proc, elem_proc, MPI_FLOAT, 0, MPI_COMM_WORLD);    // Repartimos os bloques da matriz dada nas matrices de cada proceso
  MPI_Bcast(vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD);   // Compartimos o vector dado con todos os procesos
  
  gettimeofday(&tcomm2, NULL);          // Tempo final de COMUNICACIÓN 1
  
  int microsecondsComm = (tcomm2.tv_usec - tcomm1.tv_usec)+ 1000000 * (tcomm2.tv_sec - tcomm1.tv_sec);  // Calculamos tempo parcial de COMUNICACIÓN
  
  gettimeofday(&tcomp1, NULL);          // Tempo inicial da COMPUTACIÓN

  float result_proc[N];                 // Declaramos o vector resultado para cada proceso

  for(i=0; i < filas_proc; i++) {       // Inicializamos o vector resultado a 0
    result_proc[i]=0;

    for(j=0;j < N; j++)                                   
      result_proc[i] += matrix_proc[i][j] * vector[j];    // Calculamos a multiplicación da matriz X vector
  }

  gettimeofday(&tcomp2, NULL);          // Tempo final de CÓMPUTACIÓN
  int microsecondsComp = (tcomp2.tv_usec - tcomp1.tv_usec)+ 1000000 * (tcomp2.tv_sec - tcomp1.tv_sec);  // Calculamos tempo total de COMPUTACIÓN

  gettimeofday(&tcomm2, NULL);          // Tempo final de COMUNICACIÓN 2

  MPI_Gather(result_proc, filas_proc, MPI_FLOAT, result, filas_proc, MPI_FLOAT, 0, MPI_COMM_WORLD);     // Recolectamos todos os resultados de cada proceso nun único vector resultado

  gettimeofday(&tcomm2, NULL);          // Tempo final de COMUNICACIÓN 2
  
  microsecondsComm += (tcomm2.tv_usec - tcomm1.tv_usec)+ 1000000 * (tcomm2.tv_sec - tcomm1.tv_sec);  // Calculamos tempo total de COMUNICACIÓN


  // Imprimimos RESULTADO
  if (DEBUG){   

    if (rank == 0){                     // Só imprime proceso 0
      for(i=0; i < N; i++) 
        printf(" %.2f \t ",result[i]);
    }

  // Imprimimos TEMPOS
  } else {      
    printf ("Process %d -> Time Comm (seconds) = %lf\n", rank, (double) microsecondsComm/1E6);
    printf ("Process %d -> Time Comp (seconds) = %lf\n", rank, (double) microsecondsComp/1E6);
  }    

  MPI_Finalize();

  return 0;
}
