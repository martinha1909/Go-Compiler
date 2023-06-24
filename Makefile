all:
	bison -d parser.y
	gcc -Wall -o golf *.c


clean:
	rm -rf *.o *.tab.* golf

