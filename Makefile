CC=gcc
LIBDIR=/build/home/dkgroot/usr.local/lib/
INCLDIR=/build/home/dkgroot/usr.local/include/
CFLAGS=-O1 -g -Wall -lpthread -lsqlite3 -I${INCLDIR} -L$(LIBDIR) -I.
TFLAGS=$(CFLAGS) -lrt -lm
DEPS=thpool.h thpool.c
EXE=dports2sqlite
MAIN=$(EXE) $(EXE).c $(DEPS)
DESTDIR?=bin/

all:
	@[ -d $(DESTDIR) ] || mkdir $(DESTDIR)
	$(CC) $(CFLAGS) -o $(DESTDIR)$(MAIN)
run:
	make && ./$(DESTDIR)/$(EXE)
#test:
#	$(CC) $(TFLAGS) -o $(TEST)
#	./$(EXE)_test

clean:
	@[ -d $(DESTDIR) ] && rm -r $(DESTDIR)

