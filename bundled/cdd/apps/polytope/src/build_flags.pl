# input for generate_ninja_targets.pl

# sympol and cdd interface compete for initialization of cdd global variables
my $standalone_global_init = $ConfigFlags{BundledExts} =~ /\bsympol\b/ ? '' : ' -DPOLYMAKE_CDD_STANDALONE_GLOBAL_INIT';

( 'cdd_interface.cc' => '${bundled.cdd.CFLAGS}'.$standalone_global_init,
  'cdd_float_interface.cc' => '${bundled.cdd.CFLAGS}',

  $ConfigFlags{'bundled.cdd.UseBundled'}
  ? ( staticlib => {
        LIBNAME => 'cddgmp',
	SOURCEDIR => '${root}/bundled/cdd/external/cdd/lib-src-gmp',
	SOURCES => [ qw(cddio.c cddio_f.c cddmp.c cddmp_f.c cddlib.c cddlib_f.c cddcore.c cddcore_f.c cddlp.c cddlp_f.c cddproj.c cddproj_f.c setoper.c) ],
        CFLAGS => '-DGMPRATIONAL -Wno-unused-result '.(defined $ConfigFlags{CLANGversion} ? '-Wno-incompatible-pointer-types-discards-qualifiers' : '-Wno-discarded-qualifiers'),
      } )
  : ()
)
