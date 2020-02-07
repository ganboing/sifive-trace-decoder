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
	cd $(NEWLIBPATH)/libiberty/pic; cp $(LIBLIBERTYOBJS) $(DSTDIR)
	cd $(PLATFORM)/picliberty; $(CROSSPREFIX)ar rcs libiberty.a $(LIBLIBERTYOBJS)

picopcodes:
	mkdir -p $(PLATFORM)/picopcodes
	$(eval DSTDIR = $(shell pwd)/$(PLATFORM)/picopcodes)
	cd $(NEWLIBPATH)/opcodes/.libs; cp $(LIBOPCODESOBJS) $(DSTDIR)
	cd $(PLATFORM)/picopcodes; $(CROSSPREFIX)ar rcs libopcodes.a $(LIBOPCODESOBJS)

picbfd:
	mkdir -p $(PLATFORM)/picbfd
	$(eval DSTDIR = $(shell pwd)/$(PLATFORM)/picbfd)
	cd $(NEWLIBPATH)/bfd/.libs; cp $(LIBBFDOBJS) $(DSTDIR)
	cd $(PLATFORM)/picbfd; $(CROSSPREFIX)ar rcs libbfd.a $(LIBBFDOBJS)

bfd:
	cp $(NEWLIBPATH)/bfd/libbfd.a $(PLATFORM)

liberty:
	cp $(NEWLIBPATH)/libiberty/libiberty.a $(PLATFORM)

intl:
	cp $(NEWLIBPATH)/intl/libintl.a $(PLATFORM)

opcodes:
	cp $(NEWLIBPATH)/opcodes/libopcodes.a $(PLATFORM)

zlib:
	cp $(NEWLIBPATH)/zlib/libz.a $(PLATFORM)

install:

clean:

# Other Targets
.PHONY: all libs clean dependents install
