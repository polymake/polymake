####
# Step 0: load helper functions
####
script("3d_printing_helper_functions.pl");



####
# Step 1: Generate tropical surface and curve as a _polyhedral complexes_
####

# constructing tropical plane via linear polynomial
$linearEquation = toTropicalPolynomial("max(y,z,w)", qw(w x y z));
$tropicalSurfaceTmp = new tropical::Hypersurface<Max>(POLYNOMIAL=>$linearEquation);
$tropicalSurface = new fan::PolyhedralComplex(POINTS=>$tropicalSurfaceTmp->VERTICES->minor(All,~[1]),INPUT_POLYTOPES=>$tropicalSurfaceTmp->MAXIMAL_POLYTOPES,INPUT_LINEALITY=>$tropicalSurfaceTmp->LINEALITY_SPACE->minor(All,~[1]));

# constructing tropical curve manually:
$tropicalCurveVertices = [[1,0,0,0],[1,-8,0,0],[1,-8,-6,0],[1,12,0,0],[0,0,-1,0],[1,-6,-6,0],[1,-11,-3,0],[1,-11,-6,0],[0,-1,0,0],[1,-12,-7,0],[1,4,0,-2],[1,10,0,-2],[1,8,0,-6],[0,0,0,-1],[0,1,1,1],[1,-4,6,6],[1,-4,5,5],[1,-3,5,5],[1,-5,4,4],[1,-3,3,3],[1,-5,3,3]];
$tropicalCurveCells = [[3,4],[0,5],[2,5],[4,5],[1,6],[6,7],[6,8],[2,7],[7,9],[8,9],[4,9],[10,11],[3,11],[11,12],[0,10],[10,12],[12,13],[1,13],[14,15],[8,15],[15,16],[14,17],[16,17],[16,18],[17,19],[8,18],[18,20],[0,19],[19,20],[1,20],[3,14]];
$tropicalCurve = new fan::PolyhedralComplex(POINTS=>$tropicalCurveVertices,INPUT_POLYTOPES=>$tropicalCurveCells);



####
# Step 2: create a bounding box
####
$boundingBox = generateBoundingBox($tropicalCurve);



####
# Step 3: create .scad file
####
$tropicalSurfaceBounded = intersectWithBoundingBox($tropicalSurface,$boundingBox);
$tropicalCurveBounded = intersectWithBoundingBox($tropicalCurve,$boundingBox);



$filename = "tropicalSurfaceAndCurve.scad";
generateSCADFileForSurfaceAndCurve($tropicalSurfaceBounded,$tropicalCurveBounded,$filename);
