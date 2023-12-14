clean:	
	rm *.o main

main: main.c utils.c threadpool.c utils2.c
	gcc main.c utils.c threadpool.c utils2.c -o main