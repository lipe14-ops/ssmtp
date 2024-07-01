EXE = ssmtpserver
FILE = ./src/main.c
CC = gcc
CFLAGS = -Wall

all: $(EXE)
	./dist/$(EXE)

$(EXE): $(FILE)
	$(CC) $(CFLAGS) $^ -o ./dist/$@
