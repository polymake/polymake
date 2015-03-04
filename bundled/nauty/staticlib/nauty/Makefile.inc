OwnLibraries := libnauty
LibModules := nauty naututil nautil nautinv naugraph rng
src := $(ExtensionTop)/external/nauty
vpath %.c $(src)

# We need to disable avx2 for some gcc versions as it produces invalid relocations
# see Ticket #800 and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57777
ifdef GCCversion
  ifneq ($(filter 4.7.0 4.7.1 4.7.2 4.7.3 4.8.0 4.8.1 4.8.2,${GCCversion}),)
    Cflags := $(Cflags) -mno-avx2
  endif
endif

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
