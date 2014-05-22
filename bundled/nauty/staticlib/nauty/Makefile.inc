OwnLibraries := libnauty
LibModules := nauty naututil nautil nautinv naugraph rng
src := $(ExtensionTop)/external/nauty
vpath %.c $(src)

ifeq "$(wildcard nauty.h)" ""
  override _system := $(shell uname -s)
  ifeq "$(_system)" "Darwin"
    override _hide := mv Makefile Makefile.hidden;
    override _restore := ;mv Makefile.hidden Makefile
  endif

  override _dummy := $(shell $(_hide) CC=$(CC) CFLAGS="$(Cflags)" $(src)/configure --quiet 2>/dev/null; rm makefile $(_restore))
endif

ExtraCFLAGS := -DBIGNAUTY -I.
ExtraCLEAN := nauty.h naututil.h gtools.h config.*

# Local Variables:
# mode: Makefile
# End:
