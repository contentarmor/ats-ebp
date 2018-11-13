SHELL = /bin/sh

SUBDIRS = tslib libstructures logging

.PHONY: clean subdirs $(SUBDIRS)

all: subdirs
subdirs: $(SUBDIRS)

logging:
	$(MAKE) -C $@

libstructures: 
	$(MAKE) -C $@

tslib: libstructures h264bitstream logging
	$(MAKE) -C $@

clean: 
	for dir in $(SUBDIRS); do \
		echo "Cleaning $$dir..."; \
		$(MAKE) -C $$dir clean; \
	done
