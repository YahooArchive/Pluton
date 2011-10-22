AM_CFLAGS = -I$(top_srcdir)/include @WARN_CFLAGS@	\
		`@PERLPROG@ -M'ExtUtils::Embed' -e ccopts --`

LDADD = $(top_builddir)/clientServiceLibrary/libpluton.la $(top_builddir)/commonLibrary/libcommon.a \
		`@PERLPROG@ -M'ExtUtils::Embed' -e ldopts --`

lib_LTLIBRARIES = libplutonperl.la

libplutonperl_la_SOURCES = pluton_perl.c

libplutonperl_la_LDFLAGS = -version-info 0:0:0

YahooPluton.pm pluton_perl.c: ../pluton.i
	@SWIGPROG@ -perl -o pluton_perl.c -outdir . ../pluton.i

clean-local:
	-rm -f YahooPluton.pm pluton_perl.c
