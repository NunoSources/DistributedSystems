# Alexandre Torrinha fc52757
# Nuno Fontes fc46413
# Rodrigo Simões fc52758

CLIENT = entry.o data.o network_client.o tree_client.o client_stub.o message.o sdmessage.pb-c.o
SERVER = tree.o entry.o data.o network_server.o tree_server.o tree_skel.o message.o sdmessage.pb-c.o network_client.o client_stub.o
INC_DIR = include
SRC_DIR = source
OBJ_DIR = object
BIN_DIR = binary
LIB_DIR = lib
CC = gcc
CFLAGS = -I/usr/local/include -lprotobuf-c -lpthread -DTHREADED -lzookeeper_mt
out: protobuf_b client server

client: $(CLIENT)
			$(CC) $(addprefix $(OBJ_DIR)/, $(CLIENT)) -o $(BIN_DIR)/$@ $(CFLAGS)

server: $(SERVER)
			$(CC) $(addprefix $(OBJ_DIR)/, $(SERVER)) -o $(BIN_DIR)/$@ $(CFLAGS)

%.o: $(SRC_DIR)/%.c $(addprefix $(INCLUDE_DIR)/, $($@))
			$(CC) -Wall -g -o $(OBJ_DIR)/$(@) -I include -c $< -I .

sdmessage.pb-c.o:
			$(CC) -c $(SRC_DIR)/sdmessage.pb-c.c -o $(OBJ_DIR)/sdmessage.pb-c.o -I include

protobuf_b:
			protoc --c_out=./source sdmessage.proto
			mv source/sdmessage.pb-c.h include/
			$(CC) -c $(SRC_DIR)/sdmessage.pb-c.c -o $(OBJ_DIR)/sdmessage.pb-c.o -I include
