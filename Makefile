tarball.o: tarball.c tarball.h
	gcc -c tarball.c

tarball_test.o: tarball_test.c tarball.h
	gcc -c tarball_test.c

tarball_test: tarball.o tarball_test.o
	gcc -o tarball_test tarball.o tarball_test.o

.PHONY: clean test

clean: 
	-rm *.o

test: tarball_test
	echo "ARCHIVE TEST"
	echo "hello world" > hello.txt
	./tarball_test -c hello.tar hello.txt
	tar -cvf hello2.tar hello.txt
	diff -s hello.tar hello2.tar
	-rm tarball_test hello.txt *.o *.tar
