ifeq (${ExtensionTop},)
  to_interface$O : ExtraCXXFLAGS = -I$(ProjectTop)/external/TOSimplex
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
