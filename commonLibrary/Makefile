ROOT = 	..

include $(ROOT)/make.config

CXXFLAGS += -fPIC

SRCS =	lineToArgv.cc misc.cc netString.cc rateLimit.cc		\
	serviceKey.cc shmLookupCommon.cc shmServiceCommon.cc	\
	splitInterface.cc util.cc


OBJS	= $(patsubst %.cc,%.o,$(SRCS))

LIB =	common.a

ALL =	$(LIB)

all:	$(ALL)

$(LIB): $(OBJS)
	rm -f $(LIB)
	ar -rv $(LIB) $(OBJS)

$(OBJS): depend Makefile

clean:
	rm -f $(ALL) $(OBJS)
	rm -f *.core
	rm -f make.depend

depend:	make.depend

make.depend:	Makefile
	rm -f make.depend
	touch make.depend
	$(MAKEDEPEND) -f make.depend -- $(CXXFLAGS) -- $(SRCS)

include make.depend
