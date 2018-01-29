# input for generate_ninja_targets.pl

( 'lrs_interface.cc' => '${bundled.lrs.CFLAGS}',

  $ConfigFlags{'bundled.lrs.UseBundled'}
  ? ( staticlib => {
        LIBNAME => 'lrsgmp',
        SOURCEDIR => '${root}/bundled/lrs/external/lrs',
        SOURCES => [ qw(lrslib.c lrsgmp.c) ],
        CFLAGS => '-DGMP -DLRS_QUIET',
      } )
  : ()
)
