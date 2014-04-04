package de.jreality.tutorial.projects.ksurfaces;


/**  The algorithms to generate the Gauss map of a K-surface and the surface. The arrays are in xt--coordinates, 
 * where the first index is time t and the second is the position x.  
 * 
 * @author Ulrich Pinkall, adapted for the tutorial by G. Paul Peters, 24.07.2009
 * @see Ulrich Pinkall: "Designing Cylinders with Constant Negative Curvature", in 
 * Discrete Differential Geometry, pages 57-66. Springer 2008.
 *
 */
public class KSurfaces {
	/** Computes the Gauss map of a K-surface from initial Cauchy data, i.e., two closed curves. Assums that the points of
	 * the initial data lie on the 2-sphere this method generates more points so that the quadrilaterals 
	 * (i-1,j-1), (i-1,j), (i,j),(i-2,j-1) form spherical parallelograms. 
	 * 
	 * @param initialAnnulus the initial data, i.e., a double array double[2][n][3] containing 2 polygons with n vertices,
	 *  which are both interpreted as closed curves connecting that last and the first one. 
	 * @param target a double array that will be filled with the result, i.e., an array double[m][n][3], where m&gt;1 is the number of
	 * 	time steps to be calculated.
	 */
	public static void gaussMapFromInitialAnnulus(double[][][] initialAnnulus, double[][][] target) {
		final int m = target.length;
		final int n = target[0].length;
		final double[] a = new double[3];
		
		// copy first two rows
		for (int i=0; i<2; i++) {
			for (int j=0; j<n; j++) {
				R3.copy(initialAnnulus[i][j], target[i][j]);
			}
		}
		
		// compute the other rows
		for (int i=2; i<m; i++) {
			for (int j=0; j<n; j++) {
				int k = j==0 ? n-1 : j-1;
				R3.plus(target[i-1][k], target[i-1][j], a);
				double[] w = target[i-2][k];
				double s = 2 * R3.dot(a,w) / R3.dot(a,a);
				R3.times(s, a, a);
				R3.minus(a, w, target[i][j]);
			}
		}
	}
	
	/** Calculates the K-surface from the given Gauss map, assuming that the Gauss map consists
	 * of spherical parallelograms as described in {@link #gaussMapFromInitialAnnulus(double[][][], double[][][])}. 
	 * 
	 * @param gaussMap the given Gauss map.
	 * @param target a double array with enough space to hold the resulting surface. A quad mesh where the
	 * quadrilaterals consist of (i-1,j-1), (i-1,j), (i,j),(i-2,j-1).
	 */
	public static void kSurfaceFromGaussMap(double[][][] gaussMap, double[][][] target) {
		final int m = gaussMap.length;
		final int n = gaussMap[0].length;
		final double[] a = new double[3];

		// compute first row
		R3.zero(target[0][0]);
		for (int j=0; j<n-1; j++) {
			int k = (j+1)%n;
			R3.plus(gaussMap[0][j], gaussMap[0][k], a);
			R3.cross(gaussMap[1][k], a, a);
			R3.plus(target[0][j], a, target[0][k]);
		}
			
		// compute the other rows
		for (int i=1; i<m; i++) {
			for (int j=0; j<n; j++) {
				R3.cross(gaussMap[i-1][j], gaussMap[i][j], a);
				R3.plus(target[i-1][j], a, target[i][j]);
			}
		}
	}
}
