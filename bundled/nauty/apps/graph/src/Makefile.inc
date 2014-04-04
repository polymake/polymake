ifeq (${ImportedIntoExtension},)
  StaticLibs := nauty

  GraphIso$O : ExtraCXXFLAGS = -DBIGNAUTY -I${BuildDir}/staticlib/nauty

  LIBS += ${BuildDir}/staticlib/nauty/libnauty$A
endif

# Local Variables:
# mode: Makefile
# End:
