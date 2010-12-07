ROOT =  ..

include $(ROOT)/make.config

CXXFLAGS += $(STINC)

SRCS =	\
	plutonManager.cc							\
	bitmask.cc								\
	calibrateProcesses.cc commandPort.cc configParser.cc			\
	debug.cc								\
	listenBacklog.cc listenInterface.cc loadConfigurations.cc logging.cc	\
	manager.cc								\
	periodicReports.cc pidMap.cc process.cc					\
	service.cc shmLookupWriter.cc shmServiceWriter.cc			\
	startProcess.cc startService.cc						\
	threadedObject.cc

OBJS=$(patsubst %.cc,%.o,$(SRCS))

ALL =	plutonManager

all:	$(ALL)

plutonManager:	$(OBJS) $(COMMONLIB)
	$(CXX) -o plutonManager $(OBJS) $(COMMONLIB) $(STLIB)

$(COMMONLIB):
	cd $(ROOT)/commonLibrary && make

clean:
	rm -f $(OBJS) *.core $(ALL)
	rm -f make.depend

depend: make.depend

make.depend:	Makefile
	rm -f make.depend
	touch make.depend
	$(MAKEDEPEND) -f make.depend -- $(CXXFLAGS) -- $(SRCS)

include make.depend
