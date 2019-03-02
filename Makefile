CC=gcc
LIBS=-lpthread
CFLAGS=-O3

all: agr stats

agr: src/agr.o src/dict.o src/finder.o src/inputword.o
	$(CC) $(CFLAGS) -o bin/agr src/agr.o src/dict.o src/finder.o src/inputword.o $(LIBS)

stats: src/stats.o src/dict.o src/finder.o src/inputword.o
	$(CC) $(CFLAGS) -o bin/stats src/stats.o src/dict.o src/finder.o src/inputword.o $(LIBS)

agr.o: src/agr.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $(OPTS) src/agr.c

stats.o: src/stats.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $(OPTS) src/stats.c

test.o: src/test.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $(OPTS) src/test.c

finder.o: src/finder.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $(OPTS) src/finder.c

dict.o: src/dict.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $(OPTS) src/dict.c

inputword.o: src/inputword.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $(OPTS) src/inputword.c

clean:
	rm -f src/*.o bin/*
