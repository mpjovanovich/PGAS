#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <pthread.h>
#include "pgas.h"

#define PGAS_FINALIZE_CODE 101
#define PGAS_PUT_CONTROL_CODE 102
#define PGAS_PUT_CODE 103
#define PGAS_UPDATE_CODE 104
#define PGAS_GET_CODE 105

pthread_t my_thread;
int my_max_bytes, myrank, num_procs;
MPI_Comm pgas_comm;

void *PGAS_recv_message(void *arg)
{
    MPI_Status status;
    int request_code, i, last_put_rank=0;
    int remaining_space[num_procs];

    // Receive max bytes info from all ranks
    MPI_Gather(&my_max_bytes,1,MPI_INT,remaining_space,1,MPI_INT,0,pgas_comm);

    while( 1 == 1 )
    {
        MPI_Recv(&request_code,1,MPI_INT,MPI_ANY_SOURCE,1,pgas_comm,&status);
        
        // Finalize
        if( request_code == PGAS_FINALIZE_CODE )
        {
            pthread_exit(NULL);
        }
        // Put Controller
        if( request_code == PGAS_PUT_CONTROL_CODE )
        {
            int size, i;
            int put_rank = (last_put_rank + 1) % num_procs;
            int success = 0;

            MPI_Recv(&size,1,MPI_INT,status.MPI_SOURCE,220,pgas_comm,&status); // Get size
            for( i=0; i<num_procs; ++i )
            {
                if( remaining_space[put_rank] >= size )
                {
                    remaining_space[put_rank] -= size;
                    success = 1;
                    break;
                }
                put_rank = (put_rank + 1) % num_procs;
            }

            if( success )
            {
                last_put_rank = put_rank;
                MPI_Rsend(&put_rank,1,MPI_INT,status.MPI_SOURCE,221,pgas_comm); // Send put rank
            }
            else
            {
                put_rank = -1;
                MPI_Rsend(&put_rank,1,MPI_INT,status.MPI_SOURCE,221,pgas_comm); // Send failure flag
            }
        }
        // Put
        if( request_code == PGAS_PUT_CODE )
        {
            int size;
            void *b;
            MPI_Request req;

            MPI_Recv(&size,1,MPI_INT,status.MPI_SOURCE,230,pgas_comm,&status); // Get size
            b = malloc(size); // Allocate space
            MPI_Irecv(b,size,MPI_BYTE,status.MPI_SOURCE,231,pgas_comm,&req); // Set up to receive data
            MPI_Rsend(&b,1,MPI_AINT,status.MPI_SOURCE,232,pgas_comm); // Send confirm to receive
        }
        // Update
        if( request_code == PGAS_UPDATE_CODE )
        {
            char *b;
            int offset;
            int size;
            MPI_Aint address_info[3]; // address, offset, size 
            MPI_Request req;

            MPI_Recv(&address_info,3,MPI_AINT,status.MPI_SOURCE,240,pgas_comm,&status); // Get address info 
            b = (char *)address_info[0];
            offset = address_info[1];
            size = address_info[2];
            b += offset;
            MPI_Irecv(b,size,MPI_BYTE,status.MPI_SOURCE,241,pgas_comm,&req); // Set up to receive data
            MPI_Rsend(NULL,0,MPI_INT,status.MPI_SOURCE,242,pgas_comm); // Send confirm to receive
        }
        // Get
        if( request_code == PGAS_GET_CODE )
        {
            char *b;
            int offset;
            int size;
            MPI_Aint address_info[3]; // address, offset, size 

            MPI_Recv(&address_info,3,MPI_AINT,status.MPI_SOURCE,250,pgas_comm,&status); // Get address info 
            b = (char *)address_info[0];
            offset = address_info[1];
            size = address_info[2];
            b += offset;
            MPI_Rsend(b,size,MPI_BYTE,status.MPI_SOURCE,251,pgas_comm); // Send data
        }
    }
}

int PGAS_Init(int max_bytes)
{
    int is_init, provided;
    my_max_bytes = max_bytes;

    // Create pgas comm
    MPI_Comm_dup( MPI_COMM_WORLD, &pgas_comm );
    MPI_Comm_rank( pgas_comm, &myrank );
    if( myrank == 0 )
        MPI_Comm_size( pgas_comm, &num_procs );

    // TODO: add checking for failed thread create
    // Set up MPI PGAS thread for each rank
    if( pthread_create( &my_thread, NULL, PGAS_recv_message, NULL ) != 0 )
    {
        printf( "pthread_create failed\n" );
        return PGAS_ERROR;
    }

    // TODO: If MPI isn't initialed then do that here?

    return PGAS_SUCCESS;
}

int PGAS_Put(void *buffer, int size, PGAS_HANDLE handle)
{
    MPI_Request req;
    MPI_Status status;
    int request_code_put_control = PGAS_PUT_CONTROL_CODE;
    int request_code_put = PGAS_PUT_CODE;
    MPI_Aint a;
    int put_rank;

    MPI_Irecv(&put_rank,1,MPI_INT,0,221,pgas_comm,&req);
    MPI_Send(&request_code_put_control,1,MPI_INT,0,1,pgas_comm);
    MPI_Send(&size,1,MPI_INT,0,220,pgas_comm);
    MPI_Wait(&req,&status);

    if( put_rank == -1 )
        return PGAS_NO_SPACE;

    MPI_Send(&request_code_put,1,MPI_INT,put_rank,1,pgas_comm);
    MPI_Irecv(&a,1,MPI_AINT,put_rank,232,pgas_comm,&req);
    MPI_Send(&size,1,MPI_INT,put_rank,230,pgas_comm);
    MPI_Wait(&req,&status);
    MPI_Rsend(buffer,size,MPI_BYTE,put_rank,231,pgas_comm);

    handle[0] = put_rank;
    handle[1] = a;
    handle[2] = size;

    return PGAS_SUCCESS;
}

int PGAS_Update(PGAS_HANDLE handle, int offset, int size, void *buffer)
{
    MPI_Request req;
    MPI_Status status;
    int request_code = PGAS_UPDATE_CODE;
    MPI_Aint address_info[3];

    address_info[0] = handle[1];
    address_info[1] = offset;
    address_info[2] = size;

    MPI_Send(&request_code,1,MPI_INT,handle[0],1,pgas_comm);
    MPI_Irecv(NULL,0,MPI_INT,handle[0],242,pgas_comm,&req);
    MPI_Send(address_info,3,MPI_AINT,handle[0],240,pgas_comm);
    MPI_Wait(&req,&status);
    MPI_Rsend(buffer,size,MPI_BYTE,handle[0],241,pgas_comm);

    return PGAS_SUCCESS;
}

int PGAS_Get(PGAS_HANDLE handle, int offset, int size, void *buffer)
{
    MPI_Request req;
    MPI_Status status;
    int request_code = PGAS_GET_CODE;
    MPI_Aint address_info[3];

    address_info[0] = handle[1];
    address_info[1] = offset;
    address_info[2] = size;
    
    MPI_Send(&request_code,1,MPI_INT,handle[0],1,pgas_comm);
    MPI_Irecv(buffer,size,MPI_BYTE,handle[0],251,pgas_comm,&req);
    MPI_Send(address_info,3,MPI_AINT,handle[0],250,pgas_comm);
    MPI_Wait(&req,&status);

    return PGAS_SUCCESS;
}

int PGAS_Finalize()
{
    int request_code = PGAS_FINALIZE_CODE;
    MPI_Barrier(pgas_comm);

    // Kill child threads
    MPI_Send(&request_code,1,MPI_INT,myrank,1,pgas_comm);

    // TODO: do I need to call mpi finalize here?
    return PGAS_SUCCESS;
}
