CC=gcc
LD=gcc
OCLOC?=/opt/intel/oneapi/compiler/latest/bin/opencl-aot

INCDIR=include
SRCDIR=src
BUILDDIR=build
TARGETS=main mappers sorts oclw
TARGET=app

KERNELSDIR=$(BUILDDIR)/kernels
KERNEL=kernel.elf

CFLAGS=-O3 -Wall -Werror -pedantic -D CL_TARGET_OPENCL_VERSION=300 -I$(INCDIR)
LIBS=-lm -lOpenCL


all: $(TARGET) $(KERNEL)

$(TARGET): $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(TARGETS)))
	$(LD) $(LDFLAGS) $^ -o $(TARGET) $(LIBS)

$(BUILDDIR):
	mkdir -p $@

$(KERNELSDIR):
	mkdir -p $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(KERNELSDIR)/%.obj: $(SRCDIR)/kernels/%.cl $(KERNELSDIR)
	$(OCLOC) --cmd=compile --device=cpu -bo='-cl-std=CL3.0' -o $@ $<

$(KERNEL): $(addsuffix .obj, $(addprefix $(KERNELSDIR)/, main))
	$(OCLOC) --cmd=link --device=cpu -o $@ $^


clean:
	rm -rf $(KERNELSDIR)
	rm -rf $(BUILDDIR)
	rm -f $(TARGET)
	rm -f $(KERNEL)

.PHONY: clean
