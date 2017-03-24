ifeq (${ImportedIntoExtension},)
  StaticLibs := nauty

  ifeq (${NautySrc},)
    NautySrc := ${ExtensionTop}/external/nauty
  endif

  GraphIso$O : ExtraCXXFLAGS = -DBIGNAUTY -I${BuildDir}/staticlib/nauty -I${NautySrc}

  LIBS += ${BuildDir}/staticlib/nauty/libnauty$A
endif

# Local Variables:
# mode: Makefile
# End:
