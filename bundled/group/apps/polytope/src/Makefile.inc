ifeq (${ImportedIntoExtension},)
  StaticLibs := sympol

  sympol_interface$O : ExtraCXXFLAGS += -I${ExtensionTop}/external/sympol
  sympol_raycomputation_beneathbeyond$O : ExtraCXXFLAGS += -I${ExtensionTop}/external/sympol

  LIBS += ${BuildDir}/staticlib/sympol/libsympol$A $(subst %A,${A},${CddLib} ${LrsLib})
endif

ifneq ($(filter 4.4%,${GCCversion}),)
  universal_polytope_ilps$O : ExtraCXXFLAGS += -O1
endif

# Local Variables:
# mode: Makefile
# End:
