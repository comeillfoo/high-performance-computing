CC=gcc
LD=gcc

INCDIR=include
SRCDIR=src
BUILDDIR=build
EXECUTABLE=app

CFLAGS=-O3 -Wall -Werror -pedantic -I$(INCDIR)
LIBS=-lm

all: $(BUILDDIR)/main.o $(BUILDDIR)/mappers.o $(BUILDDIR)/sorts.o
	$(LD) $(LDFLAGS) $^ -o $(EXECUTABLE) $(LIBS)

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR)
	rm -f $(EXECUTABLE)

.PHONY: clean
