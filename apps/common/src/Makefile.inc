ifneq ($(filter 4.2%,${GCCversion}),)
  perl/Plucker$O : ExtraCXXFLAGS += -fno-strict-aliasing
endif

# Local Variables:
# mode: Makefile
# End:
