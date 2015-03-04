ifeq (${ImportedIntoExtension},)
  ifeq (${BundledLrs},yes)
    StaticLibs := lrs
  endif
  LIBS += $(subst %A,${A},$(LrsLib))

  lrs_interface$O : ExtraCXXFLAGS = $(LrsCflags)
endif


# Local Variables:
# mode: Makefile
# End:
