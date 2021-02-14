#include "mpi.h"
#include "pgas.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{
    int i, j, k, rc, myrank, comm_world_sz, provided;
    PGAS_HANDLE handles[1000];
    MPI_Aint memsize;
    int *mypktvec, pktsz;

    MPI_Init_thread(0, 0, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        printf("insufficient level of thread safety provided by MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD,777);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_world_sz);

    pktsz = 1000000;
    mypktvec = malloc( pktsz );
    memset(mypktvec,1,pktsz);  // fill with 1s

    if ((myrank % 2) == 1)
        PGAS_Init(pktsz * 8);  // 8 is arbitrary (holds up to 8 pkts)
    else
        PGAS_Init(0);  // even ranks have NO space for the server in this application
    MPI_Barrier(MPI_COMM_WORLD);

    for (i=0; i < 10000; i++)
    {
        rc = PGAS_Put(mypktvec, pktsz, handles[i]);
        if (rc == PGAS_NO_SPACE)
        {
            printf("%d: PUT FAILED at %d with %d\n",myrank,i,rc);
            MPI_Abort(MPI_COMM_WORLD,-1);
        }
        printf("%d: Put at server %ld\n",myrank,handles[i][0]);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    printf("%d: DONE\n",myrank);

    PGAS_Finalize();
    MPI_Finalize();

    return 0;
}  
