CC=gcc
LD=gcc
OCLWC=tools/oclwc

INCDIR=include
SRCDIR=src
BUILDDIR=build

JUST_TARGETS=main matrix generators mappers mergers multipliers
OCL_TARGETS=ocl-main mappers sorts oclw futils

KERNELS=kernel.clbin
TOOLS=$(OCLWC)

CFLAGS=-O3 -Wall -Werror -pedantic -D CL_TARGET_OPENCL_VERSION=300 -I$(INCDIR)
LIBS=-lm
OCL_LIBS=$(LIBS) -lOpenCL


all: help

help:
	@echo 'Targets:'
	@echo '- just-main . build default main program with no parallelization'
	@echo '- omp-main  . build main with parallelization using OpenMP'
	@echo '- pt-main   . build main with parallelization using Posix Threads'
	@echo '- ocl-main  . build main with parallelization using OpenCL'

just-main: $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(JUST_TARGETS)))
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

ocl-main: $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(OCL_TARGETS))) $(TOOLS) $(KERNELS)
	$(LD) $(LDFLAGS) $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(OCL_TARGETS))) -o $@ $(OCL_LIBS)

$(OCLWC): $(BUILDDIR)/tools/oclwc.o $(BUILDDIR)/oclw.o $(BUILDDIR)/futils.o
	$(LD) $(LDFLAGS) $^ -o $@ $(OCL_LIBS)

$(KERNELS): $(addsuffix .cl, $(addprefix $(SRCDIR)/kernels/, main))
	$(OCLWC) $@ $^

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/tools:
	mkdir -p $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/tools/%.o: $(SRCDIR)/tools/%.c $(BUILDDIR)/tools
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR)
	rm -f $(OCLWC)
	rm -f $(KERNELS)
	rm -f ocl-main just-main

.PHONY: clean
