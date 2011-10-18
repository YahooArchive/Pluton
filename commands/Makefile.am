AM_CXXFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/clientServiceLibrary @WARN_CXXFLAGS@
LDADD = $(top_builddir)/clientServiceLibrary/libpluton.la $(top_builddir)/commonLibrary/libcommon.a
bin_PROGRAMS = plPing plLookup plSend plBatch plVersion plNetStringGrep plTest
dist_bin_SCRIPTS = plReloadManager

plPing_SOURCES = plPing.cc

plLookup_SOURCES = plLookup.cc

plSend_SOURCES = plSend.cc

plBatch_SOURCES = plBatch.cc

plVersion_SOURCES = plVersion.cc

plNetStringGrep_SOURCES = plNetStringGrep.cc

plTest_SOURCES = plTest.cc
