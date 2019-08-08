TOPTARGETS := all clean

SUBDIRS := Debug Release

CONFIG := Debug

CROSSPREFIX :=

$(TOPTARGETS): $(CONFIG)

$(CONFIG):
	$(MAKE) -C $@ $(MAKECMDGOALS)

Release:
	$(MAKE) -C Release $(MAKECMDGOALS)

clean-all:
	$(MAKE) -C Debug clean
	$(MAKE) -C Release clean

.PHONY: $(TOPTARGETS) $(CONFIG) clean-all Release
