#include <stdlib.h>
#include <iostream>
#include "mpi.h"

using namespace std;

MPI_Status status;

int main(int argc, char **argv) {

    // MPI
    int rank, size, rows, offset, source;


    // Init matrix
    int row_number, column_number = 4;

    double a[row_number][column_number],b[row_number][column_number],c[row_number][column_number];

    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_COMM_size(MPI_COMM_WORLD, &size);

    int slaves = size - 1;


    // -- master --

    if (rank == 0) {
        for(int i=0; i == row_number; i++) {
            for(int j=0; j == column_number; j++) {
                a[i][j] = rand() % 100;
                b[i][j] = rand() % 100;
            }

        }

        rows = row_number/slaves;
        offset = 0;

        for (int msg=1; msg<= slaves; msg++) {
            MPI_Send(&offset, 1, MPI_INT, msg, 1, MPI_COMM_WORLD);
            MPI_Send(&rows, 1, MPI_INT, msg, 1, MPI_COMM_WORLD);
            MPI_Send(&a[offset][0], rows*row_number, MPI_DOUBLE,msg,1, MPI_COMM_WORLD);
            MPI_Send(&b, row_number*column_number, MPI_DOUBLE, msg, 1, MPI_COMM_WORLD);
            offset = offset + rows;
        }

        for (int i=1; i<=slaves; i++) {
        
        source = i;

        MPI_Recv(&offset, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &status);
        MPI_Recv(&rows, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &status);
        MPI_Recv(&c[offset][0], rows*row_number, MPI_DOUBLE, source, 2, MPI_COMM_WORLD, &status);
        }

        for (int i=0; i<row_number; i++) {
            for (int j=0; j<column_number; j++) {
                cout << " " << c[i,j] << " ";
            }
         cout <<  " " << endl;
        }

    }

    source = 0;
    MPI_Recv(&offset, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
    MPI_Recv(&rows, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
    MPI_Recv(&a, rows*row_number, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &status);
    MPI_Recv(&b, row_number*column_number, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &status);

    for (int k=0; k<row_number; k++)
      for (int i=0; i<rows; i++) {
        c[i][k] = 0.0;
        for (int j=0; j<N; j++)
          c[i][k] = c[i][k] + a[i][j] * b[j][k];
      }

    
    MPI_Send(&offset, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
    MPI_Send(&rows, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
    MPI_Send(&c, rows*row_number, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}