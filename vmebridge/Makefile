# include the build environment
include ../common.mk

DIRS = \
	driver \
	lib \
	tools \
	test \
	drvrtest

.PHONY: all clean cleanall install $(DIRS)

all clean cleanall install: $(DIRS)

clean: TARGET = clean
cleanall: TARGET = cleanall
install: TARGET = install

$(DIRS):
	$(MAKE) -C $@ $(TARGET)

drvrtest: driver

test: lib

tools: lib
