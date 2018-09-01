all : main

main : main.cc coroutine.cc
	#g++ -g -Wall -o $@ $^
	g++ -DSHARED_STACK  -g -Wall -o $@ $^
clean :
	rm main
