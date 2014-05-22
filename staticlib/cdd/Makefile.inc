OwnLibraries := libcdd libcddgmp
LibModules := cddio cddmp cddlib cddcore cddlp cddproj setoper

ExtraCFLAGS := -fvisibility=hidden 

ifeq "$(OwnLibraries)" "libcdd"
  src := $(ProjectTop)/external/cdd/lib-src
else
  src := $(ProjectTop)/external/cdd/lib-src-gmp

  LibModules := $(LibModules) $(patsubst cdd%,cdd%_f,$(LibModules))

  $(addsuffix $(LO), $(LibModules)) : $(SourceDir)/globals_gmp.h

  libcddgmp : ExtraCFLAGS += -DGMPRATIONAL $(call addinclude, $(SourceDir)/globals_gmp.h)

  _system := $(shell uname -m)
  ifeq "Power Macintosh" "$(_system)"
    $(SourceDir)/globals_gmp.h : $(wildcard $(src)/*.[ch])
	nm -g libcdd$A | perl -n -e '($$name)=/ [TCD] _(?!libcdd|ddd_)(\w+)/ or next; print "#define $$name $${name}_gmp\n"' >$@
  else
    $(SourceDir)/globals_gmp.h : $(wildcard $(src)/*.[ch])
	nm -g -P --defined-only libcdd$A | egrep -v '^(libcdd|ddd_)' | awk '{ print("#define " $$1 " " $$1 "_gmp") }' >$@
  endif
endif

vpath %.c $(src)

# Local Variables:
# mode: Makefile
# End:
