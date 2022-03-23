# PGAS
Partitioned Global Address Space

------------------------------------------------------------------------------
------------------------------------------------------------------------------

All credit for this (PGAS) project belongs to Dr. Ralph Butler, who I studied under while attending MTSU. My contributions are the library implementation provided in the pgas.c and pgas.h files, as well as the makefile to link the libraries and compile a test program. Credit for the demo / test files go to him - this was a classroom "fill in the blanks" project. If you're a current student - please don't lift this implementation : )

This library is meant to be run on a cluster using MPI. As such, you'll need MPI configured, and you'll need to provide a file enumerating the host servers.

The project description is as follows:

------------------------------------------------------------------------------
------------------------------------------------------------------------------

Using C/C++, implement a parallel library that provides the user access
to a PGAS system that conforms to the interface described here:

    #include "pgas.h"

    return codes:
        PGAS_SUCCESS    successful operation
        PGAS_NO_SPACE   returned by Put if a data item can not be taken by
                        any server
        PGAS_ERROR      failure (e.g. internal error)

    typedef MPI_Aint PGAS_HANDLE[3];  // created by PGAS by Put
        0 :  server where data is stored
        1 :  address on the server where the data is actually stored
        2 :  size of the stored data

    int PGAS_Init(int max_bytes);
        max_bytes is the maximum number of bytes the PGAS system should
        use on this host before it refuses to take additional data.

    int PGAS_Put(void *buffer, int size, PGAS_HANDLE handle);
        buffer: pointer to the user's data to be stored
        size:   number of bytes to retrieve from the buffer
        handle: filled in by PGAS; described above

    int PGAS_Update(PGAS_HANDLE handle, int offset, int size, void *buffer);
        handle:  pgas handle of remote buffer (created via prior Put)
        offset:  offset from addr at which to perform the update
        size:    number of bytes to update
        addr:    addr of data to place into the remote buffer

    int PGAS_Get(PGAS_HANDLE handle, int offset, int size, void *buffer);
        handle:  obtained from a prior Put
        offset:  distance from beginning of buffer (often 0)
        size:    num bytes to retrieve from the stored data
        buffer:  buffer into which data is to be retrieved

    int PGAS_Finalize();
        cleanup and shutdown of the PGAS system (threads etc)

The PGAS library code should run in a separate thread created during
PGAS_Init.  The library should make sure that it establishes its own
communicator for its work.

The makefile should build a library named libpgas.a when the user simply
types "make" with no arguments, e.g.:
    make

The makefile should also provide a target that will compile (and link)
a C (not C++) program named pgas_test.c into an executable named pgas_test.
Note that, if your library is implemented in C++, then you must take
care to link the C program and C++ library correctly.

The user's programs can be run in this way:

    mpiexec -f some_hosts_filename  -n num_ranks  ./pgas_test  user_pgm_args

