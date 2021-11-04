CC = gcc
CFLAGS = -Wall -Wextra
EFLGAS = -pthread
SERVER_DIR = ./server
CLIENT_DIR = ./client

all: $(SERVER_DIR)/server $(CLIENT_DIR)/client

$(SERVER_DIR)/server: $(SERVER_DIR)/source_server.c
	$(CC) $(CFLAGS) $(EFLGAS) -o $(SERVER_DIR)/server $(SERVER_DIR)/source_server.c common.c

$(CLIENT_DIR)/client: $(CLIENT_DIR)/source_client.c
	$(CC) $(CFLAGS) -o $(CLIENT_DIR)/client $(CLIENT_DIR)/source_client.c common.c

clean :
	-rm -f $(SERVER_DIR)/server
	-rm -f $(CLIENT_DIR)/client
