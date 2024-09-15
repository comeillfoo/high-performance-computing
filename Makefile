CC=gcc
LD=gcc
OCLOC=/opt/intel/oneapi/compiler/latest/bin/opencl-aot
OCLWC=tools/oclwc

INCDIR=include
SRCDIR=src
BUILDDIR=build
TARGETS=main mappers sorts oclw futils
TARGET=app

KERNELSDIR=$(BUILDDIR)/kernels
KERNEL=kernel.clbin

TOOLSDIR=$(BUILDDIR)/tools
TOOLS=$(OCLWC)

CFLAGS=-O3 -Wall -Werror -pedantic -D CL_TARGET_OPENCL_VERSION=300 -I$(INCDIR)
LIBS=-lm -lOpenCL


all: $(TARGET) $(OCLWC)

$(TARGET): $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(TARGETS)))
	$(LD) $(LDFLAGS) $^ -o $(TARGET) $(LIBS)

$(BUILDDIR):
	mkdir -p $@

$(KERNELSDIR):
	mkdir -p $@

$(TOOLSDIR):
	mkdir -p $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(TOOLSDIR)/%.o: $(SRCDIR)/tools/%.c $(TOOLSDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OCLWC): $(TOOLSDIR)/oclwc.o $(BUILDDIR)/oclw.o $(BUILDDIR)/futils.o
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

# $(KERNELSDIR)/%.clobj: $(SRCDIR)/kernels/%.cl $(KERNELSDIR)
# 	$(OCLOC) --cmd=compile --device=cpu -bo="-cl-std=CL3.0" -o $@ $<

# $(KERNEL): $(addsuffix .clobj, $(addprefix $(KERNELSDIR)/, main))
# 	$(OCLOC) --cmd=build --device=cpu -o $@ $^


clean:
	rm -rf $(KERNELSDIR)
	rm -rf $(BUILDDIR)
	rm -f $(OCLWC)
	rm -f $(KERNEL)
	rm -f $(TARGET)

.PHONY: clean
