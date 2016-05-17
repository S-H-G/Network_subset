BIN = server client
OBJECT_S = server.o http_parser.o
OBJECT_C = client.o

all : $(BIN)

client : $(OBJECT_C)
	gcc -o $@ $(OBJECT_C)

client.o : client.c
	gcc -c -o $@ $<

server : $(OBJECT_S)
	gcc -o $@ $(OBJECT_S)

server.o : server.c
	gcc -c -o $@ $<

http_parser.o : http_parser.c
	gcc -c -o $@ $<

clean :
	rm -rf *.o
