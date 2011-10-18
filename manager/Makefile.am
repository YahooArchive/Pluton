AM_CPPFLAGS = -I$(top_srcdir)/include @WARN_CXXFLAGS@ @STATETHREADS_CFLAGS@
LDADD = @STATETHREADS_LIBS@ \
	$(top_builddir)/clientServiceLibrary/libpluton.la \
	$(top_builddir)/commonLibrary/libcommon.a
bin_PROGRAMS = plutonManager

plutonManager_SOURCES = plutonManager.cc bitmask.cc calibrateProcesses.cc commandPort.cc \
	 configParser.cc debug.cc listenBacklog.cc listenInterface.cc loadConfigurations.cc \
	 logging.cc manager.cc periodicReports.cc pidMap.cc process.cc service.cc \
	 shmLookupWriter.cc shmServiceWriter.cc startProcess.cc startService.cc threadedObject.cc
