ROOT =	.

include $(ROOT)/make.config

all:
	$(MAKE) -C commonLibrary all
	$(MAKE) -C clientServiceLibrary all
	$(MAKE) -C manager all
	$(MAKE) -C commands all
	$(MAKE) -C services all
	$(MAKE) -C testPrograms all
	$(MAKE) -C wrappers/perl all
	$(MAKE) -C wrappers/java all
	$(MAKE) -C wrappers/php all

version:
	@echo '#ifndef _VERSION_H'			>include/version.h
	@echo '#define _VERSION_H'			>>include/version.h
	@echo ''					>>include/version.h
	@echo 'namespace version {'			>>include/version.h
	echo  'static const char* rcsid = "build:' `hostname`:`pwd`:`date` '";'	>>include/version.h
	@echo '}'					>>include/version.h
	@echo ''					>>include/version.h
	@echo '#endif'					>>include/version.h

clean:
	$(MAKE) -C commonLibrary clean
	$(MAKE) -C clientServiceLibrary clean
	$(MAKE) -C manager clean
	$(MAKE) -C commands clean
	$(MAKE) -C services clean
	$(MAKE) -C testPrograms clean
	$(MAKE) -C wrappers/perl clean
	$(MAKE) -C wrappers/java clean
	$(MAKE) -C wrappers/php clean

regression-tests:
	$(MAKE) -C regressionTests all

install:
	sh install.sh $(INSTALLCMD) $(INSTALLDIR) $(RUNDIR) $(RUNUSER) $(RUNGROUP)
