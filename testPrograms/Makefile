ROOT = ..

include $(ROOT)/make.config

ifeq ($(shell uname),Linux)
# JSRC=$(wildcard ./*.java)
# CLASSES=$(patsubst %.java,%.class,$(JSRC))
endif

%.class: %.java
	javac -cp ../wrappers/java/jar/pluton.jar $<

CXXSRCS = bug1552859.cc	\
	sampleClient.cc	sampleProxyClient.cc sampleService.cc 	\
	shmAlign.cc shmInvalidate.cc				\
	tClientAffinity.cc tClientErrors.cc 			\
	tClientEvent2.cc tClientExecute1.cc tClientExecute2.cc	\
	tClientFault1.cc tClientGlobals1.cc tClientGlobals2.cc tClientMultiPerCaller.cc	\
	tClientPollProxy.cc tClientPollProxyBad.cc tClientReset.cc			\
	tClientResourceLeaks.cc tClientStateThreads.cc tClientpthreads.cc		\
	tInProgress.cc tLineToArgv.cc tNetStringFactory.cc tNetStringFactoryManaged.cc	\
	tNetStringGenerate.cc tNetStringGenerateEncapsulated.cc				\
	tPartialIO.cc tRateLimit.cc tRetry.cc						\
	tService1.cc tServiceExit.cc tShmService.cc					\
	tThread1.cc tThread2.cc tThread3.cc tThread4.cc tThread5.cc			\
	tc.cc ts.cc

CSRCS = tCclient1.c tCclient2.c tCwrapper1.c

OBJS = $(patsubst %.cc,%.o,$(CXXSRCS))
OBJS += $(patsubst %.c,%.o,$(CSRCS))

ALL = $(patsubst %.o,%,$(OBJS))

ifeq ($(shell uname),Linux)
ALL += tThread1 tClientpthreads
endif

all:	$(ALL)
	echo Making $(ALL)

sampleProxyClient:	sampleProxyClient.cc
	$(CXX) $(CXXFLAGS) $(STINC) sampleProxyClient.cc $(STLIB) $(LDLIBS) -o sampleProxyClient

tClientPollProxy:	tClientPollProxy.cc
	$(CXX) $(CXXFLAGS) $(STINC) tClientPollProxy.cc $(STLIB) $(LDLIBS) -o tClientPollProxy

tClientPollProxyBad:	tClientPollProxyBad.cc
	$(CXX) $(CXXFLAGS) $(STINC) tClientPollProxyBad.cc $(STLIB) $(LDLIBS) -o tClientPollProxyBad

tClientStateThreads:	tClientStateThreads.cc
	$(CXX) $(CXXFLAGS) $(STINC) tClientStateThreads.cc $(STLIB) $(LDLIBS) -o tClientStateThreads

tClientpthreads:	tClientpthreads.cc
	$(CXX) $(CXXFLAGS) -pthread tClientpthreads.cc $(LDLIBS) -o tClientpthreads

tLineToArgv:	tLineToArgv.cc
	$(CXX) $(CXXFLAGS) tLineToArgv.cc $(COMMONLIB) $(LDLIBS) -o tLineToArgv

tRateLimit:	tRateLimit.cc
	$(CXX) $(CXXFLAGS) tRateLimit.cc $(COMMONLIB) $(LDLIBS) -o tRateLimit

tShmService:	tShmService.cc ../manager/shmServiceWriter.o
	$(CXX) $(CXXFLAGS) tShmService.cc ../manager/shmServiceWriter.o $(COMMONLIB) $(LDLIBS) -o tShmService

tThread1:	tThread1.cc
	$(CXX) $(CXXFLAGS) -pthread tThread1.cc $(LDLIBS) -o tThread1

tThread2:	tThread2.cc
	$(CXX) $(CXXFLAGS) $(STINC) tThread2.cc $(STLIB) $(LDLIBS) -o tThread2

tThread4:	tThread4.cc
	$(CXX) $(CXXFLAGS) $(STINC) tThread4.cc $(STLIB) $(LDLIBS) -o tThread4

tThread5:	tThread5.cc
	$(CXX) $(CXXFLAGS) $(STINC) tThread5.cc $(STLIB) $(LDLIBS) -o tThread5


clean:
	rm -f $(OBJS) $(ALL)
