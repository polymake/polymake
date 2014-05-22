ifneq ($(filter 4.8% 4.9%,${GCCversion}),)
  bipartite_signature$O : ExtraCXXFLAGS += -O1
endif

# Local Variables:
# mode: Makefile
# End:
