AM_CPPFLAGS = -I$(top_srcdir)/include @WARN_CXXFLAGS@
lib_LTLIBRARIES = libpluton.la

libpluton_la_SOURCES = client.cc clientRequestImpl.cc perCallerClient.cc \
	 service_C.cc clientEvent.cc client_C.cc requestImpl.cc \
	 shmLookupReader.cc clientEventImpl.cc decodePacket.cc \
	 requestQueue.cc timeoutClock.cc clientImpl.cc fault.cc \
	 service.cc clientRequest.cc faultImpl.cc serviceImpl.cc

libpluton_la_LIBADD = $(top_builddir)/commonLibrary/libcommon.a

libpluton_la_LDFLAGS = -version-info 0:0:0

faultEnglish.cc.txt:	$(top_srcdir)/include/pluton/fault.h
	$(SHELL) ./generate_FaultEnglish.sh <$? >$@

faultImpl.lo:	faultEnglish.cc.txt

clean-local:
	-rm -f faultEnglish.cc.txt