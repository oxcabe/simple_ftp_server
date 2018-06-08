ARG=-g -std=gnu++0x -lpthread
BIN=bin/ftp_server

all: ftp_server



ftp_server: 
	g++ $(ARG) $(wildcard src/* include/*)  -o $(BIN)

clean:
	rm $(BIN)
