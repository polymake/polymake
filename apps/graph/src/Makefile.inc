ifneq ($(filter 4.8% 4.9%,${GCCversion}),)
  bipartite_signature$O : ExtraCXXFLAGS += -O1
endif

ifneq ($(filter 4.8% 4.9%,${GCCversion}),)
  perl/auto-diameter$O : ExtraCXXFLAGS += -fno-stack-protector
endif

# Local Variables:
# mode: Makefile
# End:
