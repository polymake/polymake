OwnLibraries := libsympol
LibModules := polyhedron polyhedrondatastorage qarray reportlevel raycomputationlrs raycomputationcdd recursionstrategy facesuptosymmetrylist configuration symmetrycomputationadm symmetrycomputationidm symmetrycomputation symmetrycomputationdirect graphconstructiondefault matrixconstruction matrixconstructiondefault 
src := $(ExtensionTop)/external/sympol/sympol $(ExtensionTop)/external/sympol/sympol/yal $(ExtensionTop)/external/sympol/sympol/symmetrygroupconstruction

vpath %.cpp $(src)

libsympol : ExtraCXXFLAGS := -I$(ExtensionTop)/external/permlib/include -I$(ExtensionTop)/external/sympol/sympol -I$(ProjectTop)/external/lrs -DGMP -DLRS_QUIET -DGMPRATIONAL -I$(ProjectTop)/external/cdd/lib-src-gmp $(call addinclude, $(ProjectTop)/staticlib/cdd/globals_gmp.h)

# Local Variables:
# mode: Makefile
# End:
