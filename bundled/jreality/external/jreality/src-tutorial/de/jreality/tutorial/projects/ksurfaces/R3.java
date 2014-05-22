package de.jreality.tutorial.projects.ksurfaces;

/** This class provides simple static methods to do verctor calculation in R<sup>3</sup>. 
 * The vectors of R<sup>3</sup> are represented by array of 3 doubles. 
 * 
 */
public class R3 {
	/** target = a + b
	 * 
	 * @param a array of 3 doubles
	 * @param b array of 3 doubles
	 * @param target array of 3 doubles
	 */
	public final static void plus(double[] a, double[] b, double[] target) {
		target[0] = a[0] + b[0];
		target[1] = a[1] + b[1];
		target[2] = a[2] + b[2];
	}
	
	/** target = a-b
	 * 
	 * @param a array of 3 doubles
	 * @param b array of 3 doubles
	 * @param target array of 3 doubles
	 */
	public final static void minus(double[] a, double[] b, double[] target) {
		target[0] = a[0] - b[0];
		target[1] = a[1] - b[1];
		target[2] = a[2] - b[2];
	}

	/** target = a x b, i.e., the <a href="http://en.wikipedia.org/wiki/Crossproduct">cross product or vector product</a><br>
	 * 
	 * WARNING:  This method fails when the target is same as one of its arguments. 
	 * 
	 * @param a array of 3 doubles
	 * @param b array of 3 doubles
	 * @param target array of 3 doubles
	 */
	public final static void cross(double[] a, double[] b, double[] target) {
		double x = -a[1]*b[2] + a[2]*b[1];
		double y = -a[2]*b[0] + a[0]*b[2];
		target[2] = -a[0]*b[1] + a[1]*b[0];
		target[0] = x;
		target[1] = y;
	}

	/** target = a, copy the entries of a into target
	 *  
	 * @param a array of 3 doubles
	 * @param target array of 3 doubles
	 */
	public final static void copy(double[] a, double[] target) {
		target[0] = a[0];
		target[1] = a[1];
		target[2] = a[2];
	}

	/** target = 0, reset the three entries of target to 0
	 *  
	 * @param target array of 3 doubles
	 */
	public final static void zero(double[] target) {
		target[0] = 0;
		target[1] = 0;
		target[2] = 0;
	}
	
	/** &lt;a,b&gt;, i.e., the <a href="http://en.wikipedia.org/wiki/Dotproduct">dot product or inner product</a>
	 * 
	 * @param a array of 3 doubles
	 * @param b array of 3 doubles
	 * @return the value of the dot product
	 */
	public final static double dot(double[] a, double[] b) {
		return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
	}
	
	/** target = s * a, product of a real with a vector
	 * 
	 * @param s the scalar real 
	 * @param a array of 3 doubles
	 * @param target array of 3 doubles
	 */
	public final static void times(double s, double[]a, double[]target) {
		target[0] = s * a[0];
		target[1] = s * a[1];
		target[2] = s * a[2];
	}

	/** |a|, the length of a vector.
	 * 
	 * @param a array of 3 doubles
	 * @return the length of a 
	 */
	public static double norm(double[] a) {
		return Math.sqrt(normSquared(a));
	}

	/** |a|^2, the square of the length of a vector.
	 * 
	 * @param a array of 3 doubles
	 * @return the square of the length of a
	 */
	public static double normSquared(double[] a) {
		return a[0]*a[0]+a[1]*a[1]+a[2]*a[2];
	}
}
