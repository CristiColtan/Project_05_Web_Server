clean:	
	rm *.o main

main: main.c utils.c threadpool.c
	gcc main.c utils.c threadpool.c -o main