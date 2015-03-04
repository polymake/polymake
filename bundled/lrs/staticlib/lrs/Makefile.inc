OwnLibraries := liblrsgmp
LibModules := lrslib lrsgmp
src := $(ExtensionTop)/external/lrs
vpath %.c $(src)

liblrsgmp : ExtraCFLAGS := -DGMP -DLRS_QUIET

# Local Variables:
# mode: Makefile
# End:
