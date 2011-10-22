ROOT=../../..

include $(ROOT)/make.config

CSRCS = client.c service.c
CXXSRCS = initThreads.cc

CFLAGS += -fPIC
CXXFLAGS += -fPIC
OBJS = client.o service.o initThreads.o

LIB_NAME = plutonjni
LIB_VERSION = 0.1
SOLIB=$(LIB_NAME).so.$(LIB_VERSION)

LINC = -I$(ROOT)/include -I$(JDK)/include -I$(JDK)/include/linux

LINK.C  = $(LINK.c)

all:	$(SOLIB)

com_yahoo_pluton_Client.h: ../com/yahoo/pluton/Client.class
	javah -force -verbose -jni -classpath .. com.yahoo.pluton.Client

com_yahoo_pluton_ClientRequest.h: ../com/yahoo/pluton/ClientRequest.class
	javah -force -verbose -jni -classpath .. com.yahoo.pluton.ClientRequest

com_yahoo_pluton_Service.h: ../com/yahoo/pluton/Service.class
	javah -force -verbose -jni -classpath .. com.yahoo.pluton.Service

$(SOLIB): $(OBJS)
	$(LD) -shared -soname $(SOLIB) -o $@ $(LDLIBS) $^

faultExceptions.c.txt:    $(ROOT)/include/pluton/fault.h generate_FaultExceptions.pl
	perl generate_FaultExceptions.pl <$(ROOT)/include/pluton/fault.h >faultExceptions.c.txt

client.o:	faultExceptions.c.txt com_yahoo_pluton_Client.h com_yahoo_pluton_ClientRequest.h

service.o:	faultExceptions.c.txt com_yahoo_pluton_Service.h

clean::
	rm -f $(OBJ) $(SOLIB) faultExceptions.c.txt
	rm -f com_yahoo_pluton_Client.h com_yahoo_pluton_Service.h
