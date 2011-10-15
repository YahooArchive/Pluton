AC_PREREQ([2.68])
AC_INIT([pluton], [0.0.1], [y6x@romeo.emu.st],,[http://markdelany.github.com/Pluton])
AC_PREFIX_DEFAULT(/usr/local/pluton)
AC_CONFIG_SRCDIR(manager/plutonManager.cc)
AC_CONFIG_HEADER(config.h)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

WARN_CXXFLAGS="-Wall -pedantic -Wno-long-long"
AC_SUBST(WARN_CXXFLAGS)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Include large-file support on Linux

AC_SYS_LARGEFILE

# Check for packages

PKG_CHECK_MODULES(STATETHREADS, st >= 1.8)
PKG_CHECK_MODULES(LIBCURL, libcurl >= 7.19.6)

# Checks for header files.

AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h locale.h \
	          netdb.h netinet/in.h stdint.h stdlib.h string.h strings.h \
		  sys/param.h sys/socket.h sys/time.h unistd.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([alarm bzero dup2 gethostbyname gettimeofday memmove memset munmap \
		realpath select socket strcasecmp strchr strdup strerror strncasecmp strtol])

AC_OUTPUT(Makefile replace/Makefile
          commonLibrary/Makefile clientServiceLibrary/Makefile
 	  manager/Makefile commands/Makefile services/Makefile
 	  testPrograms/Makefile
 	  wrappers/perl/Makefile wrappers/java/Makefile wrappers/php/Makefile)
