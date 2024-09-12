CC=gcc
LD=gcc
CFLAGS=-O3 -Wall -Werror -pedantic
LIBS=-lm

INCDIR=include
SRCDIR=src
BUILDDIR=build
EXECUTABLE=app

all: $(BUILDDIR)/main.o
	$(LD) $(LDFLAGS) $^ -o $(EXECUTABLE) $(LIBS)

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR)
	rm -f $(EXECUTABLE)

.PHONY: clean
