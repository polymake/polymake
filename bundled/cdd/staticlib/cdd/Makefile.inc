OwnLibrary := libcddgmp
LibModules := cddio cddmp cddlib cddcore cddlp cddproj setoper
LibModules := $(sort ${LibModules} $(patsubst cdd%,cdd%_f,${LibModules}))

ExtraCFLAGS := -DGMPRATIONAL

src := ${ExtensionTop}/external/cdd/lib-src-gmp

vpath %.c $(src)

# Local Variables:
# mode: Makefile
# End:
