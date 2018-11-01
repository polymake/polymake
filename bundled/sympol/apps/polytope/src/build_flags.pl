# input for generate_ninja_targets.pl

if ($ConfigFlags{'bundled.sympol.UseBundled'}) {
  ( staticlib => {
       SOURCEDIR => '${root}/bundled/sympol/external/sympol/sympol',
       SOURCES => [ qw( configuration.cpp facesuptosymmetrylist.cpp polyhedron.cpp polyhedrondatastorage.cpp polyhedronio.cpp qarray.cpp raycomputationcdd.cpp raycomputationlrs.cpp
                        recursionstrategy.cpp
                        yal/reportlevel.cpp symmetrycomputation.cpp symmetrycomputationadm.cpp symmetrycomputationdirect.cpp symmetrycomputationidm.cpp
                        symmetrygroupconstruction/graphconstructiondefault.cpp symmetrygroupconstruction/matrixconstruction.cpp symmetrygroupconstruction/matrixconstructiondefault.cpp ) ],
       'raycomputationcdd.cpp' => '-DGMPRATIONAL ${bundled.cdd.CFLAGS}',
       'raycomputationlrs.cpp' => '-DGMP -DMA ${bundled.lrs.CFLAGS}',
       $ConfigFlags{ExternalHeaders} =~ /\bpermlib\b/
       ? ( CXXFLAGS => '-I${root}/include/external/permlib' ) : (),
    },
  )
} else {
  ()
}
