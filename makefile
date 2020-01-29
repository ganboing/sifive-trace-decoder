TOPTARGETS := all clean install

SUBDIRS := Debug Release lib

CONFIG := Debug

CROSSPREFIX :=

$(TOPTARGETS): $(CONFIG)

$(CONFIG): libs
	$(MAKE) -C $@ $(MAKECMDGOALS)

Release: libs
	$(MAKE) -C Release $(MAKECMDGOALS)

libs:
	$(MAKE) -C lib NEWLIBPATH=$(NEWLIBPATH) $(MAKECMDGOALS)

clean-all:
	$(MAKE) -C Debug clean
	$(MAKE) -C Release clean

.PHONY: $(TOPTARGETS) $(CONFIG) clean-all Release
