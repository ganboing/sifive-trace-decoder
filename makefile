
ifeq ($(INSTALLPATH),)
        INSTALLABSPATH := $(realpath ./.)/install
else
        $(shell (mkdir -p "$(INSTALLPATH)"))
        INSTALLABSPATH := $(realpath $(INSTALLPATH))
endif

TOPTARGETS := all clean install

SUBDIRS := Debug Release

CONFIG := Debug

CROSSPREFIX :=

include version.mk

$(TOPTARGETS): $(CONFIG)

$(CONFIG):
	$(MAKE) -C $@ $(MAKECMDGOALS) INSTALLPATH="$(INSTALLABSPATH)"

Release:
	$(MAKE) -C Release $(MAKECMDGOALS) INSTALLPATH="$(INSTALLABSPATH)"

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
