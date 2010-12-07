ROOT = ..

include $(ROOT)/make.config


SRCS =	plPing.cc plLookup.cc plSend.cc plBatch.cc plVersion.cc plNetStringGrep.cc plTest.cc

ALL =	plPing plLookup plSend plBatch plVersion plNetStringGrep plTest

# Obsolete but not forgotten
# ALL += plTransmitter plReceiver

all:	$(ALL)

plPing:	plPing.o $(COMMONLIB)
	$(CXX) -o plPing plPing.o $(COMMONLIB) $(LDLIBS)

plLookup:	plLookup.o $(COMMONLIB)
	$(CXX) -o plLookup plLookup.o $(COMMONLIB) $(LDLIBS)

plSend:	plSend.o $(COMMONLIB)
	$(CXX) -o plSend plSend.o $(COMMONLIB) $(LDLIBS)

plBatch:	plBatch.o $(COMMONLIB)
	$(CXX) -o plBatch plBatch.o $(COMMONLIB) $(LDLIBS)

plVersion:	plVersion.o $(COMMONLIB)
	$(CXX) -o plVersion plVersion.o $(COMMONLIB) $(LDLIBS)

plNetStringGrep:	plNetStringGrep.o $(COMMONLIB)
	$(CXX) -o plNetStringGrep plNetStringGrep.o $(COMMONLIB)

plTest:	plTest.o $(COMMONLIB)
	$(CXX) -o plTest plTest.o $(COMMONLIB) $(LDLIBS)


$(COMMONLIB):
	cd $(ROOT)/commonLibrary && make

clean:
	rm -f *.core $(ALL) *.o
	rm -f make.depend

make.depend:	Makefile
	rm -f make.depend
	touch make.depend
	$(MAKEDEPEND) -f make.depend -- $(CXXFLAGS) -- $(SRCS)

include make.depend
