OwnLibrary := libnauty
LibModules := nauty naututil nautil nautinv naugraph rng naurng schreier nausparse nautaux
ifeq (${NautySrc},)
  src := ${ExtensionTop}/external/nauty
else
  src := ${NautySrc}
endif
vpath %.c $(src)

# We need to disable avx2 for some gcc versions as it produces invalid relocations
# see Ticket #800 and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57777
ifdef GCCversion
  ifneq ($(filter 4.7.0 4.7.1 4.7.2 4.7.3 4.8%,${GCCversion}),)
    Cflags := $(Cflags) -mno-avx2
  endif
endif

ifeq "$(wildcard nauty.h)" ""
  override _system := $(shell uname -s)
  ifeq "$(_system)" "Darwin"
    override _hide := mv Makefile Makefile.hidden;
    override _restore := ;mv Makefile.hidden Makefile
  endif

  override _dummy := $(shell ${_hide} CC=${CC} CFLAGS="${Cflags}" ${src}/configure --quiet 2>/dev/null; rm makefile ${_restore})
endif

# force include nauty headers here on the command line to make sure that we pick
# up the newly generated ones instead (wrong?) ones from the original tarball
# (current directory of the source file is always searched first and -I- is deprecated)
ExtraCFLAGS := -DBIGNAUTY -I. -include $(CURDIR)/nauty.h -include $(CURDIR)/naututil.h -include $(CURDIR)/gtools.h -I${src}
ExtraCLEAN := nauty.h naututil.h gtools.h config.*

# Local Variables:
# mode: Makefile
# End:
