ExtraCXXFLAGS = -I$(ExtensionTop)/external/libnormaliz

# force a recent C++ standard for normaliz but use
# tr1/unordered_{map,set} for polymake data structures
ifeq ($(filter -std=c++11 -std=gnu++11 -std=c++0x -std=gnu++0x,$(CXXflags)),)
  ExtraCXXFLAGS += -std=c++0x -DPM_FORCE_TR1
endif

# Local Variables:
# mode: Makefile
# End:
