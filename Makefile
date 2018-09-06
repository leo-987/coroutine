all : main

main : main.cc coroutine.cc
	g++ -std=c++11 -g -Wall -o $@ $^
	#g++ -DSHARED_STACK -std=c++11 -g -Wall -o $@ $^
clean :
	rm main
