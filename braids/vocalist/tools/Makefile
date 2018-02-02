OBJS = reciter.o sam.o render.o debug.o processframes.o createtransitions.o rendertabs.o
PARSER_OBJS = $(OBJS) parser.o

CC = g++

# libsdl present
CFLAGS =  -Wall -DUSESDL -g

all: parser src/wordlist.cc

src/wordlist.cc: parser src/generator.rb
	ruby ./src/generator.rb

parser: $(PARSER_OBJS)
	$(CC) -o parser $(PARSER_OBJS)

%.o: src/%.cc
	$(CC) $(CFLAGS) -Isrc -c $<

clean:
	rm *.o ./src/wordlist.cc
