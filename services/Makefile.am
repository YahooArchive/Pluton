AM_CPPFLAGS = -I$(top_srcdir)/include @WARN_CXXFLAGS@ -I$(top_srcdir)/clientServiceLibrary
libexec_PROGRAMS = curl_service echo_service
noinst_PROGRAMS = cpuLoop exit fopenLoop loadSimulator mallocLoop
LDADD = $(top_builddir)/clientServiceLibrary/libpluton.la $(top_builddir)/commonLibrary/libcommon.a

curl_service_SOURCES = curl.cc
curl_service_LDADD = @LIBCURL_LIBS@ $(LDADD)
curl_service_CXXFLAGS = @LIBCURL_CFLAGS@ $(AM_CPPFLAGS)

echo_service_SOURCES = echo.cc

# These services are just used for testing purposes

cpuLoop_SOURCES = cpuLoop.cc

exit_SOURCES = exit.cc

fopenLoop_SOURCES = fopenLoop.cc

loadSimulator_SOURCES = loadSimulator.cc

mallocLoop_SOURCES = mallocLoop.cc
