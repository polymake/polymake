ifneq ($(filter 4.2%,${GCCversion}),)
  stiefel_whitney$O : ExtraCXXFLAGS += -fno-strict-aliasing
endif

# Local Variables:
# mode: Makefile
# End:
