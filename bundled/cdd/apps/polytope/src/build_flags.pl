# input for generate_ninja_targets.pl

my $cdd_src = '${root}/bundled/cdd/external/cdd/lib-src';
my $generated_dir = '${buildroot}/staticlib/cdd';

my @generic = qw( setoper.h setoper.c splitmix64.h );
my @dualabi = qw( cdd.h cddmp.h cddtypes.h
                  cddcore.c cddio.c cddlib.c cddlp.c cddmp.c cddproj.c );
my @fullsrc = (@generic, map { ($_, s/\./_f./r) } @dualabi);

my @generated_in = map { "$cdd_src/$_" } (@generic, @dualabi);
my @generated_out = map { "$generated_dir/$_" } @fullsrc;

# sympol and cdd interface compete for initialization of cdd global variables
my $standalone_global_init = $ConfigFlags{BundledExts} =~ /\bsympol\b/ ? '' : ' -DPOLYMAKE_CDD_STANDALONE_GLOBAL_INIT';

( 'cdd_interface.cc' => '${bundled.cdd.CFLAGS}'.$standalone_global_init,
  'cdd_float_interface.cc' => '${bundled.cdd.CFLAGS}',

  $ConfigFlags{'bundled.cdd.UseBundled'}
  ? ( GENERATED => {
        out => "@generated_out", in => "@generated_in",
        command => "cd $generated_dir; ${root}/bundled/cdd/support/generate_files.sh $cdd_src $generated_dir @dualabi",
      },
      staticlib => {
        LIBNAME => 'cddgmp',
        SOURCEDIR => $generated_dir,
        SOURCES => [ grep { /\.c$/ } @fullsrc ],
        CFLAGS => "-DGMPRATIONAL -Wno-unused-result -Wno-format-extra-args ".(defined $ConfigFlags{CLANGversion} ? '-Wno-incompatible-pointer-types-discards-qualifiers' : '-Wno-discarded-qualifiers'),
      } )
  : ()
)
