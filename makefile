libpgas.a: pgas.c
	mpicc -c pgas.c
	ar rvf libpgas.a pgas.o

pgas_test: pgas_test.c libpgas.a
	mpicc -o pgas_test pgas_test.c -lpgas -lpthread -L.

clean:
	rm -f libpgas.a pgas_test *.o
