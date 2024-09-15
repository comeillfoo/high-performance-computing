CC=gcc
LD=gcc
OCLC?=/opt/intel/oneapi/compiler/latest/bin/opencl-aot

INCDIR=include
SRCDIR=src
BUILDDIR=build
TARGETS=main mappers sorts oclw
TARGET=app

KERNELSDIR=kernels
KERNELS=main

CFLAGS=-O3 -Wall -Werror -pedantic -D CL_TARGET_OPENCL_VERSION=300 -I$(INCDIR)
LIBS=-lm -lOpenCL

join-with = $(subst $(space),$1,$(strip $2))

all: $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(TARGETS)))
	$(LD) $(LDFLAGS) $^ -o $(TARGET) $(LIBS)

$(BUILDDIR):
	mkdir -p $@

$(BUILDDIR)/$(KERNELSDIR):
	mkdir -p $@

$(KERNELSDIR):
	mkdir -p $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) -c $(CFLAGS) $< -o $@


$(BUILDDIR)/$(KERNELSDIR)/%.obj: $(SRCDIR)/$(KERNELSDIR)/%.cl $(BUILDDIR)/$(KERNELSDIR)
	$(OCLC) -cmd=compile -device=cpu -bo='-cl-std=CL3.0' --input=$< --ir=$@

$(KERNELSDIR)/main.elf: $(addsuffix .obj, $(addprefix $(BUILDDIR)/$(KERNELSDIR)/, main))
	$(OCLC) -cmd=link -device=cpu -binary=$(call join-with,,,$@)


clean:
	rm -rf $(KERNELSDIR)
	rm -rf $(BUILDDIR)
	rm -f $(TARGET)

.PHONY: clean
