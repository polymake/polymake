####
# Step 0: load helper functions
####
script("3d_printing_helper_functions.pl");



####
# Step 1: Generate tropical surface as a _polyhedral complex_
####

# tropical quadratic surface via tropical polynomial
$f = toTropicalPolynomial("min(1+2*w,1+2*x,1+2*y,1+2*z,w+x,w+y,w+z,x+y,x+z,y+z)");
$tropicalSurfaceTmp = new tropical::Hypersurface<Min>(POLYNOMIAL=>$f);

# # ALTERNATIVE: via exponent matrix and coefficient vector
# $exponentVectors = [[2,0,0,0], [1,1,0,0], [1,0,1,0], [1,0,0,1], [0,1,1,0], [0,1,0,1], [0,0,1,1], [0,2,0,0], [0,0,2,0], [0,0,0,2]];
# $coefficients = [1,0,0,0,0,0,0,1,1,1];
# $tropicalSurface = new Hypersurface<Min>(MONOMIALS=>$exponentVectors, COEFFICIENTS=>$coefficients);

# converting Hypersurface to PolyhedralComplex
$tropicalSurface = new fan::PolyhedralComplex(VERTICES=>$tropicalSurfaceTmp->VERTICES->minor(All,~[1]),MAXIMAL_POLYTOPES=>$tropicalSurfaceTmp->MAXIMAL_POLYTOPES,INPUT_LINEALITY=>$tropicalSurfaceTmp->LINEALITY_SPACE->minor(All,~[1]));



####
# Step 2: create bounding box
####

# create bounding box via generateBoundingBox
$boundingBox = generateBoundingBox($tropicalSurface);

# # ALTERNATIVE: create a bounding box manually
# $boundingBox = scale(cube(3),3);



####
# Step 3: create .scad file
####

$tropicalSurfaceBounded = intersectWithBoundingBox($tropicalSurface,$boundingBox);

$filename = "tropicalSurface.scad";
generateSCADFileForSurface($tropicalSurfaceBounded,$filename);
