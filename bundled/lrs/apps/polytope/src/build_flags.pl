# input for generate_ninja_targets.pl

# sympol and lrs interface compete for initialization of lrs FILE handles
my $standalone_global_init = $ConfigFlags{BundledExts} =~ /\bsympol\b/ ? '' : ' -DPOLYMAKE_LRS_STANDALONE_GLOBAL_INIT';

( 'lrs_interface.cc' => '${bundled.lrs.CFLAGS}'.$standalone_global_init,

  $ConfigFlags{'bundled.lrs.UseBundled'}
  ? ( staticlib => {
        LIBNAME => 'lrsgmp',
        SOURCEDIR => '${root}/bundled/lrs/external/lrs',
        SOURCES => [ qw(lrslib.c lrsgmp.c lrsdriver.c) ],
        CFLAGS => '-DGMP -DMA -DLRS_QUIET',
      } )
  : ()
)
