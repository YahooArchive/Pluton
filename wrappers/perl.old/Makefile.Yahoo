# Run wih PERLversion, PERLrevision, LIBNAME and PERLarch

ROOT = ../..

include $(ROOT)/make.config

WARN =

CXXFLAGS = $(WARN) $(LINC) $(LDEF) -fPIC
CXXFLAGS += $(shell perl -MExtUtils::Embed -e ccopts)
LDFLAGS += $(shell ARCHFLAGS='-arch i386' perl -MExtUtils::Embed -e ldopts)

LIB_NAME=libplutonPerl${PERLversion}_${PERLrevision}
LIB_VERSION = 0.1
SOLIB=$(LIB_NAME).so.$(LIB_VERSION)

all:	$(SOLIB) Yahoo_$(PERLversion)_$(PERLrevision)/pluton.pm

Yahoo_$(PERLversion)_$(PERLrevision)/pluton.pm: pluton_wrap.cxx pluton.pm
	mkdir -p Yahoo_$(PERLversion)_$(PERLrevision)
	mv pluton.pm Yahoo_$(PERLversion)_$(PERLrevision)/.

pluton.pm pluton_wrap.cxx:  pluton.i Makefile swigClient.h swigService.h
	swig -Wall -perl -I$(INC) -c++ pluton.i

pluton_wrap.o:	pluton_wrap.cxx
	$(CXX) $(CXXFLAGS)	\
	-I$(PERLarch)/CORE	\
	-c pluton_wrap.cxx -o pluton_wrap.o

$(SOLIB):  pluton_wrap.o swigClient.o swigService.o
ifdef OSX
	$(CXX) -dynamiclib					\
	-install_name $(INSTALLDIR)/lib/$(LIB_NAME).1.dylib	\
	-current_version $(LIB_VERSION)				\
	-compatibility_version 1				\
	$(LDFLAGS)						\
	-o $(SOLIB)						\
	pluton_wrap.o swigClient.o swigService.o		\
	$(COMMONLIB) $(LDLIBS)
else
	$(LD) -shared -soname $(SOLIB) -o $@ $(LDLIBS) $^
	ln -sf $(SOLIB) $(LIB_NAME).so

endif

swigClient.o:	swigClient.cc swigClient.h

swigService.o:	swigService.cc swigService.h

clean:
	-rm -f pluton_wrap.* *.so *.pm *.o $(SOLIB)
	-rm -rf Yahoo_*
