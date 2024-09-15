CC=gcc
LD=gcc
OCLWC=tools/oclwc

INCDIR=include
SRCDIR=src
BUILDDIR=build
TARGETS=main mappers sorts oclw futils
TARGET=app

KERNELS=kernel.clbin
TOOLS=$(OCLWC)

CFLAGS=-O3 -Wall -Werror -pedantic -D CL_TARGET_OPENCL_VERSION=300 -I$(INCDIR)
LIBS=-lm -lOpenCL


all: $(TARGET) $(TOOLS) $(KERNELS)

$(TARGET): $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(TARGETS)))
	$(LD) $(LDFLAGS) $^ -o $(TARGET) $(LIBS)

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/tools:
	mkdir -p $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/tools/%.o: $(SRCDIR)/tools/%.c $(BUILDDIR)/tools
	$(CC) -c $(CFLAGS) $< -o $@

$(OCLWC): $(BUILDDIR)/tools/oclwc.o $(BUILDDIR)/oclw.o $(BUILDDIR)/futils.o
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

$(KERNELS): $(addsuffix .cl, $(addprefix $(SRCDIR)/kernels/, main))
	$(OCLWC) $@ $^


clean:
	rm -rf $(BUILDDIR)
	rm -f $(OCLWC)
	rm -f $(KERNELS)
	rm -f $(TARGET)

.PHONY: clean
