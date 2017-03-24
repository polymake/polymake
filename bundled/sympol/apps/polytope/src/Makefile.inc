ifeq (${ImportedIntoExtension},)
  ifeq (${BundledSympol},yes)
    StaticLibs := sympol
  endif

  LIBS += $(subst %A,${A},${SympolLib})
endif

# Local Variables:
# mode: Makefile
# End:
