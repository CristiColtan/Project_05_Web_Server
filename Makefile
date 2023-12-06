clean:	
	rm *.o main

main: main.c utils.c
	gcc main.c utils.c -o main