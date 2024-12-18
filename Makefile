CC=gcc
LD=gcc

INCDIR=include
SRCDIR=src
BUILDDIR=build

TARGETS=main matrix generators mappers mergers multipliers sorts reducers futils

PT_TARGETS=$(TARGETS) ptpool
OCL_TARGETS=$(TARGETS) oclw

OCLWC_TARGETS=$(OCLWC) oclw futils
OCLWC=tools/oclwc

OCL_KERNELS_TARGETS=ocl-main
OCL_KERNELS=ocl-main.clbin

CFLAGS=-O3 -Wall -Werror -pedantic -I$(INCDIR)
CFLAGS+=$(USERCFLAGS)

LIBS=-lm
OMP_LIBS=$(LIBS) -lgomp
OCL_LIBS=$(LIBS) -lOpenCL


all: help

help:
	@echo 'Targets:'
	@echo '- just-main . build default main program with no parallelization'
	@echo '- omp-main  . build main with parallelization using OpenMP'
	@echo '- pt-main   . build main with parallelization using Posix Threads'
	@echo '- ocl-main  . build main with parallelization using OpenCL'

just-main: $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(TARGETS)))
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

# https://www.gnu.org/software/make/manual/make.html#Target_002dspecific
omp-main: CFLAGS += -fopenmp
omp-main: $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(TARGETS)))
	$(LD) $(LDFLAGS) $^ -o $@ $(OMP_LIBS)

pt-main: CFLAGS += -pthread -DUSE_PTHREAD
pt-main: LDFLAGS += -pthread
pt-main: $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(PT_TARGETS)))
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

ocl-main: CFLAGS += -DUSE_OPENCL -DCL_TARGET_OPENCL_VERSION=300
ocl-main: _ocl-main $(OCLWC) $(OCL_KERNELS)

_ocl-main: $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(OCL_TARGETS)))
	$(LD) $(LDFLAGS) $^ -o ocl-main $(OCL_LIBS)

$(OCLWC): $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(OCLWC_TARGETS)))
	$(LD) $(LDFLAGS) $^ -o $@ $(OCL_LIBS)

$(OCL_KERNELS): $(addsuffix .cl,$(addprefix $(SRCDIR)/, $(OCL_KERNELS_TARGETS)))
	$(OCLWC) $@ $^

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/tools:
	mkdir -p $@

$(BUILDDIR)/tools/%.o: $(SRCDIR)/tools/%.c $(BUILDDIR)/tools
	$(CC) -c $(CFLAGS) $< -o $@

clean-build:
	rm -rf $(BUILDDIR)

clean-just-main:
	rm -f just-main

clean-omp-main:
	rm -f omp-main

clean-pt-main:
	rm -f pt-main

clean-ocl-main:
	rm -f $(OCLWC)
	rm -f $(OCL_KERNELS)
	rm -f ocl-main

clean: clean-build clean-just-main clean-omp-main clean-pt-main clean-ocl-main

.PHONY: clean clean-build clean-just-main clean-omp-main clean-pt-main clean-ocl-main
