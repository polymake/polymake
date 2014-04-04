ifeq (${ExtensionTop},)
  StaticLibs := cdd lrs

  lrs_interface$O : ExtraCXXFLAGS = -I$(ProjectTop)/external/lrs
  cdd_interface$O : ExtraCXXFLAGS = -I$(ProjectTop)/external/cdd/lib-src-gmp $(call addinclude, $(ProjectTop)/staticlib/cdd/globals_gmp.h)
  cdd_float_interface$O : ExtraCXXFLAGS = -I$(ProjectTop)/external/cdd/lib-src
  to_interface$O : ExtraCXXFLAGS = -I$(ProjectTop)/external/TOSimplex

  LIBS += $(BuildDir)/staticlib/lrs/liblrsgmp$A $(BuildDir)/staticlib/cdd/libcddgmp$A $(BuildDir)/staticlib/cdd/libcdd$A
endif

ifneq ($(filter 4.4% 4.5%,${GCCversion}),)
  core_point_algo$O rel_int_point$O pointed_part$O : ExtraCXXFLAGS += -fno-strict-aliasing
endif

ifneq ($(filter 4.2%,${GCCversion}),)
  perles_irrational_8_polytope$O : ExtraCXXFLAGS += -fno-strict-aliasing
endif

# Local Variables:
# mode: Makefile
# End:
