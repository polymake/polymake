ExtraCXXFLAGS = -I$(ExtensionTop)/external/libnormaliz

# force a recent C++ standard for normaliz but keep it compatible with older
# gcc versions and use tr1/unordered_{map,set} for polymake data structures
OldStd := $(shell ${PERL} -e "${CPPStd} < 201103 && print 'true'")
ifeq (${OldStd},true)
  ExtraCXXFLAGS += -std=c++0x -DPM_FORCE_TR1
endif

# Local Variables:
# mode: Makefile
# End:
