ARG=-g -std=gnu++0x -lpthread
BIN=bin
NAME=ftp_server

all: ftp_server



ftp_server:
	@mkdir -p $(BIN)
	g++ $(ARG) $(wildcard src/* include/*)  -o $(BIN)/$(NAME)

clean:
	rm $(BIN)
