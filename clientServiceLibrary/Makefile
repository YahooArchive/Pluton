ROOT =	..

include $(ROOT)/make.config

CXXFLAGS += -fPIC

LIB_NAME = libpluton
LIB_VERSION = 0.1

SOLIB=$(LIB_NAME).so.$(LIB_VERSION)

SRCS =	\
	client.cc clientEvent.cc clientEventImpl.cc clientImpl.cc	\
	clientRequest.cc clientRequestImpl.cc client_C.cc		\
	decodePacket.cc							\
	fault.cc faultImpl.cc						\
	perCallerClient.cc						\
	requestImpl.cc requestQueue.cc					\
	service.cc serviceImpl.cc service_C.cc shmLookupReader.cc	\
	timeoutClock.cc

OBJS	= $(patsubst %.cc,%.o,$(SRCS))

ALL =	$(SOLIB)

all:	$(ALL)

$(OBJS): depend Makefile

$(SOLIB): $(OBJS) $(COMMONLIB)
ifdef OSX
	$(CXX) -dynamiclib					\
	-install_name /usr/local/lib/$(LIB_NAME).$(LIB_VERSION).dylib	\
	-current_version $(LIB_VERSION)				\
	 -compatibility_version 1				\
	-o $(SOLIB) $(OBJS) $(COMMONLIB)
	ln -fs $(SOLIB) $(LIB_NAME).dylib
else
	$(CXX) -fPIC -g -o $(SOLIB) $(OBJS) $(COMMONLIB) -shared -Wl,-soname,$(SOLIB)
	ln -fs $(SOLIB) $(LIB_NAME).so
endif

$(COMMONLIB):
	cd $(ROOT)/commonLibrary && make

faultEnglish.cc.txt:	$(INC)/pluton/fault.h generate_FaultEnglish.pl
	perl ./generate_FaultEnglish.pl <$(INC)/pluton/fault.h >faultEnglish.cc.txt

faultImpl.o:	faultEnglish.cc.txt

clean:
	rm -f $(ALL) $(OBJS) faultEnglish.cc.txt
	rm -f *.core
	rm -f *.so *.so.* *.dylib *.dylib.*
	rm -f make.depend

depend: make.depend

make.depend:	Makefile
	rm -f make.depend
	touch make.depend
	$(MAKEDEPEND) -f make.depend -- $(CXXFLAGS) -- $(SRCS)

include make.depend
