OBJS = reciter.o sam.o render.o debug.o processframes.o createtransitions.o rendertabs.o
PARSER_OBJS = $(OBJS) parser.o

CC = g++

CFLAGS =  -Wall -g

all: wordlist.cc

wordlist.cc: parser
	ruby ./tools/words.rb

parser: $(PARSER_OBJS)
	$(CC) -o parser $(PARSER_OBJS)

%.o: sam/%.cc
	$(CC) $(CFLAGS) -c $<

%.o: tools/%.cc
	$(CC) $(CFLAGS) -c $<

%.o: %.cc
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o ./wordlist.cc
