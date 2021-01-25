stickyfinger: main.o tarball.o
	gcc -o stickyfingers -lm -lncurses main.o tarball.o

main.o: main.c tarball.h
	gcc -c main.c tarball.h

tarball.o: tarball.c tarball.h
	gcc -c tarball.c

tarball_test.o: tarball_test.c tarball.h
	gcc -c tarball_test.c

tarball_test: tarball.o tarball_test.o
	gcc -lm -o tarball_test tarball.o tarball_test.o

.PHONY: clean test run

clean: 
	-rm *.o

test: tarball_test
	echo "ARCHIVE TEST"
	echo "hello world" > hello.txt
	./tarball_test -c hello.tar hello.txt
	tar -cvf hello2.tar hello.txt
	diff -s hello.tar hello2.tar
	echo "EXTRACT TEST"
	mv hello.txt hello2.txt
	./tarball_test -x hello.tar
	diff -s hello.txt hello2.txt
	-rm tarball_test *.txt *.o *.tar

run: stickyfingers
	./stickyfingers
