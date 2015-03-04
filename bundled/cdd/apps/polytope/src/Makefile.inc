ifeq (${ImportedIntoExtension},)
  ifeq (${BundledCdd},yes)
    StaticLibs := cdd
  endif
  LIBS += $(subst %A,${A},$(CddLib))

  cdd_interface$O : ExtraCXXFLAGS = $(CddCflags)
  cdd_float_interface$O : ExtraCXXFLAGS = $(CddCflags)
endif


# Local Variables:
# mode: Makefile
# End:
