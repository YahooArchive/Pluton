if BUILDPHPWRAPPER
   PHPWRAPPER = wrappers/php
endif
if BUILDPERLWRAPPER
   PERLWRAPPER = wrappers/perl
endif
if BUILDJAVAWRAPPER
   JAVAWRAPPER = wrappers/java
endif


SUBDIRS = commonLibrary clientServiceLibrary manager commands services testPrograms	\
	$(PHPWRAPPER) $(PERLWRAPPER) $(JAVAWRAPPER)

ACLOCAL_AMFLAGS = -I m4

include_HEADERS = include/pluton/client.h include/pluton/clientEvent.h \
		include/pluton/clientRequest.h include/pluton/client_C.h \
		include/pluton/common.h include/pluton/fault.h include/pluton/service.h \
		include/pluton/service_C.h
