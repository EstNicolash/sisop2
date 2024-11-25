CC = gcc

# Bibliotecas
LIBS = -lpthreads
FLAGS = -Wall
# Diretórios
SRC = ./src/
OBJ = ./obj/
HDR = ./headers/
BIN = ./bin/
CLIENT_NAME = client
SERVER_NAME = client

All:
#	$(CC) -c $(SRC)db_transactions.c -I $(HDR) -o $(OBJ)db_transactions.o $(LIBS) $(FLAGS)
#	$(CC) -c $(SRC)app.c -I $(HDR) -o $(OBJ)app.o $(LIBS) $(FLAGS)
#	$(CC) $(SRC)$(CLIENT_NAME).c $(OBJ)*.o -I $(HDR) -o $(BIN)$(CLIENT_NAME).out $(LIBS) $(FLAGS)
#	$(CC) $(SRC)$(CLIENT_NAME).c $(OBJ)*.o -I $(HDR) -o $(BIN)$(CLIENT_NAME).out $(LIBS) $(FLAGS)

clean:
	rm $(BIN)*.out $(OBJ)*.o
