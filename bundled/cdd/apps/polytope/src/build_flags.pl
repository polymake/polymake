# input for generate_ninja_targets.pl

( ( map { $_ => '${bundled.cdd.CFLAGS}' } qw(cdd_interface.cc cdd_float_interface.cc) ),

  $ConfigFlags{'bundled.cdd.UseBundled'}
  ? ( staticlib => {
        LIBNAME => 'cddgmp',
	SOURCEDIR => '${root}/bundled/cdd/external/cdd/lib-src-gmp',
	SOURCES => [ qw(cddio.c cddio_f.c cddmp.c cddmp_f.c cddlib.c cddlib_f.c cddcore.c cddcore_f.c cddlp.c cddlp_f.c cddproj.c cddproj_f.c setoper.c) ],
        CFLAGS => '-DGMPRATIONAL -Wno-unused-result '.(defined $ConfigFlags{CLANGversion} ? '-Wno-incompatible-pointer-types-discards-qualifiers' : '-Wno-discarded-qualifiers'),
      } )
  : ()
)
