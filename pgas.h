#ifndef PGAS_H_
#define PGAS_H_

#include <mpi.h>

#define PGAS_ERROR 1
#define PGAS_SUCCESS 0
#define PGAS_NO_SPACE 2

typedef MPI_Aint PGAS_HANDLE[3]; // server, address, size

int PGAS_Init(int max_bytes);
int PGAS_Put(void *buffer, int size, PGAS_HANDLE handle);
int PGAS_Update(PGAS_HANDLE handle, int offset, int size, void *buffer);
int PGAS_Get(PGAS_HANDLE handle, int offset, int size, void *buffer);
int PGAS_Finalize();

#endif
