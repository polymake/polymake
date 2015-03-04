OwnLibraries := libcddgmp
LibModules := cddio cddmp cddlib cddcore cddlp cddproj setoper
LibModules := $(LibModules) $(patsubst cdd%,cdd%_f,$(LibModules))

ExtraCFLAGS := -fvisibility=hidden

src := $(ExtensionTop)/external/cdd/lib-src-gmp
libcddgmp : ExtraCFLAGS += -DGMPRATIONAL

vpath %.c $(src)

# Local Variables:
# mode: Makefile
# End:
