IDIR =../include
CC=g++
CFLAGS= -I$(IDIR) -std=c++11 -g

ODIR=obj
LDIR =../lib

LIBS=-lm

_DEPS = cacheSim.h CacheSystem.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = cacheSim.o CacheSystem.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cacheSim: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
