tarball.o: tarball.c tarball.h
	gcc -c tarball.c

tarball_test.o: tarball_test.c tarball.h
	gcc -c tarball_test.c

tarball_test: tarball.o tarball_test.o
	gcc -o tarball_test tarball.o tarball_test.o

.PHONY: clean test

clean: 
	-rm *.o *.tar

test: tarball_test
	echo "hello world" > hello.txt
	tar -cvf hello2.tar hello.txt
	./tarball_test -o hello.tar hello.txt
	-rm hello.txt
