ROOT = ..

include $(ROOT)/make.config

CXXFLAGS += $(CURLINC)

SRCS =	cpuLoop.cc curl.cc echo.cc exit.cc fopenLoop.cc loadSimulator.cc mallocLoop.cc

TESTSERVICES = cpuLoop exit fopenLoop loadSimulator mallocLoop

PRODSERVICES =	curl echo

ALL =	$(TESTSERVICES) $(PRODSERVICES)

all:	$(ALL)

cpuLoop:	cpuLoop.o $(COMMONLIB)
	$(CXX) -o cpuLoop cpuLoop.o $(COMMONLIB) $(LDLIBS)

curl:	curl.o $(COMMONLIB)
	$(CXX) -o curl curl.o $(COMMONLIB) $(LDLIBS) $(CURLLIB)

echo:	echo.o $(COMMONLIB)
	$(CXX) -o echo echo.o $(COMMONLIB) $(LDLIBS)

exit:	exit.o $(COMMONLIB)
	$(CXX) -o exit exit.o $(COMMONLIB) $(LDLIBS)

fopenLoop:	fopenLoop.o $(COMMONLIB)
	$(CXX) -o fopenLoop fopenLoop.o $(COMMONLIB) $(LDLIBS)

loadSimulator:	loadSimulator.o $(COMMONLIB)
	$(CXX) -o loadSimulator loadSimulator.o $(COMMONLIB) $(LDLIBS)

mallocLoop:	mallocLoop.o $(COMMONLIB)
	$(CXX) -o mallocLoop mallocLoop.o $(COMMONLIB) $(LDLIBS)

$(COMMONLIB):
	cd $(ROOT)/commonLibrary && make

clean:
	rm -f $(ALL) *.o
	rm -f make.depend

depend: make.depend

make.depend:	Makefile
	rm -f make.depend
	touch make.depend
	$(MAKEDEPEND) -f make.depend -- $(CXXFLAGS) -- $(SRCS)

include make.depend
