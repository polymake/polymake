# input for generate_ninja_targets.pl

if ($ConfigFlags{'bundled.sympol.UseBundled'}) {
  my $no_warnings="-Wno-deprecated-declarations";
  foreach (qw(shadow conversion zero-as-null-pointer-constant unused-but-set-variable)) {
     if ($ConfigFlags{CXXFLAGS} =~ /-W$_\b/) {
	$no_warnings .= " -Wno-$_";
     }
  }

  ( staticlib => {
       SOURCEDIR => '${root}/bundled/sympol/external/sympol/sympol',
       SOURCES => [ qw( configuration.cpp facesuptosymmetrylist.cpp polyhedron.cpp polyhedrondatastorage.cpp polyhedronio.cpp qarray.cpp raycomputationcdd.cpp raycomputationlrs.cpp
                        recursionstrategy.cpp
                        yal/reportlevel.cpp symmetrycomputation.cpp symmetrycomputationadm.cpp symmetrycomputationdirect.cpp symmetrycomputationidm.cpp
                        symmetrygroupconstruction/graphconstructiondefault.cpp symmetrygroupconstruction/matrixconstruction.cpp symmetrygroupconstruction/matrixconstructiondefault.cpp ) ],
       'raycomputationcdd.cpp' => '-DGMPRATIONAL ${bundled.cdd.CFLAGS}',
       'raycomputationlrs.cpp' => '-DGMP -DMA ${bundled.lrs.CFLAGS}',
       CXXFLAGS => $no_warnings . ($ConfigFlags{ExternalHeaders} =~ /\bpermlib\b/ ? ' -I${root}/include/external/permlib' : ''),
    },
  )
} else {
  ()
}
