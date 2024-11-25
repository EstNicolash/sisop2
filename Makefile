CC = gcc

# Bibliotecas
LIBS = -lpthread
FLAGS = -Wall
# Diretórios
SRC = ./src/
OBJ = ./obj/
HDR = ./headers/
BIN = ./bin/
CLIENT_NAME = client
SERVER_NAME = client

All:
	$(CC) -c $(SRC)file_manager.c -I $(HDR) -o $(OBJ)file_manager.o $(LIBS) $(FLAGS)
	$(CC) $(SRC)$(CLIENT_NAME).c $(OBJ)*.o -I $(HDR) -o $(BIN)$(CLIENT_NAME).out $(LIBS) $(FLAGS)

clean:
	rm -f $(BIN)*.out $(OBJ)*.o
