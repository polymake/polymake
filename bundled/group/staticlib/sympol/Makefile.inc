OwnLibrary := libsympol
LibModules := polyhedron polyhedrondatastorage qarray reportlevel raycomputationlrs raycomputationcdd recursionstrategy facesuptosymmetrylist configuration symmetrycomputationadm symmetrycomputationidm symmetrycomputation symmetrycomputationdirect graphconstructiondefault matrixconstruction matrixconstructiondefault 
src := ${ExtensionTop}/external/sympol/sympol ${ExtensionTop}/external/sympol/sympol/yal ${ExtensionTop}/external/sympol/sympol/symmetrygroupconstruction

vpath %.cpp $(src)

ExtraCXXFLAGS := -I${ExtensionTop}/external/permlib/include -I${ExtensionTop}/external/sympol/sympol ${LrsCflags} -DGMP -DLRS_QUIET ${CddCflags} -DGMPRATIONAL 

# Local Variables:
# mode: Makefile
# End:
