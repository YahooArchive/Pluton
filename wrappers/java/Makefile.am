AM_CFLAGS = -I$(top_srcdir)/include @WARN_CFLAGS@
LDADD = $(top_builddir)/clientServiceLibrary/libpluton.la $(top_builddir)/commonLibrary/libcommon.a

lib_LTLIBRARIES = libplutonjava.la

libplutonjava_la_SOURCES = pluton_java.c


libplutonjava_la_LDFLAGS = -version-info 0:0:0

YahooPluton.java YahooPlutonJNI.java pluton_java.c: ../pluton.i
	@SWIGPROG@ -java -o pluton_java.c -outdir . ../pluton.i

clean-local:
	-rm -f YahooPluton.java	YahooPlutonJNI.java pluton_java.c
