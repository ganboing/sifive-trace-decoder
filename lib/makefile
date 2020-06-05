-include ../makefile.init

RM := rm -rf

# All Target
all: libs
Debug: libs
Release: libs

libs: staticlibs piclibs

# Tool invocations

piclibs: $(PICLIBS)

staticlibs: $(STATICLIBS)

picliberty:
	mkdir -p $(PLATFORM)/picliberty
	$(eval DSTDIR = $(shell pwd)/$(PLATFORM)/picliberty)
	cd $(BINUTILSPATH)/libiberty/pic; cp $(LIBLIBERTYOBJS) $(DSTDIR)
	cd $(PLATFORM)/picliberty; $(CROSSPREFIX)ar rcs libiberty.a $(LIBLIBERTYOBJS)

picopcodes:
	mkdir -p $(PLATFORM)/picopcodes
	$(eval DSTDIR = $(shell pwd)/$(PLATFORM)/picopcodes)
	cd $(BINUTILSPATH)/opcodes/.libs; cp $(LIBOPCODESOBJS) $(DSTDIR)
	cd $(PLATFORM)/picopcodes; $(CROSSPREFIX)ar rcs libopcodes.a $(LIBOPCODESOBJS)

picbfd:
	mkdir -p $(PLATFORM)/picbfd
	$(eval DSTDIR = $(shell pwd)/$(PLATFORM)/picbfd)
	cd $(BINUTILSPATH)/bfd/.libs; cp $(LIBBFDOBJS) $(DSTDIR)
	cd $(PLATFORM)/picbfd; $(CROSSPREFIX)ar rcs libbfd.a $(LIBBFDOBJS)

bfd:
	cp $(BINUTILSPATH)/bfd/libbfd.a $(PLATFORM)
	cp $(BINUTILSPATH)/bfd/*.h $(PLATFORM)
	cp $(BINUTILSPATH)/../riscv-binutils/include/*.h $(PLATFORM)

liberty:
	cp $(BINUTILSPATH)/libiberty/libiberty.a $(PLATFORM)

intl:
	cp $(BINUTILSPATH)/intl/libintl.a $(PLATFORM)

opcodes:
	cp $(BINUTILSPATH)/opcodes/libopcodes.a $(PLATFORM)

zlib:
	cp $(BINUTILSPATH)/zlib/libz.a $(PLATFORM)

install:

clean:

# Other Targets
.PHONY: all libs clean dependents install
