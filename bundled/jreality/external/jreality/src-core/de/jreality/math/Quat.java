package de.jreality.math;

/**
 * This class provides static methods that interpret 4-vectors (double-arrays) as Quaternions.
 * 
 * @author weissman
 *
 */
public class Quat {

	private Quat() {}
	
	public static double[] add(double[] dst, double[] a, double[] b) {
		if (dst == null) dst = new double[4];
		dst[0]=a[0]+b[0];
		dst[1]=a[1]+b[1];
		dst[2]=a[2]+b[2];
		dst[3]=a[3]+b[3];
		return dst;
	}
	
	public static double[] subtract(double[] dst, double[] a, double[] b) {
		if (dst == null) dst = new double[4];
		dst[0]=a[0]-b[0];
		dst[1]=a[1]-b[1];
		dst[2]=a[2]-b[2];
		dst[3]=a[3]-b[3];
		return dst;
	}
	
	public static double[] times(double[] dst, double[] a, double[] b) {
		double[] tmp = new double[] {
		  a[0]*b[0]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3],
	      a[0]*b[1]+a[1]*b[0]+a[2]*b[3]-a[3]*b[2],
	      a[0]*b[2]-a[1]*b[3]+a[2]*b[0]+a[3]*b[1],
	      a[0]*b[3]+a[1]*b[2]-a[2]*b[1]+a[3]*b[0]
		};
		if (dst != null) System.arraycopy(tmp, 0, dst, 0, 4);
		return tmp;
	}
	
	public static double[] times(double[] dst, double a, double[] b) {
		double[] tmp = new double[] {
		  a*b[0],
	      a*b[1],
	      a*b[2],
	      a*b[3]
		};
		if (dst != null) System.arraycopy(tmp, 0, dst, 0, 4);
		return tmp;
	}
	
	public static double[] invert(double[] dst, final double[] a) {
		double ll = lengthSqared(a);
		if (dst == null) dst = new double[4];
		if (ll==0.0) {
			dst[0] = Double.POSITIVE_INFINITY;
			dst[1]=0;
			dst[2]=0;
			dst[3]=0;
		} else {
			dst[0] = a[0]/ll;
			dst[1] = -a[1]/ll;
			dst[2] = -a[2]/ll;
			dst[3] = -a[3]/ll;
		}
		return dst;
	}

	public static double lengthSqared(double[] a) {
		return Rn.euclideanNormSquared(a);
	}
	
	public static double length(double[] a) {
		return Rn.euclideanNorm(a);
	}
	
	public static double re(double[] a) {
		return a[0];
	}
	
	public static double[] im(double[] a) {
		return new double[] {a[1], a[2], a[3]};
	}
	
	public static double[] toQuat(double[] dst, double re, double[] im) {
		if (dst == null) dst = new double[4];
		dst[0]=re;
		System.arraycopy(im, 0, dst, 1, 3);
		return dst;
	}
	
	public static double[] toQuat(double[] dst, double re, double x, double y, double z) {
		if (dst == null) dst = new double[4];
		dst[0]=re;
		dst[1]=x;
		dst[2]=y;
		dst[3]=z;
		return dst;
	}

	/**
	 * compute the conjugation of a by b: b*a*b^-1
	 * @param dst the destination
	 * @param a 
	 * @param b
	 * @return the result
	 */
	public static double[] conjugateBy(double[] dst, double[] a, double[] b) {
		double[] tmp = Quat.invert(null, b);
		Quat.times(tmp, a, tmp); // tmp = a * b^-1
		Quat.times(tmp, b, tmp); // tmp = b * a * b^-1
		if (dst == null) return tmp;
		System.arraycopy(tmp, 0, dst, 0, 4);
		return dst;
	}
	
	/**
	 * return the conjugate quaternion: a_bar = a.re - a.im
	 * @param dst the destination
	 * @param a the quaternion to conjugate
	 * @return
	 */
	public static double[] conjugate(double[] dst, double[] a) {
		if (dst == null) dst = new double[4];
		dst[0]=a[0];
		dst[1]=-a[1];
		dst[2]=-a[2];
		dst[3]=-a[3];
		return dst;
	}
}
