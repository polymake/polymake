ifneq ($(filter 4.4%,${GCCversion}),)
  ppl_interface$O : ExtraCXXFLAGS += -fno-strict-aliasing
endif

