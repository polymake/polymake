####
# Step 0: load helper functions
####
script("3d_printing_helper_functions.pl");



####
# Step 1: Generate tropical curve as a _polyhedral complex_
####

# constructing tropical quadric surface
$mQuadratic = [[2,0,0,0], [1,1,0,0], [1,0,1,0], [1,0,0,1],
               [0,1,1,0], [0,1,0,1], [0,0,1,1], [0,2,0,0],
               [0,0,2,0], [0,0,0,2]];
$cQuadratic = [1,-1/4,-2/4,-3/4,-3/4,-4/4,-5/4,2/4,0,-2/4];
$TQuadratic = new tropical::Hypersurface<Min>(MONOMIALS=>$mQuadratic,
                                              COEFFICIENTS=>$cQuadratic);

# constructing tropical cubic surface
$mCubic = [[3,0,0,0], [0,3,0,0], [0,0,3,0], [0,0,0,3],
           [1,1,1,0], [1,1,0,1], [1,0,1,1], [0,1,1,1],
           [2,1,0,0], [2,0,1,0], [2,0,0,1], [1,2,0,0],
           [1,0,2,0], [1,0,0,2], [0,2,1,0], [0,2,0,1],
           [0,1,2,0], [0,1,0,2], [0,0,2,1], [0,0,1,2]];
$cCubic = [3,3,3,3,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1];
$TCubic = new tropical::Hypersurface<Min>(MONOMIALS=>$mCubic,
                                          COEFFICIENTS=>$cCubic);

# constructing a tropical sextic curve as the stable intersection
#   of a quadratic and a cubic surface
$tropicalCurveTmp = tropical::intersect($TQuadratic,$TCubic);
$tropicalCurve = new fan::PolyhedralComplex(VERTICES=>$tropicalCurveTmp->VERTICES->minor(All,~[1]),MAXIMAL_POLYTOPES=>$tropicalCurveTmp->MAXIMAL_POLYTOPES,INPUT_LINEALITY=>$tropicalCurveTmp->LINEALITY_SPACE->minor(All,~[1]));



####
# Step 2: generate a tropical surface for framing. The surface should contains the curve and be a _polyhedral complex_
####

# using the previously constructed tropical quadratic surface
$tropicalSurface = $TQuadratic;
$tropicalSurface = new fan::PolyhedralComplex(VERTICES=>$tropicalSurface->VERTICES->minor(All,~[1]),MAXIMAL_POLYTOPES=>$tropicalSurface->MAXIMAL_POLYTOPES,INPUT_LINEALITY=>$tropicalSurface->LINEALITY_SPACE->minor(All,~[1]));



####
# Step 3: create a bounding box
####

$boundingBox = generateBoundingBox($tropicalCurve);

# # ALTERNATIVE: create a bounding box manually
# $boundingBox = scale(cube(3),3);



####
# Step 4: create .scad file
####
$tropicalSurfaceFrame = intersectWithBoundingBoxForFraming($tropicalSurface,$boundingBox);
$tropicalCurveBounded = intersectWithBoundingBox($tropicalCurve,$boundingBox);

$filename = "tropicalCurve.scad";
generateSCADFileForCurve($tropicalSurfaceFrame,$tropicalCurveBounded,$filename);
