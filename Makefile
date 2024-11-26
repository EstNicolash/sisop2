CC = gcc

# Bibliotecas
LIBS = -pthread
FLAGS = -Wall
# Diretórios
SRC = ./src/
OBJ = ./obj/
HDR = ./headers/
BIN = ./bin/
CLIENT_NAME = client
SERVER_NAME = server

All:
	$(CC) -c $(SRC)file_manager.c -I $(HDR) -o $(OBJ)file_manager.o $(LIBS) $(FLAGS)
	$(CC) -c $(SRC)protocol.c -I $(HDR) -o $(OBJ)protocol.o $(LIBS) $(FLAGS)
	$(CC) -c $(SRC)client_commands.c -I $(HDR) -o $(OBJ)client_commands.o $(LIBS) $(FLAGS)
	$(CC) -c $(SRC)server_handlers.c -I $(HDR) -o $(OBJ)server_handlers.o $(LIBS) $(FLAGS)
	$(CC) $(SRC)$(CLIENT_NAME).c $(OBJ)*.o -I $(HDR) -o $(BIN)$(CLIENT_NAME).out $(LIBS) $(FLAGS)
	$(CC) $(SRC)$(SERVER_NAME).c $(OBJ)*.o -I $(HDR) -o $(BIN)$(SERVER_NAME).out $(LIBS) $(FLAGS)

clean:
	rm -f $(BIN)*.out $(OBJ)*.o
