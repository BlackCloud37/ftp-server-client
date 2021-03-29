# all: cmd_handler stdinc server
# $(shell if [ ! -e temp ];then mkdir -p temp; fi)
# $(shell if [ ! -e bin ];then mkdir -p bin; fi)
# cmd_handler: include/cmd_handler.h src/cmd_handler.c
# 	gcc -c src/cmd_handler.c -o temp/cmd_handler.o -Wall
# stdinc: include/stdinc.h src/stdinc.c 
# 	gcc -c src/stdinc.c -o temp/stdinc.o -Wall
# server: include/server.h src/server.c
# 	gcc -c src/server.c -o temp/server.o -Wall
# 	gcc -o bin/server temp/server.o temp/cmd_handler.o temp/stdinc.o -lpthread
# run:
# ./bin/server

server: include/* src/*
	gcc -o server src/server.c src/stdinc.c src/cmd_handler.c -Wall -lpthread

clean:
	rm server