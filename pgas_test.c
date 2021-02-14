#include "mpi.h"
#include "pgas.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_INTS_PER_RANK 10

static MPI_Datatype handle_type;

int main(int argc, char *argv[])
{
    int i, j, k, rc, hidx, rank, myrank, comm_world_sz, provided;
    MPI_Comm app_comm;
    PGAS_HANDLE mypkthandle, dhs[1000];
    MPI_Aint memsize;
    int *mypktvec, pktsz;
    int (*mypktarr)[NUM_INTS_PER_RANK];
    int mypartpkt[NUM_INTS_PER_RANK];

    MPI_Init_thread(0, 0, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE) {
        printf("insufficient level of thread safety provided by MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD,777);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_world_sz);

    // if (comm_world_sz < 2)
    // {
        // printf("2 or more ranks required for PGAS\n");
        // MPI_Abort(MPI_COMM_WORLD,776);
    // }


    MPI_Comm_dup(MPI_COMM_WORLD, &app_comm);

    MPI_Type_vector(3, 1, 1, MPI_AINT, &handle_type);
    MPI_Type_commit(&handle_type);

    pktsz = sizeof(int) * NUM_INTS_PER_RANK * comm_world_sz;
    mypktvec = malloc( pktsz );
    mypktarr = (int (*)[NUM_INTS_PER_RANK]) mypktvec;  // same memory

    // PGAS_Init(app_comm, pktsz * 10,  5);  // 10 -> arbitrary ; 5
    if ((myrank % 2) == 1)
        PGAS_Init(pktsz * 4);  // 4 is arbitrary (holds up to 4 pkts)
    else
        PGAS_Init(0);  // even ranks have NO space for the server in this application
    MPI_Barrier(MPI_COMM_WORLD);

    // put my packet and get it back making sure it is ok
    for (rank=0; rank < comm_world_sz; rank++)
    {
        for (i=0; i < NUM_INTS_PER_RANK; i++)
        {
            mypktarr[rank][i] = myrank * 1000 + i;
            // printf("%d: STORE %d %d: %d\n", myrank,rank,i,mypktarr[rank][i]);
        }
    }

    PGAS_Put(mypktvec, pktsz, mypkthandle);

    memset(mypktvec,0,pktsz);  // to make sure Get works
    // rc = PGAS_Get(mypkthandle, mypktvec);
    rc = PGAS_Get(mypkthandle, 0, pktsz, mypktvec);
    for (rank=0; rank < comm_world_sz; rank++)
    {
        for (i=0; i < NUM_INTS_PER_RANK; i++)
        {
            if (mypktarr[rank][i] != (myrank * 1000 + i))
            {
                printf("bad match in top Get, %d : %d\n",mypktarr[rank][i],(myrank*1000+i));
                MPI_Abort(MPI_COMM_WORLD,-1);
            }
        }
    }

    // obtain handles of all the packets that were put
    printf("%d: mypkthandle  srvr %ld\n",myrank,mypkthandle[0]);
    MPI_Allgather( (void *)mypkthandle, 1, handle_type,
                   (void *)dhs,         1, handle_type,
                   MPI_COMM_WORLD
                 );

    // get my part of every pkt, chg it, and update it (put it back)
    for (rank=0; rank < comm_world_sz; rank++)
    {
        memset(mypartpkt,0,NUM_INTS_PER_RANK*sizeof(int));
        PGAS_Get(dhs[rank], myrank * NUM_INTS_PER_RANK * sizeof(int),
                 NUM_INTS_PER_RANK * sizeof(int), mypartpkt);
        for (i=0; i < NUM_INTS_PER_RANK; i++)
        {
            if (mypartpkt[i] != (rank * 1000 + i))  // the other guy's rank
            {
                printf("%d: bad match in Get_part, %d : %d\n",myrank,mypartpkt[i],(rank*1000+i));
                MPI_Abort(MPI_COMM_WORLD,9999);
            }
            mypartpkt[i] = myrank * 10000 + i;   // chg mypartpkt 1000->10000
        }
        // update it (put it back)
        PGAS_Update(dhs[rank], myrank * NUM_INTS_PER_RANK * sizeof(int),
                    NUM_INTS_PER_RANK * sizeof(int), mypartpkt);
    }

    // make sure everybody has done all their updates
    MPI_Barrier(MPI_COMM_WORLD);
    
    memset(mypktvec,0,pktsz);
    rc = PGAS_Get(mypkthandle, 0, pktsz, mypktvec);
    for (rank=0; rank < comm_world_sz; rank++)
    {
        for (i=0; i < NUM_INTS_PER_RANK; i++)
        {
            if (mypktarr[rank][i] != (rank * 10000 + i))
            {
                printf("bad match in bot Get, %d : %d\n",mypktarr[rank][i],rank*10000+i);
                MPI_Abort(MPI_COMM_WORLD,-1);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    printf("%d: DONE\n",myrank);

    PGAS_Finalize();
    MPI_Finalize();

    return 0;
}  
