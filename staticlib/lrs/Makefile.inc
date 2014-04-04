OwnLibraries := liblrsgmp
LibModules := lrslib lrsgmp
src := $(ProjectTop)/external/lrs
vpath %.c $(src)

liblrsgmp : ExtraCFLAGS := -DGMP -DLRS_QUIET

# Local Variables:
# mode: Makefile
# End:
