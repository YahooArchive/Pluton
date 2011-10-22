AM_CFLAGS = -I$(top_srcdir)/include @WARN_CFLAGS@	\
	-I`@PHPCONFIG@ --include-dir`			\
	-I`@PHPCONFIG@ --include-dir`/Zend		\
	-I`@PHPCONFIG@ --include-dir`/main		\
	-I`@PHPCONFIG@ --include-dir`/TSRM

lib_LTLIBRARIES = libplutonphp.la

libplutonphp_la_SOURCES = pluton_php.c

LDADD = $(top_builddir)/clientServiceLibrary/libpluton.la $(top_builddir)/commonLibrary/libcommon.a

libplutonphp_la_LDFLAGS = -version-info 0:0:0

YahooPluton.php php_YahooPluton.h pluton_php.c: ../pluton.i
	@SWIGPROG@ -php -o pluton_php.c -outdir . ../pluton.i

clean-local:
	-rm -f YahooPluton.php php_YahooPluton.h pluton_php.c
