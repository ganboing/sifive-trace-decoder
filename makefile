
ifeq ($(INSTALLPATH),)
	INSTALLABSPATH := $(realpath ./install)
else
	INSTALLABSPATH := $(realpath $(INSTALLPATH))
        $(info installpath: $(INSTALLPATH))
        $(info realpath: $(realpath $(INSTALLPATH)))
endif

TOPTARGETS := all clean install

SUBDIRS := Debug Release lib

CONFIG := Debug

CROSSPREFIX :=

$(TOPTARGETS): $(CONFIG)

$(CONFIG): libs
	$(MAKE) -C $@ $(MAKECMDGOALS) INSTALLPATH="$(INSTALLABSPATH)"

Release: libs
	$(MAKE) -C Release $(MAKECMDGOALS) INSTALLPATH="$(INSTALLABSPATH)"

libs:
	$(MAKE) -C lib NEWLIBPATH=$(NEWLIBPATH) $(MAKECMDGOALS) INSTALLPATH="$(INSTALLABSPATH)"

install: install-include install-examples install-scripts

install-include:
	mkdir -p "$(INSTALLABSPATH)/include"
	cp -rp include/dqr.hpp "$(INSTALLABSPATH)/include"

install-examples:
	mkdir -p "$(INSTALLABSPATH)"
	cp -rp examples "$(INSTALLABSPATH)"

install-scripts:
	mkdir -p "$(INSTALLABSPATH)"
	cp -rp scripts "$(INSTALLABSPATH)"

clean clean-all:
	$(MAKE) -C Debug clean
	$(MAKE) -C Release clean

.PHONY: $(TOPTARGETS) $(CONFIG) clean-all Release install clean
