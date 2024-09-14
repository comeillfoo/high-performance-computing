CC=gcc
LD=gcc

INCDIR=include
SRCDIR=src
BUILDDIR=build
TARGET=app

CFLAGS=-O3 -Wall -Werror -pedantic -D CL_TARGET_OPENCL_VERSION=300 -I$(INCDIR)
LIBS=-lm -lOpenCL

all: $(BUILDDIR)/main.o $(BUILDDIR)/mappers.o $(BUILDDIR)/sorts.o
	$(LD) $(LDFLAGS) $^ -o $(TARGET) $(LIBS)

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR)
	rm -f $(TARGET)

.PHONY: clean
