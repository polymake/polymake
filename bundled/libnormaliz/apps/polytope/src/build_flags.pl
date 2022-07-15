# input for generate_ninja_targets.pl

my $nmz_src = '${root}/bundled/libnormaliz/external/libnormaliz';

( $ConfigFlags{'bundled.libnormaliz.UseBundled'}
  ? (
      'staticlib' => {
          SOURCEDIR => "$nmz_src/libnormaliz",
          SOURCES => [ qw(automorph.cpp chunk.cpp collection.cpp cone.cpp cone_dual_mode.cpp cone_property.cpp descent.cpp face_lattice.cpp full_cone.cpp general.cpp HilbertSeries.cpp input.cpp matrix.cpp nmz_hash.cpp nmz_integral.cpp nmz_nauty.cpp options.cpp output.cpp project_and_lift.cpp reduction.cpp signed_dec.cpp simplex.cpp sublattice_representation.cpp) ],
          CXXFLAGS => "$ConfigFlags{'bundled.libnormaliz.BundledNoWarnings'} -I$nmz_src",
       }
    )
  : ( )
)
