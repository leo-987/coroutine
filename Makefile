all : main

main : main.c coroutine.c
	gcc -g -Wall -o $@ $^
	#gcc -DSHARED_STACK  -g -Wall -o $@ $^
clean :
	rm main
