TOPTARGETS := all clean

SUBDIRS := Debug Release

CONFIG := Debug

$(TOPTARGETS): $(CONFIG)

$(CONFIG):
	$(MAKE) -C $@ $(MAKECMDGOALS)

clean-all:
	$(MAKE) -C Debug clean
	$(MAKE) -C Release clean

.PHONY: $(TOPTARGETS) $(CONFIG) clean-all